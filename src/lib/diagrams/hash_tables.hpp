#ifndef MIX_DD_HASH_TABLES_HPP
#define MIX_DD_HASH_TABLES_HPP

#include <array>
#include <utility>

namespace teddy
{
    /**
     *  @brief Base class for all tables.
     */
    class table_base
    {
    protected:
        using capacity_it = std::array<std::size_t, 24>::const_iterator;

    protected:
        inline static constexpr auto Capacities = std::array<std::size_t, 24>
        {
            307u,         617u,         1'237u,       2'477u,      4'957u,
            9'923u,       19'853u,      39'709u,      79'423u,     158'849u,
            317'701u,     635'413u,     1'270'849u,   2'541'701u,  5'083'423u,
            10'166'857u,  20'333'759u,  40'667'527u,  81'335'063u, 162'670'129u,
            325'340'273u, 650'680'571u, 1'301'361'143u, 2'602'722'289u
        };

    protected:
        table_base () = default;
        table_base (table_base&& other) noexcept :
            size_     (std::exchange(other.size_, 0)),
            capacity_ (std::exchange(other.capacity_, std::begin(Capacities)))
        {
        }

    protected:
        std::size_t size_     {0};
        capacity_it capacity_ {std::cbegin(Capacities)};
    };

    /**
     *  @brief Iterator for unique table.
     */
    template<class BucketIterator, class VertexData, class ArcData, std::size_t P>
    class unique_table_iterator
    {
    public:
        using node_t          = vertex<VertexData, ArcData, P>;
        using difference_type   = std::ptrdiff_t;
        using value_type        = node_t*;
        using pointer           = node_t*;
        using reference         = node_t*;
        using iterator_category = std::forward_iterator_tag;

    public:
        unique_table_iterator (BucketIterator first, BucketIterator last);

    public:
        auto operator++ ()       -> unique_table_iterator&;
        auto operator++ (int)    -> unique_table_iterator;
        auto operator*  () const -> reference;
        auto operator-> () const -> pointer;
        auto operator== (unique_table_iterator const&) const -> bool;
        auto operator!= (unique_table_iterator const&) const -> bool;
        auto get_bucket () const -> BucketIterator;

    private:
        auto find_first ()       -> node_t*;

    private:
        BucketIterator current_;
        BucketIterator last_;
        node_t*      vertex_;
    };

    /**
     *  @brief Table of unique nodes.
     */
    template<class Data, degree D>
    class unique_table : private table_base
    {
    public:
        using node_t   = node<Data, D>;
        using sons_t   = typename node_t::sons_t;
        using hash_t   = std::size_t;


        using bucket_iterator  = typename std::vector<node_t*>::iterator;
        using cbucket_iterator = typename std::vector<node_t*>::const_iterator;
        using iterator         = unique_table_iterator<bucket_iterator, VertexData, ArcData, P>;
        using const_iterator   = unique_table_iterator<cbucket_iterator, VertexData, ArcData, P>;

    public:
        unique_table ();
        unique_table (unique_table&& other);

    public:
        auto insert (node_t*, hash_t) -> node_t*;

        template<class Eq>
        auto find (sons_t const&, hash_t, Eq&&) const -> node_t*;

        auto erase           (iterator it)               -> iterator;
        auto erase           (node_t* v)               -> iterator;
        auto size            () const                    -> std::size_t;
        auto adjust_capacity ()                          -> void;
        auto merge           (unique_table& rhs)         -> void;
        auto clear           ()                          -> void;
        auto begin           ()                          -> iterator;
        auto end             ()                          -> iterator;
        auto begin           () const                    -> const_iterator;
        auto end             () const                    -> const_iterator;

    private:
        using base        = table_base;
        using capacity_it = base::capacity_it;

    private:
        static auto node_eq (node_t*, domain_t, sons_t const&) -> bool;

        auto insert_impl (node_t*, hash_t) -> node_t*;

        auto calculate_index  (node_t* v)         const        -> std::size_t;
        auto calculate_index  (vertex_a const& key) const        -> std::size_t;
        auto rehash           ()                                 -> void;

    private:
        static inline auto constexpr LoadThreshold = 0.75;

    private:
        std::vector<node_t*> buckets_;
    };

    /**
     *  @brief Cache for apply opertaion.
     */
    template<class VertexData, class ArcData, std::size_t P>
    class apply_cache : private table_base
    {
    public:
        using node_t = vertex<VertexData, ArcData, P>;

    public:
        struct entry
        {
            node_t* lhs    {nullptr};
            node_t* rhs    {nullptr};
            node_t* result {nullptr};
            auto matches (node_t* l, node_t* r) const -> bool;
        };

    public:
        using iterator = typename std::vector<entry>::iterator;

    public:
        apply_cache ();

    public:
        auto find            (node_t* l, node_t* r) -> iterator;
        auto put             (iterator it, node_t* l, node_t* r, node_t* res) -> void;
        auto adjust_capacity (std::size_t aproxCapacity) -> void;
        auto clear           () -> void;

    private:
        static inline auto constexpr LoadThreshold = 0.75;

    private:
        using base        = table_base;
        using capactiy_it = base::capacity_it;

    private:
        static auto hash       (node_t* l, node_t* r)        -> std::size_t;
        auto calculate_index   (node_t* l, node_t* r)  const -> std::size_t;
        auto rehash            (capactiy_it capacity)            -> void;
        auto find_gte_capacity (std::size_t aproxCapacity) const -> capactiy_it;

    private:
        std::vector<entry> entries_;
    };

// apply_cache definitions:

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::entry::matches
        (node_t* const l, node_t* const r) const -> bool
    {
        return result && l == lhs && r == rhs;
    }

    template<class VertexData, class ArcData, std::size_t P>
    apply_cache<VertexData, ArcData, P>::apply_cache
        () :
        entries_  (*base::capacity_)
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::find
        (node_t* const l, node_t* const r) -> iterator
    {
        auto const index = this->calculate_index(l, r);
        return std::begin(entries_) + static_cast<std::ptrdiff_t>(index);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::put
        (iterator it, node_t* const l, node_t* const r, node_t* const res) -> void
    {
        if (!it->result)
        {
            ++base::size_;
        }
        it->lhs    = l;
        it->rhs    = r;
        it->result = res;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::adjust_capacity
        (std::size_t const aproxCapacity) -> void
    {
        if (!base::size_ || base::capacity_ >= std::end(Capacities))
        {
            return;
        }

        auto const targetCapacity = this->find_gte_capacity(aproxCapacity);
        if (base::capacity_ >= targetCapacity)
        {
            return;
        }

        auto const currentLoad = static_cast<double>(base::size_) / static_cast<double>(entries_.size());
        if (currentLoad < LoadThreshold)
        {
            return;
        }

        this->rehash(targetCapacity);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::clear
        () -> void
    {
        if (base::size_ > 0)
        {
            base::size_ = 0;
            for (auto& e : entries_)
            {
                e.result = nullptr;
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::hash
        (node_t* const l, node_t* const r) -> std::size_t
    {
        auto seed = 0ul;
        auto const hash1 = std::hash<node_t*>()(l);
        auto const hash2 = std::hash<node_t*>()(r);
        seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::calculate_index
        (node_t* const l, node_t* const r) const -> std::size_t
    {
        return hash(l, r) % entries_.size();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::rehash
        (capacity_it capacity) -> void
    {
        base::capacity_       = capacity;
        auto const oldEntries = std::vector<entry>(std::move(entries_));
        entries_              = std::vector<entry>(*base::capacity_);
        for (auto const& e : oldEntries)
        {
            auto const index = this->calculate_index(e.lhs, e.rhs);
            entries_[index]  = e;
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::find_gte_capacity
        (std::size_t const aproxCapacity) const -> capacity_it
    {
        auto c = base::capacity_;
        while (c < std::end(Capacities) && *c < aproxCapacity)
        {
            ++c;
        }
        return c;
    }

// unique_table_iterator definitions:

    template<class BucketIterator, class VertexData, class ArcData, std::size_t P>
    unique_table_iterator<BucketIterator, VertexData, ArcData, P>::unique_table_iterator
        (BucketIterator first, BucketIterator last) :
        current_ (first),
        last_    (last),
        vertex_  (this->find_first())
    {
    }

    template<class BucketIterator, class VertexData, class ArcData, std::size_t P>
    auto unique_table_iterator<BucketIterator, VertexData, ArcData, P>::operator++
        () -> unique_table_iterator&
    {
        vertex_ = vertex_->get_next();
        if (!vertex_)
        {
            do
            {
                ++current_;
            }
            while (current_ != last_ && !*current_);
            vertex_ = current_ != last_ ? *current_ : nullptr;
        }
        return *this;
    }

    template<class BucketIterator, class VertexData, class ArcData, std::size_t P>
    auto unique_table_iterator<BucketIterator, VertexData, ArcData, P>::operator++
        (int) -> unique_table_iterator
    {
        auto const tmp = *this;
        ++(*this);
        return tmp;
    }

    template<class BucketIterator, class VertexData, class ArcData, std::size_t P>
    auto unique_table_iterator<BucketIterator, VertexData, ArcData, P>::operator*
        () const -> reference
    {
        return vertex_;
    }

    template<class BucketIterator, class VertexData, class ArcData, std::size_t P>
    auto unique_table_iterator<BucketIterator, VertexData, ArcData, P>::operator->
        () const -> reference
    {
        return vertex_;
    }

    template<class BucketIterator, class VertexData, class ArcData, std::size_t P>
    auto unique_table_iterator<BucketIterator, VertexData, ArcData, P>::operator==
        (unique_table_iterator const& rhs) const -> bool
    {
        return current_ == rhs.current_
            && last_    == rhs.last_
            && vertex_  == rhs.vertex_;
    }

    template<class BucketIterator, class VertexData, class ArcData, std::size_t P>
    auto unique_table_iterator<BucketIterator, VertexData, ArcData, P>::operator!=
        (unique_table_iterator const& rhs) const -> bool
    {
        return !(*this == rhs);
    }

    template<class BucketIterator, class VertexData, class ArcData, std::size_t P>
    auto unique_table_iterator<BucketIterator, VertexData, ArcData, P>::get_bucket
        () const -> BucketIterator
    {
        return current_;
    }

    template<class BucketIterator, class VertexData, class ArcData, std::size_t P>
    auto unique_table_iterator<BucketIterator, VertexData, ArcData, P>::find_first
        () -> node_t*
    {
        while (current_ != last_ && !*current_)
        {
            ++current_;
        }
        return current_ != last_ ? *current_ : nullptr;
    }

// unique_table definitions:

    template<class VertexData, class ArcData, std::size_t P>
    unique_table<VertexData, ArcData, P>::unique_table
        () :
        base     (),
        buckets_ (*capacity_, nullptr)
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    unique_table<VertexData, ArcData, P>::unique_table
        (unique_table&& other) :
        base     (std::move(other)),
        buckets_ (std::exchange(other.buckets_, std::vector<node_t*>(*capacity_, nullptr)))
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::insert
        (node_t* const n, hash_t const h) -> node_t*
    {
        auto const ret = this->insert_impl(n, h);
        ++base::size_;
        return ret;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Eq>
    auto unique_table<VertexData, ArcData, P>::find
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

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::erase
        (iterator const it) -> iterator
    {
        auto       nextIt   = ++iterator(it);
        auto const bucketIt = it.get_bucket();
        auto const v        = *it;

        if (*bucketIt == v)
        {
            *bucketIt = v->get_next();
        }
        else
        {
            auto prev = *bucketIt;
            while (prev->get_next() != v)
            {
                prev = prev->get_next();
            }
            prev->set_next(v->get_next());
        }

        --base::size_;
        v->set_next(nullptr);
        return nextIt;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::erase
        (node_t* const v) -> iterator
    {
        auto const index = static_cast<std::ptrdiff_t>(this->calculate_index(v));
        auto       it    = iterator(std::begin(buckets_) + index, std::end(buckets_));
        while (*it != v)
        {
            ++it;
        }
        return this->erase(it);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::size
        () const -> std::size_t
    {
        return base::size_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::adjust_capacity
        () -> void
    {
        if (base::size_)
        {
            while (static_cast<double>(base::size_) / static_cast<double>(*base::capacity_) > LoadThreshold)
            {
                ++base::capacity_;
            }

            if (*base::capacity_ != buckets_.size())
            {
                this->rehash();
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::merge
        (unique_table& rhs) -> void
    {
        base::size_ += rhs.size();
        this->adjust_capacity();

        auto const end = std::end(rhs);
        auto it = std::begin(rhs);
        while (it != end)
        {
            auto const v = *it;
            ++it;

            v->set_next(nullptr);
            this->insert_impl(v);
        }
        rhs.clear();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::clear
        () -> void
    {
        base::size_ = 0;
        std::fill(std::begin(buckets_), std::end(buckets_), nullptr);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::begin
        () -> iterator
    {
        return iterator(std::begin(buckets_), std::end(buckets_));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::end
        () -> iterator
    {
        return iterator(std::end(buckets_), std::end(buckets_));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::begin
        () const -> const_iterator
    {
        return const_iterator(std::begin(buckets_), std::end(buckets_));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::end
        () const -> const_iterator
    {
        return const_iterator(std::end(buckets_), std::end(buckets_));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::node_eq
        (node_t* const n, domain_t const d, sons_t const& ss) -> bool
    {
        for (auto j = 0u; j < d; ++j)
        {
            if (n->get_son(j) != ss[j])
            {
                return false;
            }
        }
        return true;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Key, class Getter>
    auto unique_table<VertexData, ArcData, P>::hash
        (Key key, Getter get_ith) -> std::size_t
    {
        auto seed = std::size_t(0);
        for (auto i = 0u; i < P; ++i)
        {
            auto const hash = std::hash<node_t*>()(get_ith(i, key));
            seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::insert_impl
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

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::calculate_index
        (node_t* const v) const -> std::size_t
    {
        auto const h = hash<node_t*>(v, [](auto const i, auto const ve)
        {
            return ve->get_son(i);
        });
        return h % buckets_.size();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::calculate_index
        (vertex_a const& key) const -> std::size_t
    {
        auto const h = hash<vertex_a const&>(key, [](auto const i, auto const& k)
        {
            return k[i];
        });
        return h % buckets_.size();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::rehash
        () -> void
    {
        auto const oldBuckets = std::vector<node_t*>(std::move(buckets_));
        buckets_ = std::vector<node_t*>(*base::capacity_, nullptr);
        for (auto bucket : oldBuckets)
        {
            while (bucket)
            {
                auto const next = bucket->get_next();
                bucket->set_next(nullptr);
                this->insert_impl(bucket);
                bucket = next;
            }
        };
    }
}

#endif