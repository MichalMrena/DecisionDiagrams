#ifndef MIX_DD_BIN_OP_CACHE_HPP
#define MIX_DD_BIN_OP_CACHE_HPP

#include "table_base.hpp"
#include <vector>

namespace teddy
{
    template<class VertexData, class ArcData, std::size_t P>
    class apply_cache : private table_base
    {
    public:
        using vertex_t = vertex<VertexData, ArcData, P>;

    public:
        struct entry
        {
            vertex_t* lhs    {nullptr};
            vertex_t* rhs    {nullptr};
            vertex_t* result {nullptr};
            auto matches (vertex_t* l, vertex_t* r) const -> bool;
        };

    public:
        using iterator = typename std::vector<entry>::iterator;

    public:
        apply_cache ();

    public:
        auto find            (vertex_t* l, vertex_t* r) -> iterator;
        auto put             (iterator it, vertex_t* l, vertex_t* r, vertex_t* res) -> void;
        auto adjust_capacity (std::size_t aproxCapacity) -> void;
        auto clear           () -> void;

    private:
        static inline auto constexpr LoadThreshold = 0.75;

    private:
        using base        = table_base;
        using capactiy_it = base::capacity_it;

    private:
        static auto hash       (vertex_t* l, vertex_t* r)        -> std::size_t;
        auto calculate_index   (vertex_t* l, vertex_t* r)  const -> std::size_t;
        auto rehash            (capactiy_it capacity)            -> void;
        auto find_gte_capacity (std::size_t aproxCapacity) const -> capactiy_it;

    private:
        std::vector<entry> entries_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::entry::matches
        (vertex_t* const l, vertex_t* const r) const -> bool
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
        (vertex_t* const l, vertex_t* const r) -> iterator
    {
        auto const index = this->calculate_index(l, r);
        return std::begin(entries_) + static_cast<std::ptrdiff_t>(index);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::put
        (iterator it, vertex_t* const l, vertex_t* const r, vertex_t* const res) -> void
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
        (vertex_t* const l, vertex_t* const r) -> std::size_t
    {
        auto seed = 0ul;
        auto const hash1 = std::hash<vertex_t*>()(l);
        auto const hash2 = std::hash<vertex_t*>()(r);
        seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::calculate_index
        (vertex_t* const l, vertex_t* const r) const -> std::size_t
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
}

#endif