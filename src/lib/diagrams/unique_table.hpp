#ifndef MIX_DD_UNIQUE_TABLE_HPP
#define MIX_DD_UNIQUE_TABLE_HPP

#include "graph.hpp"
#include <vector>
#include <functional>

namespace teddy
{
    /**
     *  @brief Iterator for unique table.
     */
    template<class BucketIterator, class VertexData, class ArcData, std::size_t P>
    class unique_table_iterator
    {
    public:
        using vertex_t          = vertex<VertexData, ArcData, P>;
        using difference_type   = std::ptrdiff_t;
        using value_type        = vertex_t*;
        using pointer           = vertex_t*;
        using reference         = vertex_t*;
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
        auto find_first ()       -> vertex_t*;

    private:
        BucketIterator current_;
        BucketIterator last_;
        vertex_t*      vertex_;
    };

    /**
     *  @brief Unique table of vertices.
     */
    template<class VertexData, class ArcData, std::size_t P>
    class unique_table
    {
    public:
        using vertex_t         = vertex<VertexData, ArcData, P>;
        using vertex_a         = std::array<vertex_t*, P>;
        using bucket_iterator  = typename std::vector<vertex_t*>::iterator;
        using cbucket_iterator = typename std::vector<vertex_t*>::const_iterator;
        using iterator         = unique_table_iterator<bucket_iterator, VertexData, ArcData, P>;
        using const_iterator   = unique_table_iterator<cbucket_iterator, VertexData, ArcData, P>;

    public:
        unique_table ();
        unique_table (unique_table&& other);

    public:
        auto insert          (vertex_t* v)               -> vertex_t*;
        auto find            (vertex_a const& key) const -> vertex_t*;
        auto erase           (iterator it)               -> iterator;
        auto erase           (vertex_t* v)               -> iterator;
        auto size            () const                    -> std::size_t;
        auto adjust_capacity ()                          -> void;
        auto merge           (unique_table& rhs)         -> void;
        auto clear           ()                          -> void;
        auto begin           ()                          -> iterator;
        auto end             ()                          -> iterator;
        auto begin           () const                    -> const_iterator;
        auto end             () const                    -> const_iterator;

    private:
        template<class Key, class Getter>
        static auto hash      (Key key, Getter get_ith)          -> std::size_t;
        static auto vertex_eq (vertex_t* v, vertex_a const& key) -> bool;
        auto insert_impl      (vertex_t* v)                      -> vertex_t*;
        auto calculate_index  (vertex_t* v)         const        -> std::size_t;
        auto calculate_index  (vertex_a const& key) const        -> std::size_t;
        auto is_overloaded    ()                    const        -> bool;
        auto rehash           ()                                 -> void;

    private:
        static inline auto constexpr LoadThreshold = 0.75;
        static inline auto constexpr Capacities    = std::array<std::size_t, 25>
        {
            307,         617,         1'237,         2'477,         4'957,
            9'923,       19'853,      39'709,        79'423,        158'849,
            317'701,     635'413,     1'270'849,     2'541'701,     5'083'423,
            10'166'857,  20'333'759,  40'667'527,    81'335'063,    162'670'129,
            325'340'273, 650'680'571, 1'301'361'143, 2'602'722'289, 5'205'444'619
        };

    private:
        std::size_t            size_;
        std::size_t const*     capacity_;
        std::vector<vertex_t*> buckets_;
    };

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
        () -> vertex_t*
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
        size_     (0),
        capacity_ (Capacities.data()),
        buckets_  (*capacity_, nullptr)
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    unique_table<VertexData, ArcData, P>::unique_table
        (unique_table&& other) :
        size_     (std::exchange(other.size_, 0)),
        capacity_ (std::exchange(other.capacity_, Capacities.data())),
        buckets_  (std::exchange(other.buckets_, std::vector<vertex_t*>(Capacities.front(), nullptr)))
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::insert
        (vertex_t* const v) -> vertex_t*
    {
        auto const ret = this->insert_impl(v);
        ++size_;
        return ret;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::find
        (vertex_a const& key) const -> vertex_t*
    {
        auto const index = this->calculate_index(key);
        auto current     = buckets_[index];
        while (current)
        {
            if (unique_table::vertex_eq(current, key))
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
        auto const nextIt   = ++iterator(it);
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

        --size_;
        v->set_next(nullptr);
        return nextIt;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::erase
        (vertex_t* const v) -> iterator
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
        return size_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::adjust_capacity
        () -> void
    {
        if (size_)
        {
            while (static_cast<double>(size_) / static_cast<double>(*capacity_) > LoadThreshold)
            {
                ++capacity_;
            }

            if (*capacity_ != buckets_.size())
            {
                this->rehash();
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::merge
        (unique_table& rhs) -> void
    {
        size_ += rhs.size();
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
        size_ = 0;
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
    auto unique_table<VertexData, ArcData, P>::vertex_eq
        (vertex_t* const v, vertex_a const& key) -> bool
    {
        for (auto i = 0u; i < P; ++i)
        {
            if (v->get_son(i) != key[i])
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
        auto seed = 0ull;
        for (auto i = 0u; i < P; ++i)
        {
            auto const hash = std::hash<vertex_t*>()(get_ith(i, key));
            seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::insert_impl
        (vertex_t* const v) -> vertex_t*
    {
        auto const index  = this->calculate_index(v);
        auto const bucket = buckets_[index];
        if (bucket)
        {
            v->set_next(bucket);
        }
        buckets_[index] = v;
        return v;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::calculate_index
        (vertex_t* const v) const -> std::size_t
    {
        auto const h = unique_table::hash<vertex_t*>(v, [](auto const i, auto const v)
        {
            return v->get_son(i);
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
    auto unique_table<VertexData, ArcData, P>::is_overloaded
        () const -> bool
    {
        auto const currentLoad = static_cast<double>(size_) / static_cast<double>(*capacity_);
        return currentLoad > LoadThreshold;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto unique_table<VertexData, ArcData, P>::rehash
        () -> void
    {
        auto const oldBuckets = std::vector<vertex_t*>(std::move(buckets_));
        buckets_ = std::vector<vertex_t*>(*capacity_, nullptr);
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