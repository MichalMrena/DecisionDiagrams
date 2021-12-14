#ifndef MIX_DD_VERTEX_MANAGER_HPP
#define MIX_DD_VERTEX_MANAGER_HPP

#include "graph.hpp"
#include "operators.hpp"
#include "unique_table.hpp"
#include "apply_cache.hpp"
#include "../utils/object_pool.hpp"
#include "../utils/more_algorithm.hpp"
#include "../utils/more_assert.hpp"

#include <array>
#include <vector>
#include <utility>
#include <type_traits>

namespace teddy
{
    template<class VertexData, class ArcData, std::size_t P>
    class vertex_manager
    {
    public:
        using log_t             = typename log_val_traits<P>::type;
        using vertex_t          = vertex<VertexData, ArcData, P>;
        using vertex_a          = std::array<vertex_t*, P>;
        using op_cache_t        = apply_cache<VertexData, ArcData, P>;
        using op_cache_iterator = typename op_cache_t::iterator;

    public:
        vertex_manager  (std::size_t varCount, std::size_t vertexCount);
        vertex_manager  (vertex_manager&& other) noexcept = default;
        vertex_manager  (vertex_manager const& other) = delete;

    public:
        auto terminal_vertex  (log_t val) const                     -> vertex_t*;
        auto terminal_vertex  (log_t val)                           -> vertex_t*;
        auto internal_vertex  (index_t index, vertex_a const& sons) -> vertex_t*;

        auto get_vertex_level        (vertex_t* v) const         -> level_t;
        auto get_level               (index_t   i) const         -> level_t;
        auto get_index               (level_t   l) const         -> index_t;
        auto get_vertex_value        (vertex_t* v) const         -> log_t;
        auto get_vertex_count        (index_t   i) const         -> std::size_t;
        auto get_vertex_count        ()            const         -> std::size_t;
        auto get_var_count           ()            const         -> std::size_t;
        auto get_last_internal_level ()            const         -> level_t;
        auto get_domain              (index_t i)   const         -> log_t;
        auto has_domains             ()            const         -> bool;
        auto set_domains             (std::vector<log_t> ds)     -> void;
        auto set_order               (std::vector<index_t> lToI) -> void;
        auto get_order               ()            const         -> std::vector<index_t> const&;
        auto set_cache_ratio         (std::size_t denom)         -> void;
        auto set_pool_ratio          (std::size_t denom)         -> void;
        auto set_reorder             (bool reorder)              -> void;
        auto is_leaf_vertex          (vertex_t* v) const         -> bool;
        auto is_leaf_index           (index_t   i) const         -> bool;
        auto is_leaf_level           (level_t   l) const         -> bool;

        template<class Op>
        auto cache_find       (vertex_t* l, vertex_t* r) -> op_cache_iterator;
        template<class Op>
        auto cache_put        (op_cache_iterator it, vertex_t* l, vertex_t* r, vertex_t* res) -> void;

        auto adjust_sizes     ()                  -> void;
        auto collect_garbage  ()                  -> void;
        auto clear            ()                  -> void;
        auto swap_vars        (index_t i)         -> void;
        auto sift_vars        ()                  -> void;

        template<class VertexOp>
        auto for_each_vertex (VertexOp op) const -> void;

        template<class VertexOp>
        auto for_each_terminal_vertex (VertexOp op) const -> void;

        static auto is_redundant  (vertex_a const& sons) -> bool;
        static auto inc_ref_count (vertex_t* v)          -> vertex_t*;
        static auto dec_ref_count (vertex_t* v)          -> void;

    private:
        using unique_table_t = unique_table<VertexData, ArcData, P>;
        using unique_table_v = std::vector<unique_table_t>;
        using leaf_vertex_a  = std::array<vertex_t*, log_val_traits<P>::valuecount>;
        using op_cache_a     = std::array<op_cache_t, op_count()>;
        using vertex_pool_t  = utils::object_pool<vertex_t>;

    private:
        auto leaf_index     () const      -> index_t;
        auto leaf_level     () const      -> level_t;
        auto leaf_count     () const      -> std::size_t;
        auto clear_cache    ()            -> void;
        auto swap_vertex    (vertex_t* v) -> void;
        auto dec_ref_try_gc (vertex_t* v) -> void;
        auto delete_vertex  (vertex_t* v) -> void;

        template<class... Args>
        auto new_vertex  (Args&&... args) -> vertex_t*;

        template<class IndexMapOp>
        auto for_each_level (IndexMapOp op) -> void;

        template<class IndexMapOp>
        auto for_each_level (IndexMapOp op) const -> void;

        template<bool IsConst, class IndexMapOp>
        auto for_each_level_impl (IndexMapOp op) const -> void;

    private:
        unique_table_v       uniqueTables_;
        leaf_vertex_a        leaves_;
        std::vector<level_t> indexToLevel_;
        std::vector<index_t> levelToIndex_;
        op_cache_a           opCaches_;
        vertex_pool_t        pool_;
        bool                 needsGc_;
        std::size_t          vertexCount_;
        std::vector<log_t>   domains_;
        std::size_t          cacheRatio_;
        bool                 reorderEnabled_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    vertex_manager<VertexData, ArcData, P>::vertex_manager
        (std::size_t const varCount, std::size_t const vertexCount) :
        uniqueTables_   (varCount),
        leaves_         ({}),
        indexToLevel_   (utils::fill_vector(varCount + 1, utils::identity)),
        levelToIndex_   (utils::fill_vector(varCount + 1, utils::identity)),
        opCaches_       ({}),
        pool_           (vertexCount),
        needsGc_        (false),
        vertexCount_    (0),
        domains_        ({}),
        cacheRatio_     (4),
        reorderEnabled_ (false)
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::set_order
        (std::vector<index_t> lToI) -> void
    {
        utils::runtime_assert(this->get_vertex_count() == 0, "vertex_manager::set_order: Manager must be empty.");
        utils::runtime_assert(this->get_var_count() == lToI.size(), "vertex_manager::set_order: Level vector size must match var count.");
        utils::runtime_assert(utils::distinct(lToI), "vertex_manager::set_order: Indices must be unique.");

        levelToIndex_ = std::move(lToI);
        indexToLevel_ = std::vector<level_t>(levelToIndex_.size());
        auto level    = 0u;
        for (auto const index : levelToIndex_)
        {
            indexToLevel_[index] = level++;
        }
        levelToIndex_.push_back(static_cast<index_t>(this->get_var_count()));
        indexToLevel_.push_back(static_cast<level_t>(this->get_var_count()));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_order
        () const -> std::vector<index_t> const&
    {
        return levelToIndex_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::set_cache_ratio
        (std::size_t const denom) -> void
    {
        utils::runtime_assert(denom > 0, "vertex_manager::set_cache_ratio: Denominator must be positive.");
        cacheRatio_ = denom;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::set_pool_ratio
        (std::size_t const denom) -> void
    {
        utils::runtime_assert(denom > 0, "vertex_manager::set_pool_ratio: Denominator must be positive.");
        pool_.set_overflow_ratio(denom);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::set_reorder
        (bool const reorder) -> void
    {
        reorderEnabled_ = reorder;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::set_domains
        (std::vector<log_t> ds) -> void
    {
        utils::runtime_assert(ds.size() == this->get_var_count(), "vertex_manager::set_domains: Argument size must match variable count.");
        domains_ = std::move(ds);
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
        return indexToLevel_[i];
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_index
        (level_t const l) const -> index_t
    {
        return levelToIndex_[l];
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
        return this->is_leaf_index(i)
            ? this->leaf_count()
            : uniqueTables_[i].size();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_vertex_count
        () const -> std::size_t
    {
        return vertexCount_;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_var_count
        () const -> std::size_t
    {
        return uniqueTables_.size();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_last_internal_level
        () const -> level_t
    {
        return static_cast<level_t>(this->get_var_count() - 1);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::get_domain
        (index_t const i) const -> log_t
    {
        return domains_.size() ? domains_[i] : P;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::has_domains
        () const -> bool
    {
        return !domains_.empty();
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
        if (needsGc_)
        {
            this->collect_garbage();
            if (reorderEnabled_)
            {
                this->sift_vars();
            }
        }

        for (auto& t : uniqueTables_)
        {
            t.adjust_capacity();
        }

        for (auto& c : opCaches_)
        {
            c.adjust_capacity(vertexCount_ / cacheRatio_);
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
        // this->for_each_vertex(std::bind_front(&vertex_pool_t::destroy, std::ref(pool_)));
        this->for_each_vertex([this](auto const v)
        {
            pool_.destroy(v);
        });
        this->for_each_level([](auto& levelMap) { levelMap.clear(); });
        std::fill(std::begin(leaves_), std::end(leaves_), nullptr);
        vertexCount_ = 0;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::swap_vars
        (index_t const i) -> void
    {
        // TODO dokončiť celý sift, ak je počet vrcholov vyšší, postupne sa vracať

        auto const iLevel    = this->get_level(i);
        auto const nextIndex = this->get_index(1 + iLevel);
        auto tmpTable        = unique_table_t(std::move(uniqueTables_[i]));
        for (auto const v : tmpTable)
        {
            this->swap_vertex(v);
        }
        uniqueTables_[i].adjust_capacity();
        uniqueTables_[nextIndex].merge(tmpTable);

        std::swap(levelToIndex_[iLevel], levelToIndex_[1 + iLevel]);
        ++indexToLevel_[i];
        --indexToLevel_[nextIndex];
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::sift_vars
        () -> void
    {
        using count_pair = struct { index_t index; std::size_t count; };

        auto const get_sift_order = [this]()
        {
            auto counts = utils::fill_vector(this->get_var_count(), [this](auto const i)
            {
                return count_pair {i, this->get_vertex_count(i)};
            });
            std::sort(std::begin(counts), std::end(counts), [](auto&& l, auto&& r){ return l.count > r.count; });
            return counts;
        };

        auto const move_var_down = [this](auto const index)
        {
            this->swap_vars(index);
        };

        auto const move_var_up = [this](auto const index)
        {
            auto const level     = this->get_level(index);
            auto const prevIndex = this->get_index(level - 1);
            this->swap_vars(prevIndex);
        };

        auto const place_variable = [&, this](auto const index)
        {
            auto level        = this->get_level(index);
            auto optimalLevel = level;
            auto optimalCount = vertexCount_;

            // Sift down.
            while (level != this->get_last_internal_level())
            {
                move_var_down(index);
                ++level;
                if (vertexCount_ < optimalCount)
                {
                    optimalCount = vertexCount_;
                    optimalLevel = level;
                }
            }

            // Sift up.
            while (level != 0)
            {
                move_var_up(index);
                --level;
                if (vertexCount_ < optimalCount)
                {
                    optimalCount = vertexCount_;
                    optimalLevel = level;
                }
            }

            // Restore optimal position.
            while (level != optimalLevel)
            {
                move_var_down(index);
                ++level;
            }
        };

        auto const siftOrder = get_sift_order();
        for (auto pair : siftOrder)
        {
            place_variable(pair.index);
        }
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
        auto const vDomain   = this->get_domain(index);
        auto const sonDomain = this->get_domain(nextIndex);
        // auto const oldSons   = utils::fill_vector(vDomain, std::bind_front(&vertex_t::get_son, v));
        auto const oldSons   = utils::fill_vector(vDomain, [v](auto const i)
        {
            return v->get_son(i);
        });
        auto const cofactors = utils::fill_array_n<P>(vDomain, [=](auto const sonIndex)
        {
            auto const son = v->get_son(sonIndex);
            return son->get_index() == nextIndex
                // ? utils::fill_array_n<P>(sonDomain, std::bind_front(&vertex_t::get_son, son))
                ? utils::fill_array_n<P>(sonDomain, [son](auto const i){ return son->get_son(i); })
                : utils::fill_array_n<P>(sonDomain, utils::constant(son));
        });
        v->set_index(nextIndex);
        v->set_sons(utils::fill_array_n<P>(sonDomain, [=, &cofactors](auto const i)
        {
            return this->internal_vertex(index, utils::fill_array_n<P>(vDomain, [=, &cofactors](auto const j)
            {
                return cofactors[j][i];
            }));
        }));
        v->for_each_son(inc_ref_count);
        // utils::for_all(oldSons, std::bind_front(&vertex_manager::dec_ref_try_gc, this));
        utils::for_all(oldSons, [this](auto const s)
        {
            this->dec_ref_try_gc(s);
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::delete_vertex
        (vertex_t* const v) -> void
    {
        --vertexCount_;
        pool_.destroy(v);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto vertex_manager<VertexData, ArcData, P>::dec_ref_try_gc
        (vertex_t* const v) -> void
    {
        v->dec_ref_count();

        if (v->get_ref_count() > 0 || this->is_leaf_vertex(v))
        {
            return;
        }

        // v->for_each_son(std::bind_front(&vertex_manager::dec_ref_try_gc, this));
        v->for_each_son([this](auto const s)
        {
            this->dec_ref_try_gc(s);
        });
        uniqueTables_[v->get_index()].erase(v);
        this->delete_vertex(v);
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

        std::for_each_n(std::begin(levelToIndex_), this->get_var_count(), [&](auto const i)
        {
            op(indexToMapRef[i]);
        });
    }
}

#endif