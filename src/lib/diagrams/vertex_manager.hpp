#ifndef MIX_DD_VERTEX_MANAGER
#define MIX_DD_VERTEX_MANAGER

#include "typedefs.hpp"
#include "graph.hpp"
#include "../utils/hash.hpp"
#include "../utils/more_iterator.hpp"
#include "../utils/more_algorithm.hpp"
#include "../utils/more_functional.hpp"
#include "../utils/more_assert.hpp"

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
        using log_t    = typename log_val_traits<P>::type;
        using vertex_t = vertex<VertexData, ArcData, P>;
        using son_a    = std::array<vertex_t*, P>;
        using index_v  = std::vector<level_t>;
        using level_v  = std::vector<level_t>;

    public:
        vertex_manager  (std::size_t const varCount);
        vertex_manager  (vertex_manager&& other);
        ~vertex_manager ();

        auto set_order (index_v levelToIndex) -> void;

    public:
        auto terminal_vertex     (log_t const val)       -> vertex_t*;
        auto get_terminal_vertex (log_t const val) const -> vertex_t*;
        auto has_terminal_vertex (log_t const val) const -> bool;
        auto internal_vertex     (index_t const index, son_a const& sons) -> vertex_t*;
        auto get_vertex_level           (vertex_t* const v) const -> level_t;
        auto get_level           (index_t const i)   const -> level_t;
        auto get_index           (level_t const l)   const -> index_t;
        auto get_vertex_value    (vertex_t* const v) const -> log_t;
        auto is_leaf_vertex      (vertex_t* const v) const -> bool;
        auto is_leaf_index       (index_t   const i) const -> bool;
        auto is_leaf_level       (level_t   const l) const -> bool;
        auto vertex_count        (index_t   const i) const -> std::size_t;
        auto vertex_count        () const -> std::size_t;
        auto var_count           () const -> std::size_t;
        auto collect_garbage     ()       -> void;
        auto clear               ()       -> void;

        auto swap_vars (index_t const i) -> void;

        template<class VertexOp>
        auto for_each_vertex (VertexOp op) const -> void;

        template<class VertexOp>
        auto for_each_terminal_vertex (VertexOp op) const -> void;

        static auto is_redundant  (son_a const& sons) -> bool;
        static auto inc_ref_count (vertex_t* const v) -> vertex_t*;
        static auto dec_ref_count (vertex_t* const v) -> void;

    private:
        // using index_map      = std::unordered_map<son_a, vertex_t*, utils::tuple_hash_t<son_a>>;
        using index_map      = std::unordered_multimap<son_a, vertex_t*, utils::tuple_hash_t<son_a>>;
        using index_map_v    = std::vector<index_map>;
        using leaf_vertex_a  = std::array<vertex_t*, log_val_traits<P>::valuecount>;
        using alloc_t        = std::allocator<vertex_t>;
        using alloc_traits_t = std::allocator_traits<alloc_t>;

    private:
        auto leaf_index       () const -> index_t;
        auto leaf_level       () const -> level_t;
        auto leaf_count       () const -> std::size_t;
        auto new_empty_vertex () -> vertex_t*;
        auto new_shallow_copy (vertex_t* const v) -> vertex_t*;
        auto delete_vertex    (vertex_t* const v) -> void;
        auto find_inverse     (level_v const& indexToLevel) const -> index_v;
        auto swap_vertex      (vertex_t* const v) -> void;

        template<class IndexMapOp>
        auto for_each_level (IndexMapOp op) -> void;

        template<class IndexMapOp>
        auto for_each_level (IndexMapOp op) const -> void;

        template<bool IsConst, class IndexMapOp>
        auto for_each_level_impl (IndexMapOp op) const -> void;

    private:
        index_map_v   indexToMap_;
        leaf_vertex_a leaves_;
        level_v       indexToLevel_;
        index_v       levelToIndex_;
        alloc_t       alloc_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    vertex_manager<VertexData, ArcData, P>::vertex_manager
        (std::size_t const varCount) :
        indexToMap_ {varCount},
        leaves_     {{}},
        alloc_      {alloc_t {}}
    {
        // TODO init levelToIndex_ & indexToLevel_
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex_manager<VertexData, ArcData, P>::vertex_manager
        (vertex_manager&& other) :
        indexToMap_    {std::move(other.indexToMap_)},
        leaves_        {std::move(other.leaves_)},
        indexToLevel_  {std::move(other.indexToLevel_)},
        levelToIndex_  {std::move(other.levelToIndex_)},
        alloc_         {std::move(other.alloc_)}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex_manager<VertexData, ArcData, P>::~vertex_manager
        ()
    {
        this->for_each_vertex(std::bind_front(&vertex_manager::delete_vertex, this));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::set_order
        (index_v levelToIndex) -> void
    {
        utils::runtime_assert( 0 == this->vertex_count()
                             , "vertex_manager::set_order: Manager must be empty." );
        utils::runtime_assert( this->var_count() == levelToIndex.size()
                             , "vertex_manager::set_order: Level vector size must match var count." );
        levelToIndex_ = std::move(levelToIndex);
        indexToLevel_ = this->find_inverse(levelToIndex_);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::terminal_vertex
        (log_t const val) -> vertex_t*
    {
        // TODO put asserts in methods like this
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

        auto& indexMap = indexToMap_[index];
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
    auto vertex_manager<VertexData, ArcData, P>::get_vertex_level
        (vertex_t* const v) const -> level_t
    {
        return this->get_level(v->get_index());
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_level
        (index_t const i) const -> level_t
    {
        return indexToLevel_.empty()  ? i :
               this->is_leaf_index(i) ? i : indexToLevel_[i];
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_index
        (level_t const l) const -> index_t
    {
        return levelToIndex_.empty()   ? l :
               l == this->leaf_level() ? l : levelToIndex_[l];
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_vertex_value
        (vertex_t* const v) const -> log_t
    {
        if (this->is_leaf_vertex(v))
        {
            return static_cast<log_t>(utils::index_of( std::begin(leaves_)
                                                     , std::end(leaves_), v ));
        }
        else
        {
            return log_val_traits<P>::nondetermined;
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::is_leaf_vertex
        (vertex_t* const v) const -> bool
    {
        return this->is_leaf_index(v->get_index());
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::is_leaf_index
        (index_t const i) const -> bool
    {
        return i == this->leaf_index();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::is_leaf_level
        (level_t const l) const -> bool
    {
        return l == this->leaf_level();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::vertex_count
        (index_t const i) const -> std::size_t
    {
        // TODO assert
        if (this->leaf_level() == i)
        {
            return this->leaf_count();
        }
        else
        {
            auto count = 0ul;
            auto const& map = indexToMap_[i];
            std::for_each(std::begin(map), std::end(map), [&count](auto&&){ ++count; });
            return count;
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::vertex_count
        () const -> std::size_t
    {
        auto count = std::size_t {0};
        this->for_each_level([&count](auto const& level) { count += level.size(); });
        count += this->leaf_count();
        return count;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::var_count
        () const -> std::size_t
    {
        return indexToMap_.size();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::collect_garbage
        () -> void
    {
        this->for_each_level([this](auto& indexMap)
        {
            auto const end = std::end(indexMap);
            auto it = std::begin(indexMap);

            while (it != end)
            {
                auto const v = it->second;
                if (0 == v->get_ref_count())
                {
                    v->for_each_son(dec_ref_count);
                    this->delete_vertex(v);
                    it = indexMap.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        });

        for (auto i = 0u; i < leaves_.size(); ++i)
        {
            if (leaves_[i] && 0 == leaves_[i]->get_ref_count())
            {
                this->delete_vertex(std::exchange(leaves_[i], nullptr));
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::clear
        () -> void
    {
        this->for_each_vertex(std::bind_front(&vertex_manager::delete_vertex, this));
        this->for_each_level([](auto& levelMap) { levelMap.clear(); });
        std::fill(std::begin(leaves_), std::end(leaves_), nullptr);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::swap_vars
        (index_t const i) -> void
    {
        // TODO bound check

        auto const iLevel    = this->get_level(i);
        auto const nextIndex = this->get_index(1 + iLevel);
        auto tmpIndexMap     = index_map(std::move(indexToMap_[i]));
        for (auto&& [key, v] : tmpIndexMap)
        {
            this->swap_vertex(v);
        }
        indexToMap_[nextIndex].merge(tmpIndexMap);

        std::swap(levelToIndex_[iLevel], levelToIndex_[1 + iLevel]);
        ++indexToLevel_[i];
        --indexToLevel_[nextIndex];
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto vertex_manager<VertexData, ArcData, P>::for_each_vertex
        (VertexOp op) const -> void
    {
        for (auto& indexMap : indexToMap_)
        {
            for (auto& [key, v] : indexMap)
            {
                op(v);
            }
        }

        for (auto const v : leaves_)
        {
            if (v)
            {
                op(v);
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto vertex_manager<VertexData, ArcData, P>::for_each_terminal_vertex
        (VertexOp op) const -> void
    {
        for (auto const v : leaves_)
        {
            if (v)
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
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::new_empty_vertex
        () -> vertex_t*
    {
        return alloc_traits_t::allocate(alloc_, 1);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::new_shallow_copy
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
        return static_cast<index_t>(this->var_count());
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::leaf_level
        () const -> level_t
    {
        return static_cast<level_t>(this->var_count());
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::leaf_count
        () const -> std::size_t
    {
        auto const nulls = std::count( std::begin(leaves_)
                                     , std::end(leaves_)
                                     , nullptr );
        return leaves_.size() - static_cast<std::size_t>(nulls);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::delete_vertex
        (vertex_t* const v) -> void
    {
        alloc_traits_t::deallocate(alloc_, v, 1);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::find_inverse
        (level_v const& indexToLevel) const -> level_v
    {
        auto levelToIndex = std::vector<index_t>(indexToLevel.size());

        auto i = 0u;
        for (auto const level : indexToLevel)
        {
            levelToIndex[level] = i++;
        }

        return levelToIndex;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::swap_vertex
        (vertex_t* const v) -> void
    {
        auto const index     = v->get_index();
        auto const nextIndex = this->get_index(1 + this->get_vertex_level(v));
        auto const cofactors = utils::fill_array<P>([=](auto const sonIndex)
        {
            return utils::fill_array<P>([=](auto const sonSonIndex)
            {
                auto const son = v->get_son(sonIndex);
                return son->get_index() == nextIndex
                            ? son->get_son(sonSonIndex)
                            : son;
            });
        });
        v->for_each_son(dec_ref_count);
        v->set_index(nextIndex);
        v->set_sons(utils::fill_array<P>([=, this, &cofactors](auto const i)
        {
            return this->internal_vertex(index, utils::fill_array<P>([=, &cofactors](auto const j)
            {
                return cofactors[j][i];
            }));
        }));
        v->for_each_son(inc_ref_count);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class IndexMapOp>
    auto vertex_manager<VertexData, ArcData, P>::for_each_level
        (IndexMapOp op) -> void
    {
        this->for_each_level_impl<false>(op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class IndexMapOp>
    auto vertex_manager<VertexData, ArcData, P>::for_each_level
        (IndexMapOp op) const -> void
    {
        this->for_each_level_impl<true>(op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<bool IsConst, class IndexMapOp>
    auto vertex_manager<VertexData, ArcData, P>::for_each_level_impl
        (IndexMapOp op) const -> void
    {
        using index_map_v_ref = std::conditional_t<IsConst, index_map_v const&, index_map_v&>;
        auto&& indexToMapRef = [this]() -> index_map_v_ref
        {
            if constexpr (IsConst)
            {
                return indexToMap_;
            }
            else
            {
                return const_cast<vertex_manager&>(*this).indexToMap_;
            }
        }();

        if (levelToIndex_.empty())
        {
            for (auto index = 0u; index < this->var_count(); ++index)
            {
                op(indexToMapRef[index]);
            }
        }
        else
        {
            for (auto const index : levelToIndex_)
            {
                op(indexToMapRef[index]);
            }
        }
    }
}

#endif