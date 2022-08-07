#ifndef LIBTEDDY_DETAILS_HASH_TABLES_HPP
#define LIBTEDDY_DETAILS_HASH_TABLES_HPP

#include <libteddy/details/operators.hpp>
#include <algorithm>
#include <array>
#include <vector>
#include <utility>

namespace teddy
{
    /**
     *  @brief Base class for hash tables.
     */
    class table_base
    {
    private:
        inline static constexpr auto Capacities = std::array<std::size_t, 24>
        {
            307u,         617u,         1'237u,         2'477u,
            4'957u,       9'923u,       19'853u,        39'709u,
            79'423u,      158'849u,     317'701u,       635'413u,
            1'270'849u,   2'541'701u,   5'083'423u,     10'166'857u,
            20'333'759u,  40'667'527u,  81'335'063u,    162'670'129u,
            325'340'273u, 650'680'571u, 1'301'361'143u, 2'602'722'289u
        };

    public:
        static auto gte_capacity (std::size_t) -> std::size_t;
    };

    /**
     *  @brief Iterator for the unique table.
     */
    template<class BucketIt, class Data, degree D>
    class unique_table_it
    {
    public:
        using node_t            = node<Data, D>;
        using difference_type   = std::ptrdiff_t;
        using value_type        = node_t*;
        using pointer           = node_t*;
        using reference         = node_t*;
        using iterator_category = std::forward_iterator_tag;

    public:
        unique_table_it (BucketIt, BucketIt);

    public:
        auto operator++ ()       -> unique_table_it&;
        auto operator++ (int)    -> unique_table_it;
        auto operator*  () const -> reference;
        auto operator-> () const -> pointer;
        auto operator== (unique_table_it const&) const -> bool;
        auto operator!= (unique_table_it const&) const -> bool;
        auto get_bucket () const -> BucketIt;

    private:
        auto find_first ()       -> node_t*;
        auto move_next  ()       -> node_t*;

    private:
        BucketIt bucketIt_;
        BucketIt lastBucketIt_;
        node_t*  node_;
    };

    /**
     *  @brief Table of unique nodes.
     */
    template<class Data, degree D>
    class unique_table
    {
    public:
        using node_t = node<Data, D>;
        using sons_t = typename node_t::sons_t;
        using hash_t = std::size_t;
        using bucket_iterator  = typename std::vector<node_t*>::iterator;
        using cbucket_iterator = typename std::vector<node_t*>::const_iterator;
        using iterator         = unique_table_it<bucket_iterator,  Data, D>;
        using const_iterator   = unique_table_it<cbucket_iterator, Data, D>;

    public:
        /**
         *  \brief Initializes empty table.
         */
        unique_table ();

        /**
         *  \brief Move constructs tbale from \p other .
         */
        unique_table (unique_table&& other);

    public:
        /**
         *  \brief Tries to find internal node.
         *  \param sons Sons of the wanted node.
         *  \param domain Number of sons.
         *  \return Pointer to the found node, nullptr if not found.
         *          Hash of the node that can be used in insertion.
         */
        auto find ( sons_t const& sons
                  , std::size_t   domain ) const -> std::pair<node_t*, hash_t>;

        /**
         *  \brief Adds all nodes from \p other into this table.
         *  Adjusts capacity if necessary.
         *  \param other Table to merge into this one.
         *  \param domain Number of sons in this and \p other .
         */
        auto merge (unique_table other, std::size_t domain) -> void;

        /**
         *  \brief Inserts \p node using pre-computed \p hash .
         *  \param node Node to be inserted.
         *  \param hash Hash value of \p node .
         */
        auto insert (node_t* node, hash_t hash) -> void;

        /**
         *  \brief Erases node pointed to by \p it .
         *  \param it Iterator to the node to be deleted.
         *  \return Iterator to the next node.
         */
        auto erase (iterator it) -> iterator;

        /**
         *  \brief Erases \p node .
         *  \param node Node to be erased.
         *  \param domain Number of sons.
         *  \return Iterator to the next node.
         */
        auto erase (node_t* node, std::size_t domain) -> iterator;

        /**
         *  \brief Adjusts capacity of the table (number of buckets).
         *  \param domain Number of sons.
         */
        auto adjust_capacity (std::size_t domain) -> void;

        /**
         *  \return Number of nodes in the table.
         */
        auto size () const -> std::size_t;

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
        auto rehash (std::size_t newCapacity, std::size_t domain) -> void;

        /**
         *  \return Current capacity.
         */
        auto capacity () const -> std::size_t;

        /**
         *  \return Current load factor.
         */
        auto load_factor () const -> double;

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
        static auto node_hash ( sons_t const& sons
                              , std::size_t   domain ) -> hash_t;

        /**
         *  \brief Compares two nodes for equality
         *  (whether they have the same sons).
         *  \param node First node.
         *  \param sons Sons of the second node.
         *  \param domain Number of sons.
         *  \return True if the nodes are equal, false otherwise.
         */
        static auto node_equals ( node_t*       node
                                , sons_t const& sons
                                , std::size_t   domain ) -> bool;

    private:
        std::vector<node_t*> buckets_;
        std::size_t          size_;
    };

    /**
     *  @brief Cache for the apply opertaion.
     */
    template<class Data, degree D>
    class apply_cache
    {
    public:
        using node_t = node<Data, D>;

    public:
        struct entry_t
        {
            uint_t  oid    {};
            node_t* lhs    {nullptr};
            node_t* rhs    {nullptr};
            node_t* result {nullptr};
        };

    public:
        apply_cache ();
        apply_cache (apply_cache&&);

    public:
        // TODO bez zablon, posielat rovno ID
        template<bin_op O>
        auto find (node_t*, node_t*) -> node_t*;

        template<bin_op O>
        auto put (node_t*, node_t*, node_t*) -> void;

        auto adjust_capacity (std::size_t) -> void;
        auto rm_unused       ()            -> void;
        auto clear           ()            -> void;

    private:
        inline static constexpr auto LoadThreshold = 0.75;

    private:
        auto capacity    () const -> std::size_t;
        auto load_factor () const -> double;
        auto put_impl (uint_t, node_t*, node_t*, node_t*) -> void;
        auto rehash (std::size_t) -> void;
        static auto hash (uint_t, node_t*, node_t*) -> std::size_t;

    public:
        std::vector<entry_t> entries_;
        std::size_t          size_;
    };

// table_base definitions:

    inline auto table_base::gte_capacity
        (std::size_t const target) -> std::size_t
    {
        auto const p  = [target](auto const c){ return c > target; };
        auto const it = std::find_if( std::begin(Capacities)
                                    , std::end(Capacities)
                                    , p );
        return it == std::end(Capacities) ? Capacities.back() : *it;
    }

// apply_cache definitions:

    template<class Data, degree D>
    apply_cache<Data, D>::apply_cache
        () :
        entries_ (table_base::gte_capacity(0)),
        size_    (0)
    {
    }

    template<class Data, degree D>
    apply_cache<Data, D>::apply_cache
        (apply_cache&& other) :
        entries_ (std::move(other.entries_)),
        size_    (std::exchange(other.size_, 0))
    {
    }

    template<class Data, degree D>
    template<bin_op O>
    auto apply_cache<Data, D>::find
        ( node_t* const l
        , node_t* const r ) -> node_t*
    {
        auto const oid     = op_id(O());
        auto const index   = hash(oid, l, r) % entries_.size();
        auto&      entry   = entries_[index];
        auto const matches = entry.oid == oid
                         and entry.lhs == l
                         and entry.rhs == r;
        return matches ? entry.result : nullptr;
    }

    template<class Data, degree D>
    template<bin_op O>
    auto apply_cache<Data, D>::put
        ( node_t* const l
        , node_t* const r
        , node_t* const res ) -> void
    {
        auto const oid = op_id(O());
        this->put_impl(oid, l, r, res);
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::adjust_capacity
        (std::size_t const aproxCapacity) -> void
    {
        this->rehash(table_base::gte_capacity(aproxCapacity));
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::rm_unused
        () -> void
    {
        for (auto& e : entries_)
        {
            if (e.result)
            {
                auto const used = e.lhs->is_used()
                              and e.rhs->is_used()
                              and e.result->is_used();
                if (not used)
                {
                    e = entry_t {};
                    --size_;
                }
            }
        }
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::clear
        () -> void
    {
        size_ = 0;
        for (auto& e : entries_)
        {
            e = entry_t {};
        }
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::capacity
        () const -> std::size_t
    {
        return entries_.size();
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::load_factor
        () const -> double
    {
        return static_cast<double>(size_)
             / static_cast<double>(this->capacity());
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::hash
        (uint_t const oid, node_t* const l, node_t* const r) -> std::size_t
    {
        auto const hash1 = std::hash<uint_t>()(oid);
        auto const hash2 = std::hash<node_t*>()(l);
        auto const hash3 = std::hash<node_t*>()(r);
        auto result = 0ul;
        result ^= hash1 + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= hash2 + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= hash3 + 0x9e3779b9 + (result << 6) + (result >> 2);
        return result;
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::put_impl
        ( uint_t const  oid
        , node_t* const l
        , node_t* const r
        , node_t* const res ) -> void
    {
        auto const index = hash(oid, l, r) % this->capacity();
        auto&      entry = entries_[index];
        if (not entry.result)
        {
            ++size_;
        }
        entry.oid    = oid;
        entry.lhs    = l;
        entry.rhs    = r;
        entry.result = res;
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::rehash
        (std::size_t const newCapacity) -> void
    {
        if (this->capacity() == newCapacity)
        {
            return;
        }

        debug::out( "apply_cache: Load factor is ", this->load_factor()
                  , ". Capacity is ", this->capacity()
                  , " should be ", newCapacity, "." );

        auto const oldEntries = std::vector<entry_t>(std::move(entries_));
        entries_              = std::vector<entry_t>(newCapacity);
        size_                 = 0;
        for (auto const& e : oldEntries)
        {
            if (e.result)
            {
                this->put_impl(e.oid, e.lhs, e.rhs, e.result);
            }
        }

        debug::out(" New load factor is ", this->load_factor(), ".\n");
    }

// unique_table_it definitions:

    template<class BucketIt, class Data, degree D>
    unique_table_it<BucketIt, Data, D>::unique_table_it
        (BucketIt const first, BucketIt const last) :
        bucketIt_     (first),
        lastBucketIt_ (last),
        node_         (this->move_next())
    {
    }

    template<class BucketIt, class Data, degree D>
    auto unique_table_it<BucketIt, Data, D>::operator++
        () -> unique_table_it&
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
    auto unique_table_it<BucketIt, Data, D>::operator++
        (int) -> unique_table_it
    {
        auto const tmp = *this;
        ++(*this);
        return tmp;
    }

    template<class BucketIt, class Data, degree D>
    auto unique_table_it<BucketIt, Data, D>::operator*
        () const -> reference
    {
        return node_;
    }

    template<class BucketIt, class Data, degree D>
    auto unique_table_it<BucketIt, Data, D>::operator->
        () const -> reference
    {
        return node_;
    }

    template<class BucketIt, class Data, degree D>
    auto unique_table_it<BucketIt, Data, D>::operator==
        (unique_table_it const& rhs) const -> bool
    {
        return bucketIt_     == rhs.bucketIt_
           and lastBucketIt_ == rhs.lastBucketIt_
           and node_         == rhs.node_;
    }

    template<class BucketIt, class Data, degree D>
    auto unique_table_it<BucketIt, Data, D>::operator!=
        (unique_table_it const& rhs) const -> bool
    {
        return !(*this == rhs);
    }

    template<class BucketIt, class Data, degree D>
    auto unique_table_it<BucketIt, Data, D>::get_bucket
        () const -> BucketIt
    {
        return bucketIt_;
    }

    template<class BucketIt, class Data, degree D>
    auto unique_table_it<BucketIt, Data, D>::move_next
        () -> node_t*
    {
        while (bucketIt_ != lastBucketIt_ and not *bucketIt_)
        {
            ++bucketIt_;
        }
        return bucketIt_ != lastBucketIt_ ? *bucketIt_ : nullptr;
    }

// unique_table definitions:

    template<class Data, degree D>
    unique_table<Data, D>::unique_table
        () :
        buckets_ (table_base::gte_capacity(0), nullptr),
        size_    (0)
    {
    }

    template<class Data, degree D>
    unique_table<Data, D>::unique_table
        (unique_table&& other) :
        buckets_ (std::move(other.buckets_)),
        size_    (std::exchange(other.size_, 0))
    {
        other.buckets_.resize(table_base::gte_capacity(0), nullptr);
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::find
        ( sons_t const&     sons
        , std::size_t const domain ) const -> std::pair<node_t*, hash_t>
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
    auto unique_table<Data, D>::merge
        (unique_table other, std::size_t const domain) -> void
    {
        size_ += other.size();
        this->adjust_capacity(domain);

        auto const end = std::end(other);
        auto it = std::begin(other);
        while (it != end)
        {
            auto const node = *it;
            ++it;

            node->set_next(nullptr);
            this->insert_impl(node, node_hash(node->get_sons(), domain));
        }
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::insert
        (node_t* const node, hash_t const hash) -> void
    {
        this->insert_impl(node, hash);
        ++size_;
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::erase
        (iterator const it) -> iterator
    {
        auto       nextIt   = ++iterator(it);
        auto const bucketIt = it.get_bucket();
        auto const n        = *it;

        if (*bucketIt == n)
        {
            *bucketIt = n->get_next();
        }
        else
        {
            auto prev = *bucketIt;
            while (prev->get_next() != n)
            {
                prev = prev->get_next();
            }
            prev->set_next(n->get_next());
        }

        --size_;
        n->set_next(nullptr);
        return nextIt;
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::erase
        (node_t* const node, std::size_t const domain) -> iterator
    {
        auto const hash  = node_hash(node->get_sons(), domain);
        auto const index = static_cast<std::ptrdiff_t>(hash % buckets_.size());
        auto it = iterator(std::begin(buckets_) + index, std::end(buckets_));
        while (*it != node)
        {
            ++it;
        }
        return this->erase(it);
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::adjust_capacity
        (std::size_t const domain) -> void
    {
        auto const aproxCapacity = 4 * (size_ / 3);
        auto const newCapacity   = table_base::gte_capacity(aproxCapacity);
        this->rehash(newCapacity, domain);
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::size
        () const -> std::size_t
    {
        return size_;
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::clear
        () -> void
    {
        size_ = 0;
        std::fill(std::begin(buckets_), std::end(buckets_), nullptr);
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::begin
        () -> iterator
    {
        return iterator(std::begin(buckets_), std::end(buckets_));
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::end
        () -> iterator
    {
        return iterator(std::end(buckets_), std::end(buckets_));
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::begin
        () const -> const_iterator
    {
        return const_iterator(std::begin(buckets_), std::end(buckets_));
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::end
        () const -> const_iterator
    {
        return const_iterator(std::end(buckets_), std::end(buckets_));
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::rehash
        (std::size_t const newCapacity, std::size_t const domain) -> void
    {
        if (buckets_.size() >= newCapacity)
        {
            return;
        }

        debug::out("  unique_table: Load factor is "
                  , this->load_factor(), ". Capacity is ", this->capacity()
                  , " should be ", newCapacity );

        auto const oldBuckets = std::vector<node_t*>(std::move(buckets_));
        buckets_ = std::vector<node_t*>(newCapacity, nullptr);
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

        debug::out(". New load factor is ", this->load_factor(), ".\n");
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::capacity
        () const -> std::size_t
    {
        return buckets_.size();
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::load_factor
        () const -> double
    {
        return static_cast<double>(size_)
             / static_cast<double>(this->capacity());
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::insert_impl
        (node_t* const node, hash_t const hash) -> node_t*
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
    auto unique_table<Data, D>::node_hash
        (sons_t const& sons, std::size_t const domain ) -> hash_t
    {
        auto result = hash_t(0);
        for (auto k = 0u; k < domain; ++k)
        {
            auto const hash = std::hash<node_t*>()(sons[k]);
            result ^= hash + 0x9e3779b9 + (result << 6) + (result >> 2);
        }
        return result;
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::node_equals
        ( node_t* const     node
        , sons_t const&     sons
        , std::size_t const domain) -> bool
    {
        for (auto k = 0u; k < domain; ++k)
        {
            if (node->get_son(k) != sons[k])
            {
                return false;
            }
        }
        return true;
    }
}

#endif