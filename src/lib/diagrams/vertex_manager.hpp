#ifndef MIX_DD_VERTEX_MANAGER
#define MIX_DD_VERTEX_MANAGER

#include "typedefs.hpp"
#include "graph.hpp"
#include "vertex_level_iterator.hpp"
#include "../utils/hash.hpp"
#include "../utils/more_iterator.hpp"
#include "../utils/more_algorithm.hpp"
#include "../utils/more_functional.hpp"

#include <cstddef>
#include <array>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <functional>
#include <utility>

#include <type_traits>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    class vertex_manager
    {
    public:
        using log_t            = typename log_val_traits<P>::type;
        using vertex_t         = vertex<VertexData, ArcData, P>;
        using son_a            = std::array<vertex_t*, P>;
        using index_map        = std::unordered_map<son_a, vertex_t*, utils::tuple_hash_t<son_a>>;
        using level_iterator   = vertex_level_iterator<typename index_map::const_iterator>;
        using level_iterator_p = std::pair<level_iterator, level_iterator>;

    public:
        vertex_manager  (std::size_t const varCount);
        vertex_manager  (vertex_manager&& other);
        ~vertex_manager ();
        explicit vertex_manager (vertex_manager const& other);

    public:
        auto terminal_vertex     (log_t const val) -> vertex_t*;
        auto get_terminal_vertex (log_t const val) const -> vertex_t*;
        auto has_terminal_vertex (log_t const val) const -> bool;
        auto internal_vertex     (index_t const index, son_a const& sons) -> vertex_t*;
        auto get_level           (vertex_t* const v) const -> level_t;
        auto get_level           (index_t const i)   const -> level_t;
        auto get_terminal_value  (vertex_t* const v) const -> log_t;
        auto is_leaf             (vertex_t* const v) const -> bool;
        auto get_var_count       () const -> std::size_t;
        auto get_level_iterators (level_t const level) const -> level_iterator_p;

        template<class Op, class Filter = utils::not_null_t>
        auto for_each_terminal (Op op, Filter filter = Filter()) -> void;

        static auto is_redundant  (son_a const& sons) -> bool;
        static auto inc_ref_count (vertex_t* const v) -> vertex_t*;
        static auto dec_ref_count (vertex_t* const v) -> void;

    private:
        using index_map_v    = std::vector<index_map>;
        using level_v        = std::vector<level_t>;
        using leaf_vertex_a  = std::array<vertex_t*, log_val_traits<P>::valuecount>;
        using alloc_t        = std::allocator<vertex_t>;
        using alloc_traits_t = std::allocator_traits<alloc_t>;

    private:
        auto leaf_index         () const -> index_t;
        auto get_level_index    (level_t const level) const -> index_t;
        auto new_empty_vertex   () -> vertex_t*;
        auto new_shallow_vertex (vertex_t* const v) -> vertex_t*;
        auto delete_vertex      (vertex_t* const v) -> void;
        auto do_deep_copy       (vertex_manager const& other) -> void;
        auto do_shallow_copy    (vertex_manager const& other) -> std::unordered_map<vertex_t*, vertex_t*>;

    private:
        index_map_v   indexMaps_;
        leaf_vertex_a leaves_;
        level_v       levels_;
        alloc_t       alloc_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    vertex_manager<VertexData, ArcData, P>::vertex_manager
        (std::size_t const varCount) :
        indexMaps_ {varCount},
        leaves_    {{}},
        alloc_     {alloc_t {}}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex_manager<VertexData, ArcData, P>::vertex_manager
        (vertex_manager&& other) :
        indexMaps_ {std::move(other.indexMaps_)},
        leaves_    {std::move(other.leaves_)},
        levels_    {std::move(other.levels_)},
        alloc_     {std::move(other.alloc_)}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex_manager<VertexData, ArcData, P>::vertex_manager
        (vertex_manager const& other) :
        indexMaps_ {other.get_var_count()},
        leaves_    {{}},
        levels_    {other.levels_},
        alloc_     {other.alloc_}
    {
        this->do_deep_copy(other);
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex_manager<VertexData, ArcData, P>::~vertex_manager
        ()
    {
        for (auto& indexMap : indexMaps_)
        {
            for (auto& [key, v] : indexMap)
            {
                this->delete_vertex(v);
            }
        }

        for (auto const v : leaves_)
        {
            if (v)
            {
                this->delete_vertex(v);
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::terminal_vertex
        (log_t const val) -> vertex_t*
    {
        if (leaves_[val])
        {
            return leaves_[val];
        }

        auto const v = this->new_empty_vertex();
        alloc_traits_t::construct(alloc_, v, this->leaf_index());
        leaves_[val] = v;
        return v;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_terminal_vertex
        (log_t const val) const -> vertex_t*
    {
        return leaves_[val];
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::has_terminal_vertex
        (log_t const val) const -> bool
    {
        return leaves_[val];
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::internal_vertex
        (index_t const index, son_a const& sons) -> vertex_t*
    {
        if (is_redundant(sons))
        {
            return sons.front();
        }

        auto& indexMap = indexMaps_[index];
        auto existing  = indexMap.find(sons);
        if (indexMap.end() != existing)
        {
            return existing->second;
        }

        auto const v = this->new_empty_vertex();
        alloc_traits_t::construct(alloc_, v, index, sons);
        indexMap.emplace(sons, v);

        for (auto v : sons)
        {
            v->inc_ref_count();
        }

        return v;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_level
        (vertex_t* const v) const -> level_t
    {
        return this->get_level(v->get_index());
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_level
        (index_t const i) const -> level_t
    {
        return levels_.empty() ? i : levels_[i];
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_terminal_value
        (vertex_t* const v) const -> log_t
    {
        return this->is_leaf(v) ? static_cast<log_t>(utils::index_of( std::begin(leaves_)
                                                                    , std::end(leaves_), v ))
                                : log_val_traits<P>::nondetermined;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::is_leaf
        (vertex_t* const v) const -> bool
    {
        return v->get_index() == this->leaf_index();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_var_count
        () const -> std::size_t
    {
        return indexMaps_.size();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_level_iterators
        (level_t const level) const -> level_iterator_p
    {
        auto const& indexMap = indexMaps_[this->get_level_index(level)];
        return std::make_pair(std::begin(indexMap), std::end(indexMap));
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Op, class Filter>
    auto vertex_manager<VertexData, ArcData, P>::for_each_terminal
        (Op op, Filter filter) -> void
    {
        for (auto const v : leaves_)
        {
            if (filter(v))
            {
                op(v);
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::is_redundant
        (son_a const& sons) -> bool
    {
        return std::end(sons) == std::adjacent_find( std::begin(sons)
                                                   , std::end(sons)
                                                   , std::not_equal_to<>() );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::inc_ref_count
        (vertex_t* const v) -> vertex_t*
    {
        v->inc_ref_count();
        return v;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::dec_ref_count
        (vertex_t* const v) -> void
    {
        v->dec_ref_count();
        if (0 == v->get_ref_count())
        {
            for (auto i = log_t {0}; i < P; ++i)
            {
                auto const son = v->get_son(i);
                if (son)
                {
                    dec_ref_count(son);
                }
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::new_empty_vertex
        () -> vertex_t*
    {
        return alloc_traits_t::allocate(alloc_, 1);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::new_shallow_vertex
        (vertex_t* const v) -> vertex_t*
    {
        auto const newv = alloc_traits_t::allocate(alloc_, 1);
        alloc_traits_t::construct(alloc_, newv, *v);
        return newv;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::leaf_index
        () const -> index_t
    {
        return static_cast<index_t>(this->get_var_count());
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_level_index
        (level_t const level) const -> index_t
    {
        return levels_.empty() ? level
                               : utils::index_of(std::begin(levels_), std::end(levels_), level);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::delete_vertex
        (vertex_t* const v) -> void
    {
        alloc_traits_t::deallocate(alloc_, v, 1);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::do_deep_copy
        (vertex_manager const& other) -> void
    {
        auto shallowMap = this->do_shallow_copy(other);

        for (auto&& [thismap, othermap] : utils::zip(indexMaps_, other.indexMaps_))
        {
            for (auto const& [otherkey, otherv] : othermap)
            {
                auto newkey = utils::map_to_array(otherkey, [&](auto const v)
                {
                    return shallowMap.at(v);
                });
                auto newv = shallowMap.at(otherv);
                thismap.emplace(newkey, newv);
            }
        }

        leaves_ = utils::map_to_array(other.leaves_, [&](auto const v)
        {
            return shallowMap.at(v);
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::do_shallow_copy
        (vertex_manager const& other) -> std::unordered_map<vertex_t*, vertex_t*>
    {
        auto map = std::unordered_map<vertex_t*, vertex_t*>();

        for (auto const& indexMap : other.indexMaps_)
        {
            for (auto const& [key, v] : indexMap)
            {
                map.emplace(v, this->new_shallow_vertex(v));
            }
        }

        for (auto const v : other.leaves_)
        {
            if (v)
            {
                map.emplace(v, this->new_shallow_vertex(v));
            }
        }

        map.emplace(nullptr, nullptr);

        return map;
    }
}

#endif