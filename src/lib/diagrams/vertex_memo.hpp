#ifndef MIX_DD_VERTEX_MEMO_HPP
#define MIX_DD_VERTEX_MEMO_HPP

#include "typedefs.hpp"
#include "graph.hpp"
#include "operators.hpp"
#include "../utils/hash.hpp"

#include <tuple>
#include <utility>
#include <unordered_map>

namespace mix::dd
{
    // TODO nope just plain hashmap
    template<class VertexData, class ArcData, std::size_t P, class Key>
    class vertex_memo
    {
    public:
        using vertex_t       = vertex<VertexData, ArcData, P>;
        using key_hash       = utils::tuple_hash_t<Key>;
        using memo_m         = std::unordered_map<Key, vertex_t*, key_hash>;
        using iterator       = typename memo_m::iterator;
        using const_iterator = typename memo_m::const_iterator;

    public:
        auto emplace (Key const& k, vertex_t* const v) -> void;
        auto find    (Key const& k) const -> const_iterator;
        auto end     () const -> const_iterator;
        auto clear   ()       -> void;

    private:
        memo_m memo_;
    };

    template<class VertexData, class ArcData, std::size_t P, class Key>
    auto vertex_memo<VertexData, ArcData, P, Key>::emplace
        (Key const& k, vertex_t* const v) -> void
    {
        memo_.emplace(k, v);
    }

    template<class VertexData, class ArcData, std::size_t P, class Key>
    auto vertex_memo<VertexData, ArcData, P, Key>::find
        (Key const& k) const -> const_iterator
    {
        return memo_.find(k);
    }

    template<class VertexData, class ArcData, std::size_t P, class Key>
    auto vertex_memo<VertexData, ArcData, P, Key>::end
        () const -> const_iterator
    {
        return memo_.cend();
    }

    template<class VertexData, class ArcData, std::size_t P, class Key>
    auto vertex_memo<VertexData, ArcData, P, Key>::clear
        () -> void
    {
        memo_.clear();
    }
}

#endif