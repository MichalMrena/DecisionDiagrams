#ifndef MIX_DD_BIN_OP_CACHE_HPP
#define MIX_DD_BIN_OP_CACHE_HPP

#include "graph.hpp"
#include <vector>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    class apply_cache
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
        static inline auto constexpr Capacities    = std::array<std::size_t, 25>
        {
            307,         617,         1'237,         2'477,         4'957,
            9'923,       19'853,      39'709,        79'423,        158'849,
            317'701,     635'413,     1'270'849,     2'541'701,     5'083'423,
            10'166'857,  20'333'759,  40'667'527,    81'335'063,    162'670'129,
            325'340'273, 650'680'571, 1'301'361'143, 2'602'722'289, 5'205'444'619
        };

    private:
        static auto hash       (vertex_t* l, vertex_t* r)               -> std::size_t;
        auto calculate_index   (vertex_t* l, vertex_t* r)         const -> std::size_t;
        auto possibly_rehash   (std::size_t        aproxCapacity) const -> bool;
        auto rehash            (std::size_t const* capacity)            -> void;
        auto find_gte_capacity (std::size_t        aproxCapacity) const -> std::size_t const*;

    private:
        std::size_t        size_;
        std::size_t const* capacity_;
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
        size_     (0),
        capacity_ (Capacities.data()),
        entries_  (*capacity_)
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
            ++size_;
        }
        it->lhs    = l;
        it->rhs    = r;
        it->result = res;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::adjust_capacity
        (std::size_t const aproxCapacity) -> void
    {
        if (!size_ || !(capacity_ < std::end(Capacities)))
        {
            return;
        }

        auto const targetCapacity = this->find_gte_capacity(aproxCapacity);
        if (capacity_ >= targetCapacity)
        {
            return;
        }

        auto const currentLoad = static_cast<double>(size_) / static_cast<double>(entries_.size());
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
        if (size_ > 0)
        {
            size_ = 0;
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
        auto seed = 0ull;
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
        (std::size_t const* capacity) -> void
    {
        capacity_             = capacity;
        auto const oldEntries = std::vector<entry>(std::move(entries_));
        entries_              = std::vector<entry>(*capacity_);
        for (auto const& e : oldEntries)
        {
            auto const index = this->calculate_index(e.lhs, e.rhs);
            entries_[index]  = e;
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto apply_cache<VertexData, ArcData, P>::find_gte_capacity
        (std::size_t const aproxCapacity) const -> std::size_t const*
    {
        auto c = capacity_;
        while (c < std::end(Capacities) && *c < aproxCapacity)
        {
            ++c;
        }
        return c;
    }
}

#endif