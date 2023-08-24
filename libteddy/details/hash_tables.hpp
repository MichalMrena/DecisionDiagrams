#ifndef LIBTEDDY_DETAILS_HASH_TABLES_HPP
#define LIBTEDDY_DETAILS_HASH_TABLES_HPP

#include <libteddy/details/config.hpp>
#include <libteddy/details/debug.hpp>
#include <libteddy/details/node.hpp>
#include <libteddy/details/tools.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>
    #include <vector>
    #include <new>
#include "libteddy/details/types.hpp"

namespace teddy
{
/**
 *  \brief Base class for hash tables.
 */
class table_base
{
private:
    static constexpr int64 Capacities[] {
        307,         617,         1'237,         2'477,        4'957,
        9'923,       19'853,      39'709,        79'423,       158'849,
        317'701,     635'413,     1'270'849,     2'541'701,    5'083'423,
        10'166'857,  20'333'759,  40'667'527,    81'335'063,   162'670'129,
        325'340'273, 650'680'571, 1'301'361'143, 2'602'722'289};

public:
    static auto get_gte_capacity (int64 desiredCapacity) -> int64;
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
        int32 opId_ {};
        node_t* lhs_ {nullptr};
        node_t* rhs_ {nullptr};
        node_t* result_ {nullptr};
    };

public:
    apply_cache(int64 capacity);
    apply_cache(apply_cache&& other) noexcept;
    ~apply_cache()                      = default;

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
    static constexpr double LoadThreshold = 0.75;

private:
    [[nodiscard]] auto get_capacity () const -> int64;
    [[nodiscard]] auto get_load_factor () const -> double;
    auto rehash (int64 newCapacity) -> void;

private:
    std::vector<cache_entry> entries_;
    int64 size_;
};

/**
 *  \brief naive implementation of open addressing hash map
 */
template<class Data, class Degree>
class open_unique_table
{
public:
    using node_t = node<Data, Degree>;
    using son_container = typename node_t::son_container;

    // TODO use just node_t** for sons (std::span before it was cool)

public:
    struct result_of_find
    {
        node_t* node_;
        std::size_t hash_;
    };

public:
    open_unique_table(int64 capacity, int32 domain);

    open_unique_table(const open_unique_table& other);

    open_unique_table(open_unique_table&& other) noexcept;

    ~open_unique_table();

    auto operator= (open_unique_table const&)   = delete;
    auto operator= (open_unique_table&&)        = delete;

    /**
     *  \brief Tries to find internal node
     *  \param sons Sons of the wanted node
     *  \return Pointer to a memory place where a pointer to the node is located
     *          or where the nullptr is located if there is no such node
     */
    [[nodiscard]] auto find (son_container const& sons) const -> node_t**;

    /**
     *  \brief Inserts \p node using pre-computed \p hash
     *  \param node Node to be inserted
     *  \param position Position for the insertion
     */
    auto insert (node_t* node, node_t** position) -> void;

    /**
     *  \brief Adds all nodes from \p other into this table.
     *  Adjusts capacity if necessary.
     *  \param other Table to merge into this one
     */
    auto merge (open_unique_table other) -> void;

    /**
     *  \brief TODO
     */
    auto erase (node_t* node) -> void;

    /**
     *  \brief TODO
     */
    auto erase_ith (int64 i) -> void;

    /**
     *  \return Number of nodes in the table.
     */
    [[nodiscard]] auto get_size () const -> int64;

    /**
     *  \brief TODO
     */
    [[nodiscard]] auto get_capacity () const -> int64;

    /**
     *  \brief TODO
     */
    [[nodiscard]] auto get_ith (int64 i) const -> node_t*;

    /**
     *  \brief Clears the table.
     */
    auto clear () -> void;

    /**
     *  \brief TODO
     */
    auto rehash_after_erasures () -> void;

private:
    /**
     *  \brief TODO
     */
    auto probe_next (int64 i) const -> int64;

    /**
     *  \brief TODO treba hash?
     */
    auto insert_impl (node_t* node) -> void;

    /**
     *  \brief TODO
     */
    auto rehash(int64 newCapacity) -> void;

    /**
     *  \brief TODO
     */
    auto calculate_index (std::size_t hash) const -> int64;

    /**
     *  \brief TODO
     */
    auto calculate_index (node_t* node) const -> int64;

    /**
     *  \brief TODO
     */
    auto calculate_index (son_container const& sons) const -> int64;

    /**
     *  \brief TODO
     */
    auto node_hash (node_t* node) const -> std::size_t;

    /**
     *  \brief Computes hash value of a node with \p sons .
     *  \param sons Sons of the node.
     *  \return Hash value of the node.
     */
    auto node_hash (son_container const& sons) const -> std::size_t;

    /**
     *  \brief Compares two nodes for equality
     *  (whether they have the same sons).
     *  \param node First node.
     *  \param sons Sons of the second node.
     *  \return True if the nodes are equal, false otherwise.
     */
    auto node_equals (node_t* node, son_container const& sons) const -> bool;

private:
    static constexpr double LOAD_THRESHOLD = 0.60;

private:
    bool didRehash_;
    int32 domain_;
    int64 size_;
    int64 capacity_;
    node_t** data_;
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

// apply_cache definitions:

template<class Data, class Degree>
apply_cache<Data, Degree>::apply_cache(int64 const capacity) :
    entries_(as_usize(table_base::get_gte_capacity(capacity))),
    size_(0)
{
}

template<class Data, class Degree>
apply_cache<Data, Degree>::apply_cache(apply_cache&& other) noexcept :
    entries_(static_cast<std::vector<cache_entry>&&>(other.entries_)),
    size_(utils::exchange(other.size_, 0))
{
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::find(
    int32 const opId,
    node_t* const lhs,
    node_t* const rhs
) -> node_t*
{
    std::size_t const hash  = utils::pack_hash(opId, lhs, rhs);
    std::size_t const index = hash % size(entries_);
    cache_entry& entry      = entries_[index];
    bool const matches      = entry.opId_ == opId &&
                              entry.lhs_ == lhs &&
                              entry.rhs_ == rhs;
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
    std::size_t const index = hash % size(entries_);
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
    if (this->get_capacity() < aproxCapacity)
    {
        this->rehash(table_base::get_gte_capacity(aproxCapacity));
    }
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::remove_unused() -> void
{
    for (cache_entry& entry : entries_)
    {
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
    for (cache_entry& entry : entries_)
    {
        entry = cache_entry {};
    }
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::get_capacity() const -> int64
{
    return ssize(entries_);
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::get_load_factor() const -> double
{
    return static_cast<double>(size_)
         / static_cast<double>(this->get_capacity());
}

template<class Data, class Degree>
auto apply_cache<Data, Degree>::rehash(int64 const newCapacity) -> void
{
    #ifdef LIBTEDDY_VERBOSE
    debug::out(
        "apply_cache: Load factor is ",
        this->get_load_factor(),
        ". Capacity is ",
        this->get_capacity(),
        " should be ",
        newCapacity,
        "."
    );
    #endif

    std::vector<cache_entry> const oldEntries(
        static_cast<std::vector<cache_entry>&&>(entries_)
    );
    entries_ = std::vector<cache_entry>(as_usize(newCapacity));
    size_    = 0;
    for (cache_entry const& entry : oldEntries)
    {
        if (entry.result_)
        {
            this->put(entry.opId_, entry.result_, entry.lhs_, entry.rhs_);
        }
    }

    #ifdef LIBTEDDY_VERBOSE
    debug::out(" New load factor is ", this->get_load_factor(), ".\n");
    #endif
}

// open_unique_table definitions:

template<class Data, class Degree>
open_unique_table<Data, Degree>::open_unique_table(
    int64 const capacity,
    int32 const domain
) :
    didRehash_(false),
    domain_(domain),
    size_(0),
    capacity_(table_base::get_gte_capacity(capacity)),
    data_(
        static_cast<node_t**>(
            std::calloc(as_usize(capacity_), sizeof(node_t*))
        )
    )
{
}

template<class Data, class Degree>
open_unique_table<Data, Degree>::open_unique_table (
    open_unique_table const& other
) :
    didRehash_(other.didRehash_),
    domain_(other.domain_),
    size_(other.size_),
    capacity_(other.capacity_),
    data_(
        static_cast<node_t**>(
            std::malloc(as_usize(capacity_) * sizeof(node_t*))
        )
    )
{
    std::memcpy(data_, other.data_, as_usize(capacity_) * sizeof(node_t*));
}

template<class Data, class Degree>
open_unique_table<Data, Degree>::open_unique_table(
    open_unique_table&& other
) noexcept :
    didRehash_(other.didRehash_),
    domain_(other.domain_),
    size_(utils::exchange(other.size_, 0)),
    capacity_(other.capacity_),
    data_(utils::exchange(other.data_, nullptr))
{
}

template<class Data, class Degree>
open_unique_table<Data, Degree>::~open_unique_table()
{
    std::free(data_);
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::find(
    son_container const& sons
) const -> node_t**
{
    int64 index = this->calculate_index(sons);
    while (data_[index] && not node_equals(data_[index], sons))
    {
        index = this->probe_next(index);
    }
    return data_ + index;
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::insert(
    node_t* const node,
    node_t** const position
) -> void
{
    /**
     *  Insert is only called after a prior call to find. In most cases,
     *  the position found by find can be just used directly.
     *  Only in an unlikely case when rehashing took place in between
     *  the calls---due to the gc rehash---we need to find the position again
     *  since it was invalidated in the rehash operation.
     */

    if (not didRehash_)
    {
        *position = node;
    }
    else [[unlikely]]
    {
        didRehash_ = false;
        int64 index = this->calculate_index(node);
        while (data_[index])
        {
            index = this->probe_next(index);
        }
        data_[index] = node;
    }

    ++size_;

    auto const loadFactor = static_cast<double>(size_)
                          / static_cast<double>(capacity_);
    if (loadFactor > LOAD_THRESHOLD)
    {
        this->rehash(table_base::get_gte_capacity(capacity_ + 1));
    }
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::merge(open_unique_table other) -> void
{
    size_ += other.size_;

    // The new capacity should be slightly below LOAD_THRESHOLD so that the next
    // insertion won't immediately rehash. 100 is just a magic number.
    int64 const newCapacity = static_cast<int64>(
        static_cast<double>(size_) / LOAD_THRESHOLD + 100
    );

    if (newCapacity > capacity_)
    {
        this->rehash(newCapacity);
    }

    for (int64 i = 0; i < other.capacity_; ++i)
    {
        node_t* const node = other.data_[i];
        if (node)
        {
            this->insert_impl(node);
        }
    }
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::erase (node_t* const node) -> void
{
    this->erase_ith(this->calculate_index(node));
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::erase_ith(int64 i) -> void
{
    data_[i] = nullptr;
    --size_;
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::get_size () const -> int64
{
    return size_;
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::get_capacity () const -> int64
{
    return capacity_;
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::get_ith (int64 const i) const -> node_t*
{
    return data_[i];
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::clear () -> void
{
    size_ = 0;
    std::memset(data_, 0, as_usize(capacity_) * sizeof(node_t*));
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::rehash_after_erasures () -> void
{
    this->rehash(capacity_);
    didRehash_ = true;
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::probe_next (
    int64 const i
) const -> int64
{
    return static_cast<int64>(
        static_cast<std::size_t>(i + 1) % static_cast<std::size_t>(capacity_)
    );
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::insert_impl (node_t* const node) -> void
{
    int64 index = this->calculate_index(node);
    while (data_[index])
    {
        index = this->probe_next(index);
    }
    data_[index] = node;
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::rehash (int64 const newCapacity) -> void
{
    int64 const oldCapacity = capacity_;
    node_t** const oldData = data_;
    capacity_ = newCapacity;
    data_ = static_cast<node_t**>(
        std::calloc(as_usize(newCapacity), sizeof(node_t*))
    );
    for (int64 i = 0; i < oldCapacity; ++i)
    {
        node_t* const node = oldData[i];
        if (node)
        {
            this->insert_impl(node);
        }
    }
    std::free(oldData);

    #ifdef LIBTEDDY_VERBOSE
    debug::out(
        "unique_table::rehash\tfrom ",
        oldCapacity,
        " to ",
        newCapacity,
        "\n"
    );
    #endif
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::calculate_index (
    std::size_t const hash
) const -> int64
{
    return static_cast<int64>(hash % static_cast<std::size_t>(capacity_));
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::calculate_index (
    node_t* const node
) const -> int64
{
    std::size_t const hash = this->node_hash(node);
    std::size_t const index = hash % static_cast<std::size_t>(capacity_);
    return static_cast<int64>(index);
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::calculate_index (
    son_container const& sons
) const -> int64
{
    std::size_t const hash = this->node_hash(sons);
    std::size_t const index = hash % static_cast<std::size_t>(capacity_);
    return static_cast<int64>(index);
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::node_hash (
    node_t* const node
) const -> std::size_t
{
    return this->node_hash(node->get_sons());
}

template<class Data, class Degree>
auto open_unique_table<Data, Degree>::node_hash (
    son_container const& sons
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
auto open_unique_table<Data, Degree>::node_equals (
    node_t* const node,
    son_container const& sons
) const -> bool
{
    for (int32 k = 0; k < domain_; ++k)
    {
        if (node->get_son(k) != sons[k])
        {
            return false;
        }
    }
    return true;
}

} // namespace teddy

#endif