#ifndef LIBTEDDY_DETAILS_HASH_TABLES_HPP
#define LIBTEDDY_DETAILS_HASH_TABLES_HPP

#include <libteddy/details/config.hpp>
#include <libteddy/details/debug.hpp>
#include <libteddy/details/node.hpp>
#include <libteddy/details/tools.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace teddy
{
/**
 *  \brief Base class for hash tables.
 */
class table_base
{
public:
    static auto get_gte_capacity (int64 capacity) -> int64;

private:
    static int64 constexpr Capacities[] {
        307,         617,         1'237,         2'477,        4'957,
        9'923,       19'853,      39'709,        79'423,       158'849,
        317'701,     635'413,     1'270'849,     2'541'701,    5'083'423,
        10'166'857,  20'333'759,  40'667'527,    81'335'063,   162'670'129,
        325'340'273, 650'680'571, 1'301'361'143, 2'602'722'289
    };
};

/**
 *  \brief Iterator for the unique table
 */
template<class Data, class Degree>
class unique_table_iterator
{
public:
    using node_t = node<Data, Degree>;

public:
    unique_table_iterator(node_t** firstBucket, node_t** lastBucket);
    unique_table_iterator(node_t** bucket, node_t** lastBucket, node_t* node);

public:
    auto operator++ () -> unique_table_iterator&;
    auto operator++ (int) -> unique_table_iterator;
    auto operator* () const -> node_t*;
    auto operator== (unique_table_iterator const& other) const -> bool;
    auto operator!= (unique_table_iterator const& other) const -> bool;
    auto get_bucket () const -> node_t**;

private:
    /**
     *  \brief Moves to the next non-empty bucket and return its head
     *  \return Head of the next non-empty bucket
     */
    auto move_to_next_bucket () -> node_t*;

private:
    node_t** bucket_;
    node_t** lastBucket_;
    node_t* node_;
};

/**
 *  \brief Table of unique nodes.
 */
template<class Data, class Degree>
class unique_table
{
public:
    using node_t        = node<Data, Degree>;
    using son_container = typename node_t::son_container;
    using iterator      = unique_table_iterator<Data, Degree>;

public:
    struct result_of_find
    {
        node_t* node_;
        std::size_t hash_;
    };

public:
    /**
     *  \brief Initializes empty table
     *  \param capacity Initial capacity
     *  \param domain Domain of nodes
     */
    unique_table(int64 capacity, int32 domain);

    /**
     *  \brief Copt constructor
     */
    unique_table(unique_table const&);

    /**
     *  \brief Move constructor
     */
    unique_table(unique_table&& other) noexcept;

    /**
     *  \brief Destructor
     */
    ~unique_table();

    auto operator= (unique_table const&) = delete;
    auto operator= (unique_table&&)      = delete;

public:
    /**
     *  \brief Tries to find an internal node
     *  \param sons Sons of the desired node
     *  \return Pointer to the node, nullptr if not found
     *          Hash of the node that can be used in insertion
     */
    [[nodiscard]] auto find (son_container const& sons) const -> result_of_find;

    /**
     *  \brief Adds all nodes from \p other into this table
     *  Adjusts capacity if necessary.
     *  \param other Table to merge into this one
     *  \param domain Number of sons in this and \p other
     */
    auto merge (unique_table other) -> void;

    /**
     *  \brief Inserts \p node using pre-computed \p hash
     *  \param node Node to be inserted
     *  \param hash Hash value of \p node
     */
    auto insert (node_t* node, std::size_t hash) -> void;

    /**
     *  \brief Erases node pointed to by \p it
     *  \param nodeIt Iterator to the node to be deleted
     *  \return Iterator to the next node
     */
    auto erase (iterator nodeIt) -> iterator;

    /**
     *  \brief Erases \p node
     *  \param node Node to be erased
     *  \return Iterator to the next node
     */
    auto erase (node_t* node) -> iterator;

    /**
     *  \brief Adjusts capacity of the table (number of buckets)
     */
    auto adjust_capacity () -> void;

    /**
     *  \return Number of nodes in the table
     */
    [[nodiscard]] auto get_size () const -> int64;

    /**
     *  \brief Clears the table
     */
    auto clear () -> void;

    /**
     *  \return Begin iterator
     */
    [[nodiscard]] auto begin () -> iterator;

    /**
     *  \return End iterator
     */
    [[nodiscard]] auto end () -> iterator;

    /**
     *  \return Const begin iterator
     */
    [[nodiscard]] auto begin () const -> iterator;

    /**
     *  \return Const end iterator
     */
    [[nodiscard]] auto end () const -> iterator;

private:
    /**
     *  \brief Adjusts capacity of the table (number of buckets)
     *  Does nothing if actual capacity is >= \p newCapacity
     *  \param newCapacity New capacity
     */
    auto rehash (int64 newCapacity) -> void;

    /**
     *  \return Current load factor
     */
    [[nodiscard]] auto get_load_factor () const -> double;

    /**
     *  \brief Inserts \p node using pre-computed \p hash
     *  Does NOT increase size
     *  \param node Node to be inserted
     *  \param hash Hash value of \p node
     */
    auto insert_impl (node_t* node, std::size_t hash) -> node_t*;

    /**
     *  \brief Erases \p node
     *  \param bucket Bucket containing \p node
     *  \param node Node to be erased
     *  \return Iterator to the next node
     */
    auto erase_impl (node_t** bucket, node_t* node) -> iterator;

    /**
     *  \brief Computes hash value of a node with \p sons
     *  \param sons Sons of the node
     *  \return Hash value of the node
     */
    [[nodiscard]] auto node_hash (son_container const& sons
    ) const -> std::size_t;

    /**
     *  \brief Compares two nodes for equality
     *  (whether they have the same sons)
     *  \param node First node
     *  \param sons Sons of the second node
     *  \return True if the nodes are equal, false otherwise
     */
    [[nodiscard]] auto node_equals (node_t* node, son_container const& sons)
        const -> bool;

    /**
     *  \brief Allocates \p count nullptr initialized buckets
     */
    [[nodiscard]] auto callocate_buckets (int64 count) -> node_t**;

    /**
     *  \brief Allocates \p count uninitialized buckets
     */
    [[nodiscard]] auto mallocate_buckets (int64 count) -> node_t**;

private:
    static double constexpr LOAD_THRESHOLD = 0.75;

private:
    int32 domain_;
    int64 size_;
    int64 capacity_;
    node_t** buckets_;
};

/**
 *  \brief Cache for the apply opertaion.
 */
template<class Data, class Degree>
class apply_cache
{
public:
    using node_t = node<Data, Degree>;

public:
    struct cache_entry
    {
        int32 opId_;
        node_t* lhs_;
        node_t* rhs_;
        node_t* result_;
    };

public:
    explicit apply_cache(int64 capacity);
    apply_cache(apply_cache&& other) noexcept;
    ~apply_cache();

    apply_cache(apply_cache const&)     = delete;
    auto operator= (apply_cache const&) = delete;
    auto operator= (apply_cache&&)      = delete;

public:
    /**
     *  \brief Looks up result of an operation
     *  \param opId id of the operation
     *  \param lhs first operand
     *  \param rhs second operand
     *  \result result of the previous operation or nullptr
     */
    auto find (int32 opId, node_t* lhs, node_t* rhs) -> node_t*;

    /**
     *  \brief Puts the result into the cache possibly overwriting old value
     *  \param opId id of the operation
     *  \param result result
     *  \param lhs first operand
     *  \param rhs second operand
     *  \result result of the previous operation or nullptr
     */
    auto put (int32 opId, node_t* result, node_t* lhs, node_t* rhs) -> void;

    /**
     *  \brief Increases the capacity so that it is close to \p aproxCapacity
     *  Never lowers the capacity!
     *  \param aproxCapacity new capacity
     */
    auto grow_capacity (int64 aproxCapacity) -> void;

    /**
     *  \brief Removes entries pointing to unused nodes
     */
    auto remove_unused () -> void;

    /**
     *  \brief Clears all entries
     */
    auto clear () -> void;

private:
    /**
     *  \return Current load factor
     */
    [[nodiscard]] auto get_load_factor () const -> double;

    /**
     *  \brief Adjusts capacity of the table (number of entries)
     *  Does nothing if actual capacity is >= \p newCapacity
     *  \param newCapacity New capacity
     */
    auto rehash (int64 newCapacity) -> void;

    /**
     *  \brief Allocates \p count nullptr initialized entries
     */
    [[nodiscard]] static auto callocate_entries (int64 count) -> cache_entry*;

private:
    int64 size_;
    int64 capacity_;
    cache_entry* entries_;
};

// table_base definitions:

inline auto table_base::get_gte_capacity(int64 const desiredCapacity) -> int64
{
    for (int64 const tableCapacity : Capacities)
    {
        if (tableCapacity > desiredCapacity)
        {
            return tableCapacity;
        }
    }
    return Capacities[std::size(Capacities) - 1];
}

// unique_table_iterator definitions:

template<class Data, class Degree>
unique_table_iterator<Data, Degree>::unique_table_iterator(
    node_t** const firstBucket,
    node_t** const lastBucket
) :
    bucket_(firstBucket),
    lastBucket_(lastBucket),
    node_(this->move_to_next_bucket())
{
}

template<class Data, class Degree>
unique_table_iterator<Data, Degree>::unique_table_iterator(
    node_t** const bucket,
    node_t** const lastBucket,
    node_t* const node
) :
    bucket_(bucket),
    lastBucket_(lastBucket),
    node_(node)
{
}

template<class Data, class Degree>
auto unique_table_iterator<Data, Degree>::operator++ ()
    -> unique_table_iterator&
{
    node_ = node_->get_next();
    if (not node_)
    {
        ++bucket_;
        node_ = this->move_to_next_bucket();
    }
    return *this;
}

template<class Data, class Degree>
auto unique_table_iterator<Data, Degree>::operator++ (int
) -> unique_table_iterator
{
    auto const tmp = *this;
    ++(*this);
    return tmp;
}

template<class Data, class Degree>
auto unique_table_iterator<Data, Degree>::operator* () const -> node_t*
{
    return node_;
}

template<class Data, class Degree>
auto unique_table_iterator<Data, Degree>::operator== (
    unique_table_iterator const& other
) const -> bool
{
    return bucket_ == other.bucket_ && node_ == other.node_;
}

template<class Data, class Degree>
auto unique_table_iterator<Data, Degree>::operator!= (
    unique_table_iterator const& other
) const -> bool
{
    return not (*this == other);
}

template<class Data, class Degree>
auto unique_table_iterator<Data, Degree>::get_bucket() const -> node_t**
{
    return bucket_;
}

template<class Data, class Degree>
auto unique_table_iterator<Data, Degree>::move_to_next_bucket() -> node_t*
{
    while (bucket_ != lastBucket_ && not *bucket_)
    {
        ++bucket_;
    }
    return bucket_ != lastBucket_ ? *bucket_ : nullptr;
}

// unique_table definitions:

template<class Data, class Degree>
unique_table<Data, Degree>::unique_table(
    int64 const capacity,
    int32 const domain
) :
    domain_(domain),
    size_(0),
    capacity_(table_base::get_gte_capacity(capacity)),
    buckets_(callocate_buckets(capacity_))
{
}

template<class Data, class Degree>
unique_table<Data, Degree>::unique_table(unique_table const& other) :
    domain_(other.domain_),
    size_(other.size_),
    capacity_(other.capacity_),
    buckets_(mallocate_buckets(other.capacity_))
{
    std::memcpy(
        buckets_,
        other.buckets_,
        static_cast<std::size_t>(capacity_) * sizeof(node_t*)
    );
}

template<class Data, class Degree>
unique_table<Data, Degree>::unique_table(unique_table&& other) noexcept :
    domain_(other.domain_),
    size_(utils::exchange(other.size_, 0)),
    capacity_(other.capacity_),
    buckets_(utils::exchange(other.buckets_, nullptr))
{
}

template<class Data, class Degree>
unique_table<Data, Degree>::~unique_table()
{
    std::free(buckets_);
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::find(son_container const& sons
) const -> result_of_find
{
    std::size_t const hash = this->node_hash(sons);
    auto const index
        = static_cast<int64>(hash % static_cast<std::size_t>(capacity_));
    node_t* current = buckets_[index];
    while (current)
    {
        if (this->node_equals(current, sons))
        {
            return {current, hash};
        }
        current = current->get_next();
    }
    return {nullptr, hash};
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::merge(unique_table other) -> void
{
    size_ += other.size_;
    this->adjust_capacity();
    auto it        = other.begin();
    auto const end = other.end();
    while (it != end)
    {
        node_t* const otherNode = *it;
        ++it;
        otherNode->set_next(nullptr);
        std::size_t const hash = this->node_hash(otherNode->get_sons());
        this->insert_impl(otherNode, hash);
    }
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::insert(
    node_t* const node,
    std::size_t const hash
) -> void
{
    this->insert_impl(node, hash);
    ++size_;
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::erase(iterator const nodeIt) -> iterator
{
    node_t** const bucket = nodeIt.get_bucket();
    node_t* const node    = *nodeIt;
    return this->erase_impl(bucket, node);
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::erase(node_t* const node) -> iterator
{
    std::size_t const hash = this->node_hash(node->get_sons());
    auto const index
        = static_cast<int64>(hash % static_cast<std::size_t>(capacity_));
    return this->erase_impl(buckets_ + index, node);
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::adjust_capacity() -> void
{
    auto const aproxCapacity
        = static_cast<int64>(static_cast<double>(size_) / LOAD_THRESHOLD);
    int64 const newCapacity = table_base::get_gte_capacity(aproxCapacity);
    if (newCapacity > capacity_)
    {
        this->rehash(newCapacity);
    }
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::get_size() const -> int64
{
    return size_;
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::clear() -> void
{
    size_ = 0;
    std::memset(
        buckets_,
        0,
        static_cast<std::size_t>(capacity_) * sizeof(node_t*)
    );
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::begin() -> iterator
{
    return iterator(buckets_, buckets_ + capacity_);
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::end() -> iterator
{
    return iterator(buckets_ + capacity_, buckets_ + capacity_);
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::begin() const -> iterator
{
    return iterator(buckets_, buckets_ + capacity_);
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::end() const -> iterator
{
    return iterator(buckets_ + capacity_, buckets_ + capacity_);
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::rehash(int64 const newCapacity) -> void
{
#ifdef LIBTEDDY_VERBOSE
    debug::out(
        "  unique_table::rehash\tload before ",
        this->get_load_factor(),
        " capacity is ",
        capacity_,
        " should be ",
        newCapacity
    );
#endif

    node_t** const oldBuckets = buckets_;
    int64 const oldCapacity   = capacity_;
    buckets_                  = callocate_buckets(newCapacity);
    capacity_                 = newCapacity;
    for (int64 i = 0; i < oldCapacity; ++i)
    {
        node_t* node = oldBuckets[i];
        while (node)
        {
            node_t* const next     = node->get_next();
            std::size_t const hash = this->node_hash(node->get_sons());
            node->set_next(nullptr);
            this->insert_impl(node, hash);
            node = next;
        }
    };

#ifdef LIBTEDDY_VERBOSE
    debug::out(", load after ", this->get_load_factor(), "\n");
#endif
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::get_load_factor() const -> double
{
    return static_cast<double>(size_) / static_cast<double>(capacity_);
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::insert_impl(
    node_t* const node,
    std::size_t const hash
) -> node_t*
{
    std::size_t const index = hash % static_cast<std::size_t>(capacity_);
    node_t* const bucket    = buckets_[index];
    if (bucket)
    {
        node->set_next(bucket);
    }
    buckets_[index] = node;
    return node;
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::erase_impl(
    node_t** const bucket,
    node_t* const node
) -> iterator
{
    iterator retIt(bucket, buckets_ + capacity_, node);
    ++retIt;
    --size_;

    if (*bucket == node)
    {
        *bucket = node->get_next();
        node->set_next(nullptr);
        return retIt;
    }

    node_t* prev = *bucket;
    while (prev->get_next() != node)
    {
        prev = prev->get_next();
    }
    prev->set_next(node->get_next());
    node->set_next(nullptr);
    return retIt;
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::node_hash(son_container const& sons
) const -> std::size_t
{
    std::size_t result = 0;
    for (int32 k = 0; k < domain_; ++k)
    {
        utils::add_hash(result, sons[as_uindex(k)]);
    }
    return result;
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::node_equals(
    node_t* const node,
    son_container const& sons
) const -> bool
{
    for (int32 k = 0; k < domain_; ++k)
    {
        if (node->get_son(k) != sons[as_uindex(k)])
        {
            return false;
        }
    }
    return true;
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::callocate_buckets(int64 const count
) -> node_t**
{
    return static_cast<node_t**>(
        std::calloc(static_cast<std::size_t>(count), sizeof(node_t*))
    );
}

template<class Data, class Degree>
auto unique_table<Data, Degree>::mallocate_buckets(int64 const count
) -> node_t**
{
    return static_cast<node_t**>(
        std::malloc(static_cast<std::size_t>(count) * sizeof(node_t*))
    );
}

// apply_cache definitions:

template<class Data, class Degree>
apply_cache<Data, Degree>::apply_cache(int64 const capacity) :
    size_(0),
    capacity_(table_base::get_gte_capacity(capacity)),
    entries_(callocate_entries(capacity_))
{
}

template<class Data, class Degree>
apply_cache<Data, Degree>::apply_cache(apply_cache&& other) noexcept :
    size_(utils::exchange(other.size_, 0)),
    capacity_(other.capacity_),
    entries_(utils::exchange(other.entries_, nullptr))
{
}

template<class Data, class Degree>
apply_cache<Data, Degree>::~apply_cache()
{
    std::free(entries_);
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::find(
    int32 const opId,
    node_t* const lhs,
    node_t* const rhs
) -> node_t*
{
    std::size_t const hash  = utils::pack_hash(opId, lhs, rhs);
    std::size_t const index = hash % static_cast<std::size_t>(capacity_);
    cache_entry& entry      = entries_[index];
    bool const matches
        = entry.opId_ == opId && entry.lhs_ == lhs && entry.rhs_ == rhs;
    return matches ? entry.result_ : nullptr;
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::put(
    int32 const opId,
    node_t* const result,
    node_t* const lhs,
    node_t* const rhs
) -> void
{
    std::size_t const hash  = utils::pack_hash(opId, lhs, rhs);
    std::size_t const index = hash % static_cast<std::size_t>(capacity_);
    cache_entry& entry      = entries_[index];
    if (not entry.result_)
    {
        ++size_;
    }
    entry.opId_   = opId;
    entry.lhs_    = lhs;
    entry.rhs_    = rhs;
    entry.result_ = result;
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::grow_capacity(int64 const aproxCapacity) -> void
{
    int64 const newCapacity = table_base::get_gte_capacity(aproxCapacity);
    if (newCapacity > capacity_)
    {
        this->rehash(newCapacity);
    }
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::remove_unused() -> void
{
    for (int64 i = 0; i < capacity_; ++i)
    {
        cache_entry& entry = entries_[i];
        if (entry.result_)
        {
            bool const isUsed = entry.lhs_->is_used() && entry.rhs_->is_used()
                             && entry.result_->is_used();
            if (not isUsed)
            {
                entry = cache_entry {};
                --size_;
            }
        }
    }
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::clear() -> void
{
    size_ = 0;
    std::memset(
        entries_,
        0,
        static_cast<std::size_t>(capacity_) * sizeof(cache_entry)
    );
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::get_load_factor() const -> double
{
    return static_cast<double>(size_) / static_cast<double>(capacity_);
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::rehash(int64 const newCapacity) -> void
{
#ifdef LIBTEDDY_VERBOSE
    debug::out(
        "apply_cache::rehash\tload is ",
        this->get_load_factor(),
        ", capacity is ",
        capacity_,
        " should be ",
        newCapacity
    );
#endif

    cache_entry* const oldEntries = entries_;
    int64 const oldCapacity       = capacity_;
    entries_                      = callocate_entries(newCapacity);
    capacity_                     = newCapacity;
    size_                         = 0;
    for (int64 i = 0; i < oldCapacity; ++i)
    {
        cache_entry const& entry = oldEntries[i];
        if (entry.result_)
        {
            this->put(entry.opId_, entry.result_, entry.lhs_, entry.rhs_);
        }
    }

#ifdef LIBTEDDY_VERBOSE
    debug::out(" new load is ", this->get_load_factor(), "\n");
#endif
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::callocate_entries(int64 const count
) -> cache_entry*
{
    return static_cast<cache_entry*>(
        std::calloc(static_cast<std::size_t>(count), sizeof(cache_entry))
    );
}

} // namespace teddy

#endif