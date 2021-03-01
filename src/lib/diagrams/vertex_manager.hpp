#ifndef MIX_DD_VERTEX_MANAGER_HPP
#define MIX_DD_VERTEX_MANAGER_HPP

#include "typedefs.hpp"
#include "graph.hpp"
#include "operators.hpp"
#include "unique_table.hpp"
#include "bin_op_cache.hpp"
#include "../utils/object_pool.hpp"
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
        using log_t             = typename log_val_traits<P>::type;
        using vertex_t          = vertex<VertexData, ArcData, P>;
        using vertex_a          = std::array<vertex_t*, P>;
        using index_v           = std::vector<level_t>;
        using level_v           = std::vector<level_t>;
        using op_cache_t        = bin_op_cache<VertexData, ArcData, P>;
        using op_cache_iterator = typename op_cache_t::iterator;

    public:
        vertex_manager  (std::size_t const varCount);
        vertex_manager  (vertex_manager&& other) = default;
        ~vertex_manager ();

        auto set_order (index_v levelToIndex) -> void;

    public:
        auto terminal_vertex  (log_t const val) const                     -> vertex_t*;
        auto terminal_vertex  (log_t const val)                           -> vertex_t*;
        auto internal_vertex  (index_t const index, vertex_a const& sons) -> vertex_t*;

        auto get_vertex_level (vertex_t* const v) const -> level_t;
        auto get_level        (index_t   const i) const -> level_t;
        auto get_index        (level_t   const l) const -> index_t;
        auto get_vertex_value (vertex_t* const v) const -> log_t;
        auto get_vertex_count (index_t   const i) const -> std::size_t;
        auto get_vertex_count ()                  const -> std::size_t;
        auto get_var_count    ()                  const -> std::size_t;
        auto get_last_level   ()                  const -> level_t;

        auto is_leaf_vertex   (vertex_t* const v) const -> bool;
        auto is_leaf_index    (index_t   const i) const -> bool;
        auto is_leaf_level    (level_t   const l) const -> bool;

        template<class Op>
        auto cache_find       (vertex_t* const l, vertex_t* const r) -> op_cache_iterator;
        template<class Op>
        auto cache_put        (op_cache_iterator it, vertex_t* const l, vertex_t* const r, vertex_t* const res) -> void;

        auto adjust_sizes     ()                        -> void;
        auto collect_garbage  ()                        -> void;
        auto clear            ()                        -> void;

        auto swap_vars (index_t const i) -> void;

        template<class VertexOp>
        auto for_each_vertex (VertexOp op) const -> void;

        template<class VertexOp>
        auto for_each_terminal_vertex (VertexOp op) const -> void;

        static auto is_redundant  (vertex_a const& sons) -> bool;
        static auto inc_ref_count (vertex_t* const v)    -> vertex_t*;
        static auto dec_ref_count (vertex_t* const v)    -> void;

    private:
        using unique_table_t = unique_table<VertexData, ArcData, P>;
        using unique_table_v = std::vector<unique_table_t>;
        using leaf_vertex_a  = std::array<vertex_t*, log_val_traits<P>::valuecount>;
        using op_cache_a     = std::array<op_cache_t, op_count()>;
        using vertex_pool_t  = utils::object_pool<vertex_t>;

    private:
        auto leaf_index  () const -> index_t;
        auto leaf_level  () const -> level_t;
        auto leaf_count  () const -> std::size_t;

        auto clear_cache () -> void;

        auto swap_vertex (vertex_t* const v) -> void;

        auto delete_vertex (vertex_t* const v) -> void;

        template<class... Args>
        auto new_vertex  (Args&&... args) -> vertex_t*;

        template<class IndexMapOp>
        auto for_each_level (IndexMapOp op) -> void;

        template<class IndexMapOp>
        auto for_each_level (IndexMapOp op) const -> void;

        template<bool IsConst, class IndexMapOp>
        auto for_each_level_impl (IndexMapOp op) const -> void;

    private:
        inline static constexpr auto PoolSize = 2'000'000;

    private:
        unique_table_v uniqueTables_;
        leaf_vertex_a  leaves_;
        level_v        indexToLevel_;
        index_v        levelToIndex_;
        op_cache_a     opCaches_;
        vertex_pool_t  pool_;
        bool           needsGc_;
        std::size_t    vertexCount_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    vertex_manager<VertexData, ArcData, P>::vertex_manager
        (std::size_t const varCount) :
        uniqueTables_ {varCount},
        leaves_       {{}},
        pool_         {PoolSize},
        needsGc_      {false},
        vertexCount_  {0}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    vertex_manager<VertexData, ArcData, P>::~vertex_manager
        ()
    {
        // Note: toto teoreticky s prealokovaným poolom netreba.
        // this->for_each_vertex(std::bind_front(&vertex_pool_t::destroy, std::ref(pool_)));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::set_order
        (index_v levelToIndex) -> void
    {
        utils::runtime_assert(0 == this->get_vertex_count(), "vertex_manager::set_order: Manager must be empty.");
        utils::runtime_assert(this->get_var_count() == levelToIndex.size(), "vertex_manager::set_order: Level vector size must match var count.");

        levelToIndex_ = std::move(levelToIndex);
        indexToLevel_ = std::vector<level_t>(levelToIndex_.size());
        auto level    = 0u;
        for (auto const index : levelToIndex_)
        {
            indexToLevel_[index] = level++;
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::terminal_vertex
        (log_t const val) const -> vertex_t*
    {
        return leaves_[val];
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::terminal_vertex
        (log_t const val) -> vertex_t*
    {
        if (leaves_[val])
        {
            return leaves_[val];
        }

        auto const v = this->new_vertex(this->leaf_index());
        leaves_[val] = v;
        return v;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::internal_vertex
        (index_t const index, vertex_a const& sons) -> vertex_t*
    {
        if (is_redundant(sons))
        {
            return sons.front();
        }

        auto& indexMap      = uniqueTables_[index];
        auto const existing = indexMap.find(sons);
        if (existing)
        {
            return existing;
        }

        auto v = this->new_vertex(index, sons);
        indexMap.insert(v);
        v->for_each_son(inc_ref_count);

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
    auto vertex_manager<VertexData, ArcData, P>::get_vertex_count
        (index_t const i) const -> std::size_t
    {
        if (this->leaf_level() == i)
        {
            return this->leaf_count();
        }
        else
        {
            auto count = 0ul;
            auto const& map = uniqueTables_[i];
            std::for_each(std::begin(map), std::end(map), [&count](auto&&){ ++count; });
            return count;
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_vertex_count
        () const -> std::size_t
    {
        auto count = std::size_t {0};
        this->for_each_level([&count](auto const& level) { count += level.size(); });
        count += this->leaf_count();
        return count;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_var_count
        () const -> std::size_t
    {
        return uniqueTables_.size();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_last_level
        () const -> level_t
    {
        return static_cast<level_t>(this->get_var_count() - 1);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Op>
    auto vertex_manager<VertexData, ArcData, P>::cache_find
        (vertex_t* const l, vertex_t* const r) -> op_cache_iterator
    {
        auto& cache = opCaches_[op_id(Op())];
        return cache.find(l, r);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Op>
    auto vertex_manager<VertexData, ArcData, P>::cache_put
        (op_cache_iterator it, vertex_t* const l, vertex_t* const r, vertex_t* const res) -> void
    {
        auto& cache = opCaches_[op_id(Op())];
        cache.put(it, l, r, res);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::adjust_sizes
        () -> void
    {
        for (auto& t : uniqueTables_)
        {
            t.adjust_capacity();
        }

        for (auto& c : opCaches_)
        {
            c.adjust_capacity(vertexCount_ / 4);
        }

        if (needsGc_)
        {
            this->collect_garbage();
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::collect_garbage
        () -> void
    {
        needsGc_ = false;
        this->clear_cache();

        this->for_each_level([this](auto& indexMap)
        {
            auto const end = std::end(indexMap);
            auto it = std::begin(indexMap);

            while (it != end)
            {
                auto const v = *it;
                if (0 == v->get_ref_count())
                {
                    v->for_each_son(dec_ref_count);
                    it = indexMap.erase(it);
                    this->delete_vertex(v);
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
        this->for_each_vertex(std::bind_front(&vertex_pool_t::destroy, std::ref(pool_)));
        this->for_each_level([](auto& levelMap) { levelMap.clear(); });
        std::fill(std::begin(leaves_), std::end(leaves_), nullptr);
        vertexCount_ = 0;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::swap_vars
        (index_t const i) -> void
    {
        auto const iLevel    = this->get_level(i);
        auto const nextIndex = this->get_index(1 + iLevel);
        auto tmpIndexMap     = unique_table_t(std::move(uniqueTables_[i]));
        for (auto&& [key, v] : tmpIndexMap)
        {
            this->swap_vertex(v);
        }
        uniqueTables_[nextIndex].merge(tmpIndexMap);

        std::swap(levelToIndex_[iLevel], levelToIndex_[1 + iLevel]);
        ++indexToLevel_[i];
        --indexToLevel_[nextIndex];
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto vertex_manager<VertexData, ArcData, P>::for_each_vertex
        (VertexOp op) const -> void
    {
        for (auto const& table : uniqueTables_)
        {
            auto const end = std::end(table);
            auto it = std::begin(table);
            while (it != end)
            {
                auto const v = *it;
                ++it;
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
        (vertex_a const& sons) -> bool
    {
        auto const end = std::find(std::begin(sons), std::end(sons), nullptr);
        return end == std::adjacent_find( std::begin(sons)
                                        , end
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
    auto vertex_manager<VertexData, ArcData, P>::leaf_index
        () const -> index_t
    {
        return static_cast<index_t>(this->get_var_count());
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::leaf_level
        () const -> level_t
    {
        return static_cast<level_t>(this->get_var_count());
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
    auto vertex_manager<VertexData, ArcData, P>::clear_cache
        () -> void
    {
        for (auto& c : opCaches_)
        {
            c.clear();
        }
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
    auto vertex_manager<VertexData, ArcData, P>::delete_vertex
        (vertex_t* const v) -> void
    {
        --vertexCount_;
        pool_.destroy(v);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class... Args>
    auto vertex_manager<VertexData, ArcData, P>::new_vertex
        (Args&&... args) -> vertex_t*
    {
        ++vertexCount_;
        auto const v = pool_.try_create(std::forward<Args>(args)...);
        if (v)
        {
            return v;
        }

        needsGc_ = true;

        return pool_.force_create(std::forward<Args>(args)...);
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
        using index_map_v_ref = std::conditional_t<IsConst, unique_table_v const&, unique_table_v&>;
        auto&& indexToMapRef = [this]() -> index_map_v_ref
        {
            if constexpr (IsConst)
            {
                return uniqueTables_;
            }
            else
            {
                return const_cast<vertex_manager&>(*this).uniqueTables_;
            }
        }();

        if (levelToIndex_.empty())
        {
            for (auto index = 0u; index < this->get_var_count(); ++index)
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