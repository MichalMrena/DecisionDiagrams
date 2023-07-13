#ifndef LIBTEDDY_DETAILS_HASH_TABLES_HPP
#define LIBTEDDY_DETAILS_HASH_TABLES_HPP

#include <libteddy/details/debug.hpp>
#include <libteddy/details/node.hpp>
#include <libteddy/details/operators.hpp>

#include <algorithm>
#include <array>
#include <tuple>
#include <vector>

namespace teddy
{
/**
 *  \brief Base class for hash tables.
 */
class table_base
{
private:
    inline static constexpr auto Capacities = std::array<int64, 24> {
        307,         617,         1'237,         2'477,        4'957,
        9'923,       19'853,      39'709,        79'423,       158'849,
        317'701,     635'413,     1'270'849,     2'541'701,    5'083'423,
        10'166'857,  20'333'759,  40'667'527,    81'335'063,   162'670'129,
        325'340'273, 650'680'571, 1'301'361'143, 2'602'722'289};

public:
    static auto get_gte_capacity (int64 capacity) -> int64;
};

/**
 *  \brief Iterator for the unique table.
 */
template<class BucketIt, class Data, degree D>
class unique_table_iterator
{
public:
    using node_t            = node<Data, D>;
    using difference_type   = std::ptrdiff_t;
    using value_type        = node_t*;
    using pointer           = node_t*;
    using reference         = node_t*;
    using iterator_category = std::forward_iterator_tag;

public:
    unique_table_iterator(BucketIt first, BucketIt last);

public:
    auto operator++ () -> unique_table_iterator&;
    auto operator++ (int) -> unique_table_iterator;
    auto operator* () const -> reference;
    auto operator->() const -> pointer;
    auto operator== (unique_table_iterator const& other) const -> bool;
    auto operator!= (unique_table_iterator const& other) const -> bool;
    auto get_bucket () const -> BucketIt;

private:
    auto find_first () -> node_t*;
    auto move_next () -> node_t*;

private:
    BucketIt bucketIt_;
    BucketIt lastBucketIt_;
    node_t* node_;
};

/**
 *  \brief Table of unique nodes.
 */
template<class Data, degree D>
class unique_table
{
public:
    using node_t           = node<Data, D>;
    using sons_t           = typename node_t::son_container;
    using hash_t           = std::size_t;
    using bucket_iterator  = typename std::vector<node_t*>::iterator;
    using cbucket_iterator = typename std::vector<node_t*>::const_iterator;
    using iterator         = unique_table_iterator<bucket_iterator, Data, D>;
    using const_iterator   = unique_table_iterator<cbucket_iterator, Data, D>;

public:
    /**
     *  \brief Initializes empty table
     */
    unique_table();

    /**
     *  \brief Move constructs tbale from \p other
     */
    unique_table(unique_table&& other) noexcept;

    ~unique_table()                      = default;
    unique_table(unique_table const&)    = delete;
    auto operator= (unique_table const&) = delete;
    auto operator= (unique_table&&)      = delete;

public:
    /**
     *  \brief Tries to find internal node
     *  \param sons Sons of the wanted node
     *  \param domain Number of sons
     *  \return Pointer to the found node, nullptr if not found.
     *          Hash of the node that can be used in insertion.
     */
    auto find (sons_t const& sons, int32 domain) const
        -> std::tuple<node_t*, hash_t>;

    /**
     *  \brief Adds all nodes from \p other into this table.
     *  Adjusts capacity if necessary.
     *  \param other Table to merge into this one
     *  \param domain Number of sons in this and \p other
     */
    auto merge (unique_table other, int32 domain) -> void;

    /**
     *  \brief Inserts \p node using pre-computed \p hash
     *  \param node Node to be inserted
     *  \param hash Hash value of \p node
     */
    auto insert (node_t* node, hash_t hash) -> void;

    /**
     *  \brief Erases node pointed to by \p it
     *  \param nodeIt Iterator to the node to be deleted
     *  \return Iterator to the next node
     */
    auto erase (iterator nodeIt) -> iterator;

    /**
     *  \brief Erases \p node .
     *  \param node Node to be erased.
     *  \param domain Number of sons.
     *  \return Iterator to the next node.
     */
    auto erase (node_t* node, int32 domain) -> iterator;

    /**
     *  \brief Adjusts capacity of the table (number of buckets).
     *  \param domain Number of sons.
     */
    auto adjust_capacity (int32 domain) -> void;

    /**
     *  \return Number of nodes in the table.
     */
    [[nodiscard]] auto size () const -> int64;

    /**
     *  \brief Clears the table.
     */
    auto clear () -> void;

    /**
     *  \return Begin iterator.
     */
    auto begin () -> iterator;

    /**
     *  \return End iterator.
     */
    auto end () -> iterator;

    /**
     *  \return Const begin iterator.
     */
    auto begin () const -> const_iterator;

    /**
     *  \return Const end iterator.
     */
    auto end () const -> const_iterator;

private:
    /**
     *  \brief Adjusts capacity of the table (number of buckets).
     *  Does nothing if actual capacity is >= \p newCapacity .
     *  \param newCapacity New capacity.
     *  \param domain Number of sons.
     */
    auto rehash (int64 newCapacity, int32 domain) -> void;

    /**
     *  \return Current capacity.
     */
    [[nodiscard]] auto get_capacity () const -> int64;

    /**
     *  \return Current load factor.
     */
    [[nodiscard]] auto get_load_factor () const -> double;

    /**
     *  \brief Inserts \p node using pre-computed \p hash .
     *  Does NOT increase size.
     *  \param node Node to be inserted.
     *  \param hash Hash value of \p node .
     */
    auto insert_impl (node_t* node, hash_t hash) -> node_t*;

    /**
     *  \brief Computes hash value of a node with \p sons .
     *  \param sons Sons of the node.
     *  \param domain Number of sons.
     *  \return Hash value of the node.
     */
    static auto node_hash (sons_t const& sons, int32 domain) -> hash_t;

    /**
     *  \brief Compares two nodes for equality
     *  (whether they have the same sons).
     *  \param node First node.
     *  \param sons Sons of the second node.
     *  \param domain Number of sons.
     *  \return True if the nodes are equal, false otherwise.
     */
    static auto node_equals (node_t* node, sons_t const& sons, int32 domain)
        -> bool;

private:
    std::vector<node_t*> buckets_;
    int64 size_;
};

/**
 *  \brief Cache for the apply opertaion.
 */
template<class Data, degree Degree>
class apply_cache
{
public:
    using node_t = node<Data, Degree>;
    using hash_t = std::size_t;

public:
    struct cache_entry
    {
        int32 opId_ {};
        node_t* lhs_ {nullptr};
        node_t* rhs_ {nullptr};
        node_t* result_ {nullptr};
    };

public:
    apply_cache();
    apply_cache(apply_cache&& other) noexcept;
    ~apply_cache()                      = default;

    apply_cache(apply_cache const&)     = delete;
    auto operator= (apply_cache const&) = delete;
    auto operator= (apply_cache&&)      = delete;

public:
    auto find (int32 opId, node_t* lhs, node_t* rhs) -> node_t*;
    auto put (int32 opId, node_t* result, node_t* lhs, node_t* rhs) -> void;
    auto adjust_capacity (int64 aproxCapacity) -> void;
    auto remove_unused () -> void;
    auto clear () -> void;

private:
    inline static constexpr auto LoadThreshold = 0.75;

private:
    [[nodiscard]] auto get_capacity () const -> int64;
    [[nodiscard]] auto get_load_factor () const -> double;
    auto rehash (int64 newCapacity) -> void;

private:
    std::vector<cache_entry> entries_;
    int64 size_;
};

// table_base definitions:

inline auto table_base::get_gte_capacity(int64 const capacity) -> int64
{
    auto const predicate = [capacity] (int64 const currentCapacity)
    {
        return currentCapacity > capacity;
    };
    auto const* const tableIt
        = std::find_if(begin(Capacities), end(Capacities), predicate);
    return tableIt == std::end(Capacities) ? Capacities.back() : *tableIt;
}

// apply_cache definitions:

template<class Data, degree D>
apply_cache<Data, D>::apply_cache() :
    entries_(as_usize(table_base::get_gte_capacity(0))),
    size_(0)
{
}

template<class Data, degree D>
apply_cache<Data, D>::apply_cache(apply_cache&& other) noexcept :
    entries_(std::move(other.entries_)),
    size_(std::exchange(other.size_, 0))
{
}

template<class Data, degree D>
auto apply_cache<Data, D>::find(
    int32 const opId,
    node_t* const lhs,
    node_t* const rhs
) -> node_t*
{
    auto const index
        = utils::tuple_hash()(std::make_tuple(opId, lhs, rhs)) % size(entries_);
    auto& entry = entries_[index];
    auto const matches
        = entry.opId_ == opId && entry.lhs_ == lhs && entry.rhs_ == rhs;
    return matches ? entry.result_ : nullptr;
}

template<class Data, degree Degree>
auto apply_cache<Data, Degree>::put(
    int32 const opId,
    node_t* const result,
    node_t* const lhs,
    node_t* const rhs
) -> void
{
    auto const tupleHash = utils::tuple_hash()(std::make_tuple(opId, lhs, rhs));
    auto const index     = static_cast<int64>(tupleHash) % this->get_capacity();
    auto& entry          = entries_[as_uindex(index)];
    if (not entry.result_)
    {
        ++size_;
    }
    entry.opId_   = opId;
    entry.lhs_    = lhs;
    entry.rhs_    = rhs;
    entry.result_ = result;
}

template<class Data, degree D>
auto apply_cache<Data, D>::adjust_capacity(int64 const aproxCapacity) -> void
{
    this->rehash(table_base::get_gte_capacity(aproxCapacity));
}

template<class Data, degree D>
auto apply_cache<Data, D>::remove_unused() -> void
{
    for (auto& entry : entries_)
    {
        if (entry.result_)
        {
            auto const used = entry.lhs_->is_used() && entry.rhs_->is_used()
                           && entry.result_->is_used();
            if (not used)
            {
                entry = cache_entry {};
                --size_;
            }
        }
    }
}

template<class Data, degree D>
auto apply_cache<Data, D>::clear() -> void
{
    size_ = 0;
    for (auto& entry : entries_)
    {
        entry = cache_entry {};
    }
}

template<class Data, degree D>
auto apply_cache<Data, D>::get_capacity() const -> int64
{
    return ssize(entries_);
}

template<class Data, degree D>
auto apply_cache<Data, D>::get_load_factor() const -> double
{
    return static_cast<double>(size_)
         / static_cast<double>(this->get_capacity());
}

template<class Data, degree D>
auto apply_cache<Data, D>::rehash(int64 const newCapacity) -> void
{
    if (this->get_capacity() == newCapacity)
    {
        return;
    }

    debug::out(
        "apply_cache: Load factor is ",
        this->get_load_factor(),
        ". Capacity is ",
        this->get_capacity(),
        " should be ",
        newCapacity,
        "."
    );

    auto const oldEntries = std::vector<cache_entry>(std::move(entries_));
    entries_              = std::vector<cache_entry>(as_usize(newCapacity));
    size_                 = 0;
    for (auto const& entry : oldEntries)
    {
        if (entry.result_)
        {
            this->put(entry.opId_, entry.result_, entry.lhs_, entry.rhs_);
        }
    }

    debug::out(" New load factor is ", this->get_load_factor(), ".\n");
}

// unique_table_iterator definitions:

template<class BucketIt, class Data, degree D>
unique_table_iterator<BucketIt, Data, D>::unique_table_iterator(
    BucketIt const first,
    BucketIt const last
) :
    bucketIt_(first),
    lastBucketIt_(last),
    node_(this->move_next())
{
}

template<class BucketIt, class Data, degree D>
auto unique_table_iterator<BucketIt, Data, D>::operator++ ()
    -> unique_table_iterator&
{
    node_ = node_->get_next();
    if (not node_)
    {
        ++bucketIt_;
        node_ = this->move_next();
    }
    return *this;
}

template<class BucketIt, class Data, degree D>
auto unique_table_iterator<BucketIt, Data, D>::operator++ (int)
    -> unique_table_iterator
{
    auto const tmp = *this;
    ++(*this);
    return tmp;
}

template<class BucketIt, class Data, degree D>
auto unique_table_iterator<BucketIt, Data, D>::operator* () const -> reference
{
    return node_;
}

template<class BucketIt, class Data, degree D>
auto unique_table_iterator<BucketIt, Data, D>::operator->() const -> reference
{
    return node_;
}

template<class BucketIt, class Data, degree D>
auto unique_table_iterator<BucketIt, Data, D>::operator== (
    unique_table_iterator const& other
) const -> bool
{
    return bucketIt_ == other.bucketIt_ && lastBucketIt_ == other.lastBucketIt_
        && node_ == other.node_;
}

template<class BucketIt, class Data, degree D>
auto unique_table_iterator<BucketIt, Data, D>::operator!= (
    unique_table_iterator const& other
) const -> bool
{
    return ! (*this == other);
}

template<class BucketIt, class Data, degree D>
auto unique_table_iterator<BucketIt, Data, D>::get_bucket() const -> BucketIt
{
    return bucketIt_;
}

template<class BucketIt, class Data, degree D>
auto unique_table_iterator<BucketIt, Data, D>::move_next() -> node_t*
{
    while (bucketIt_ != lastBucketIt_ && not *bucketIt_)
    {
        ++bucketIt_;
    }
    return bucketIt_ != lastBucketIt_ ? *bucketIt_ : nullptr;
}

// unique_table definitions:

template<class Data, degree D>
unique_table<Data, D>::unique_table() :
    buckets_(as_usize(table_base::get_gte_capacity(0)), nullptr),
    size_(0)
{
}

template<class Data, degree D>
unique_table<Data, D>::unique_table(unique_table&& other) noexcept :
    buckets_(std::move(other.buckets_)),
    size_(std::exchange(other.size_, 0))
{
    other.buckets_.resize(as_usize(table_base::get_gte_capacity(0)), nullptr);
}

template<class Data, degree D>
auto unique_table<Data, D>::find(sons_t const& sons, int32 const domain) const
    -> std::tuple<node_t*, hash_t>
{
    auto const hash  = node_hash(sons, domain);
    auto const index = hash % buckets_.size();
    auto current     = buckets_[index];
    while (current)
    {
        if (node_equals(current, sons, domain))
        {
            return {current, hash};
        }
        current = current->get_next();
    }
    return {nullptr, hash};
}

template<class Data, degree D>
auto unique_table<Data, D>::merge(unique_table other, int32 const domain)
    -> void
{
    size_ += other.size();
    this->adjust_capacity(domain);
    auto const endIt = other.end();
    auto otherIt     = other.begin();
    while (otherIt != endIt)
    {
        auto const node = *otherIt;
        ++otherIt;
        node->set_next(nullptr);
        this->insert_impl(node, node_hash(node->get_sons(), domain));
    }
}

template<class Data, degree D>
auto unique_table<Data, D>::insert(node_t* const node, hash_t const hash)
    -> void
{
    this->insert_impl(node, hash);
    ++size_;
}

template<class Data, degree D>
auto unique_table<Data, D>::erase(iterator const nodeIt) -> iterator
{
    auto nextIt         = ++iterator(nodeIt);
    auto const bucketIt = nodeIt.get_bucket();
    auto const node     = *nodeIt;

    if (*bucketIt == node)
    {
        *bucketIt = node->get_next();
    }
    else
    {
        auto prev = *bucketIt;
        while (prev->get_next() != node)
        {
            prev = prev->get_next();
        }
        prev->set_next(node->get_next());
    }

    --size_;
    node->set_next(nullptr);
    return nextIt;
}

template<class Data, degree D>
auto unique_table<Data, D>::erase(node_t* const node, int32 const domain)
    -> iterator
{
    auto const hash  = node_hash(node->get_sons(), domain);
    auto const index = static_cast<std::ptrdiff_t>(hash % buckets_.size());
    auto tableIt = iterator(std::begin(buckets_) + index, std::end(buckets_));
    while (*tableIt != node)
    {
        ++tableIt;
    }
    return this->erase(tableIt);
}

template<class Data, degree D>
auto unique_table<Data, D>::adjust_capacity(int32 const domain) -> void
{
    auto const aproxCapacity = 4 * (size_ / 3);
    auto const newCapacity   = table_base::get_gte_capacity(aproxCapacity);
    this->rehash(newCapacity, domain);
}

template<class Data, degree D>
auto unique_table<Data, D>::size() const -> int64
{
    return size_;
}

template<class Data, degree D>
auto unique_table<Data, D>::clear() -> void
{
    size_ = 0;
    std::fill(std::begin(buckets_), std::end(buckets_), nullptr);
}

template<class Data, degree D>
auto unique_table<Data, D>::begin() -> iterator
{
    return iterator(std::begin(buckets_), std::end(buckets_));
}

template<class Data, degree D>
auto unique_table<Data, D>::end() -> iterator
{
    return iterator(std::end(buckets_), std::end(buckets_));
}

template<class Data, degree D>
auto unique_table<Data, D>::begin() const -> const_iterator
{
    return const_iterator(std::begin(buckets_), std::end(buckets_));
}

template<class Data, degree D>
auto unique_table<Data, D>::end() const -> const_iterator
{
    return const_iterator(std::end(buckets_), std::end(buckets_));
}

template<class Data, degree D>
auto unique_table<Data, D>::rehash(int64 const newCapacity, int32 const domain)
    -> void
{
    if (ssize(buckets_) >= newCapacity)
    {
        return;
    }

    debug::out(
        "  unique_table: Load factor is ",
        this->get_load_factor(),
        ". Capacity is ",
        this->get_capacity(),
        " should be ",
        newCapacity
    );

    auto const oldBuckets = std::vector<node_t*>(std::move(buckets_));
    buckets_ = std::vector<node_t*>(as_usize(newCapacity), nullptr);
    for (auto bucket : oldBuckets)
    {
        while (bucket)
        {
            auto const next = bucket->get_next();
            auto const hash = node_hash(bucket->get_sons(), domain);
            bucket->set_next(nullptr);
            this->insert_impl(bucket, hash);
            bucket = next;
        }
    };

    debug::out(". New load factor is ", this->get_load_factor(), ".\n");
}

template<class Data, degree D>
auto unique_table<Data, D>::get_capacity() const -> int64
{
    return static_cast<int64>(buckets_.size());
}

template<class Data, degree D>
auto unique_table<Data, D>::get_load_factor() const -> double
{
    return static_cast<double>(size_)
         / static_cast<double>(this->get_capacity());
}

template<class Data, degree D>
auto unique_table<Data, D>::insert_impl(node_t* const node, hash_t const hash)
    -> node_t*
{
    auto const index  = hash % buckets_.size();
    auto const bucket = buckets_[index];
    if (bucket)
    {
        node->set_next(bucket);
    }
    buckets_[index] = node;
    return node;
}

template<class Data, degree D>
auto unique_table<Data, D>::node_hash(sons_t const& sons, int32 const domain)
    -> hash_t
{
    auto result = hash_t(0);
    for (auto k = 0; k < domain; ++k)
    {
        auto const hash = std::hash<node_t*>()(sons[as_uindex(k)]);
        result ^= hash + 0x9e3779b9 + (result << 6) + (result >> 2);
    }
    return result;
}

template<class Data, degree D>
auto unique_table<Data, D>::node_equals(
    node_t* const node,
    sons_t const& sons,
    int32 const domain
) -> bool
{
    for (auto k = 0; k < domain; ++k)
    {
        if (node->get_son(k) != sons[as_uindex(k)])
        {
            return false;
        }
    }
    return true;
}
} // namespace teddy

#endif