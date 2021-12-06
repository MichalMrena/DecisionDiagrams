#ifndef MIX_DD_HASH_TABLES_HPP
#define MIX_DD_HASH_TABLES_HPP

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
        unique_table ();
        unique_table (unique_table&&);

    public:
        template<class Eq>
        auto find (sons_t const&, hash_t, Eq&&) const -> node_t*;

        template<class Hash>
        auto merge (unique_table&, Hash&&) -> void;

        auto insert (node_t*, hash_t) -> node_t*;
        auto erase  (iterator)        -> iterator;
        auto erase  (node_t*, hash_t) -> iterator;
        auto size   () const          -> std::size_t;
        auto clear  ()                -> void;
        auto begin  ()                -> iterator;
        auto end    ()                -> iterator;
        auto begin  () const          -> const_iterator;
        auto end    () const          -> const_iterator;

        template<class Hash>
        auto adjust_capacity (Hash&&) -> void;

    private:
        template<class Hash>
        auto rehash (std::size_t, Hash&&) -> void;

        auto insert_impl (node_t*, hash_t) -> node_t*;

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
        struct entry
        {
            node_t* lhs    {nullptr};
            node_t* rhs    {nullptr};
            node_t* result {nullptr};
            auto matches (node_t*, node_t*) const -> bool;
            auto reset () -> void;
        };

    public:
        using iterator = typename std::vector<entry>::iterator;

    public:
        apply_cache ();
        apply_cache (apply_cache&&);

    public:
        auto find            (node_t*, node_t*) -> node_t*;
        auto put             (node_t*, node_t*, node_t*) -> void;
        auto adjust_capacity (std::size_t) -> void;
        auto rm_unused       () -> void;
        auto clear           () -> void;
            // auto size            () const -> std::size_t;
            // {
            //     return size_;
            // }
            // auto get_load ()
            // {
            //     return static_cast<double>(size_)
            //         / static_cast<double>(entries_.size());
            // }

    private:
        inline static constexpr auto LoadThreshold = 0.75;

    private:
        auto rehash      (std::size_t)      -> void;
        static auto hash (node_t*, node_t*) -> std::size_t;

    public:
        std::vector<entry> entries_;
        std::size_t        size_;
    };

// table_base definitions:

    inline auto table_base::gte_capacity
        (std::size_t const target) -> std::size_t
    {
        // TODO use ranges
        auto const p  = [target](auto const c){ return c > target; };
        auto const it = std::find_if( std::begin(Capacities)
                                    , std::end(Capacities)
                                    , p );
        return it == std::end(Capacities) ? Capacities.back() : *it;
    }

// apply_cache definitions:

    template<class Data, degree D>
    auto apply_cache<Data, D>::entry::matches
        (node_t* const l, node_t* const r) const -> bool
    {
        return l == lhs and r == rhs;
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::entry::reset
        () -> void
    {
        *this = {};
    }

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
    auto apply_cache<Data, D>::find
        ( node_t* const l
        , node_t* const r ) -> node_t*
    {
        auto const index = hash(l, r) % entries_.size();
        auto& e = entries_[index];
        // TODO metodu netreba
        return e.result and e.matches(l, r) ? e.result : nullptr;
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::put
        ( node_t* const l
        , node_t* const r
        , node_t* const res ) -> void
    {
        auto const index = hash(l, r) % entries_.size();
        auto& e = entries_[index];
        if (not e.result)
        {
            ++size_;
        }
        e.lhs    = l;
        e.rhs    = r;
        e.result = res;
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::adjust_capacity
        (std::size_t const aproxCapacity) -> void
    {
        // auto const currentLoad = static_cast<double>(size_)
        //                        / static_cast<double>(entries_.size());
        // if (currentLoad < LoadThreshold)
        // {
        //     return;
        // }

        this->rehash(table_base::gte_capacity(aproxCapacity));
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::rm_unused
        () -> void
    {
        if (size_ > 0)
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
                        e.reset();
                        --size_;
                    }
                }
            }
        }
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::clear
        () -> void
    {
        if (size_ > 0)
        {
            size_ = 0;
            for (auto& e : entries_)
            {
                e.result = nullptr;
            }
        }
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::hash
        (node_t* const l, node_t* const r) -> std::size_t
    {
        auto seed = 0ul;
        auto const hash1 = std::hash<node_t*>()(l);
        auto const hash2 = std::hash<node_t*>()(r);
        seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }

    template<class Data, degree D>
    auto apply_cache<Data, D>::rehash
        (std::size_t const newCapacity) -> void
    {
        if (entries_.size() == newCapacity)
        {
            return;
        }

        auto const oldEntries = std::vector<entry>(std::move(entries_));
        entries_              = std::vector<entry>(newCapacity);
        for (auto const& e : oldEntries)
        {
            auto const index = hash(e.lhs, e.rhs) % entries_.size();
            entries_[index]  = e;
        }
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
    }

    template<class Data, degree D>
    template<class Eq>
    auto unique_table<Data, D>::find
        (sons_t const& ss, hash_t const h, Eq&& eq) const -> node_t*
    {
        auto const index = h % buckets_.size();
        auto current     = buckets_[index];
        while (current)
        {
            if (eq(current, ss))
            {
                return current;
            }
            current = current->get_next();
        }
        return nullptr;
    }

    template<class Data, degree D>
    template<class Hash>
    auto unique_table<Data, D>::merge
        (unique_table& rhs, Hash&& hash) -> void
    {
        size_ += rhs.size();
        this->adjust_capacity();

        auto const end = std::end(rhs);
        auto it = std::begin(rhs);
        while (it != end)
        {
            auto const n = *it;
            ++it;

            n->set_next(nullptr);
            this->insert_impl(n, hash(n));
        }
        rhs.clear();
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::insert
        (node_t* const n, hash_t const h) -> node_t*
    {
        auto const ret = this->insert_impl(n, h);
        ++size_;
        return ret;
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
        (node_t* const n, hash_t const h) -> iterator
    {
        auto const index = static_cast<std::ptrdiff_t>(h % buckets_.size());
        auto       it    = iterator( std::begin(buckets_) + index
                                   , std::end(buckets_) );
        while (*it != n)
        {
            ++it;
        }
        return this->erase(it);
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
    template<class Hash>
    auto unique_table<Data, D>::adjust_capacity
        (Hash&& h) -> void
    {
        auto const aproxCapacity = 4 * (size_ / 3);
        auto const newCapacity   = table_base::gte_capacity(aproxCapacity);
        this->rehash(newCapacity, h);
    }

    template<class Data, degree D>
    template<class Hash>
    auto unique_table<Data, D>::rehash
        (std::size_t const newCapacity, Hash&& hash) -> void
    {
        if (buckets_.size() == newCapacity)
        {
            return;
        }

        auto const oldBuckets = std::vector<node_t*>(std::move(buckets_));
        buckets_ = std::vector<node_t*>(newCapacity, nullptr);
        for (auto bucket : oldBuckets)
        {
            while (bucket)
            {
                auto const next = bucket->get_next();
                auto const h = hash(bucket->get_index(), bucket->get_sons());
                bucket->set_next(nullptr);
                this->insert_impl(bucket, h);
                bucket = next;
            }
        };
    }

    template<class Data, degree D>
    auto unique_table<Data, D>::insert_impl
        (node_t* const n, hash_t const h) -> node_t*
    {
        auto const index  = h % buckets_.size();
        auto const bucket = buckets_[index];
        if (bucket)
        {
            n->set_next(bucket);
        }
        buckets_[index] = n;
        return n;
    }
}

#endif