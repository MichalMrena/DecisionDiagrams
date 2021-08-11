#ifndef MIX_DD_node_manager_HPP
#define MIX_DD_node_manager_HPP

#include "utils.hpp"
#include "node.hpp"
#include "node_pool.hpp"
#include "hash_tables.hpp"
#include "operators.hpp"

#include <vector>

namespace teddy
{
    namespace domains
    {
        class mixed
        {
        private:
            std::vector<uint_t> ds_;
        public:
            mixed (std::vector<uint_t> ds) :
                ds_ (std::move(ds))
            {
            };
            auto operator[] (uint_t const i)
            {
                return ds_[i];
            }
        };

        template<uint_t N>
        struct fixed
        {
            static_assert(N > 1);
            constexpr auto operator[] (uint_t const)
            {
                return N;
            }
        };

        template<class T>
        struct is_fixed : public std::false_type {};

        template<uint_t N>
        struct is_fixed<fixed<N>> : public std::true_type {};

        template<class T>
        using is_mixed = std::is_same<T, mixed>;
    }

    template<class T>
    concept domain = domains::is_mixed<T>()() or domains::is_fixed<T>()();

    template<class Data, degree Degree, domain Domain>
    class node_manager
    {
    public:
        using node_t            = node<Data, Degree>;
        using sons_t            = typename node_t::sons_t;

        using op_cache_t        = apply_cache<VertexData, ArcData, P>;
        using op_cache_iterator = typename op_cache_t::iterator;

    private:
        inline static constexpr auto FixDomain = domains::is_fixed<Domain>()();
        inline static constexpr auto MixDomain = domains::is_mixed<Domain>()();
        struct common_init {};

    public:
        node_manager ( std::size_t vars
                     , std::size_t nodes ) requires(FixDomain);
        node_manager ( std::size_t vars
                     , std::size_t nodes
                     , domains::mixed ) requires(MixDomain);

        node_manager (node_manager&&) noexcept = default;
        node_manager (node_manager const&) = delete;

    private:
        node_manager ( common_init
                     , std::size_t vars
                     , std::size_t nodes
                     , Domain );

    public:
        auto get_terminal_node (uint_t) const      -> node_t*;
        auto terminal_node     (uint_t)            -> node_t*;
        auto internal_node     (index_t, sons_t&&) -> node_t*;

        auto get_vertex_level        (node_t* v) const         -> level_t;
        auto get_level               (index_t   i) const         -> level_t;
        auto get_index               (level_t   l) const         -> index_t;
        auto get_vertex_value        (node_t* v) const         -> log_t;
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
        auto is_leaf_vertex          (node_t* v) const         -> bool;
        auto is_leaf_index           (index_t   i) const         -> bool;
        auto is_leaf_level           (level_t   l) const         -> bool;

        template<class Op>
        auto cache_find       (node_t* l, node_t* r) -> op_cache_iterator;
        template<class Op>
        auto cache_put        (op_cache_iterator it, node_t* l, node_t* r, node_t* res) -> void;

        auto adjust_sizes     ()                  -> void;
        auto collect_garbage  ()                  -> void;
        auto clear            ()                  -> void;
        auto swap_vars        (index_t i)         -> void;
        auto sift_vars        ()                  -> void;

        template<class VertexOp>
        auto for_each_vertex (VertexOp op) const -> void;

        template<class VertexOp>
        auto for_each_terminal_node (VertexOp op) const -> void;


        auto is_redundant (index_t, sons_t const&) const -> bool;

        template<class NodeOp>
        auto for_each_son (node_t*, NodeOp&&) -> void;

        static auto inc_ref_count (node_t* v)       -> node_t*;
        static auto dec_ref_count (node_t* v)       -> void;

    private:
        using unique_table_t = unique_table<VertexData, ArcData, P>;
        using unique_table_v = std::vector<unique_table_t>;
        using leaf_vertex_a  = std::array<node_t*, log_val_traits<P>::valuecount>;
        using op_cache_a     = std::array<op_cache_t, op_count()>;
        using vertex_pool_t  = utils::object_pool<node_t>;

    private:
        auto leaf_index     () const      -> index_t;
        auto leaf_level     () const      -> level_t;
        auto leaf_count     () const      -> std::size_t;
        auto clear_cache    ()            -> void;
        auto swap_vertex    (node_t* v) -> void;
        auto dec_ref_try_gc (node_t* v) -> void;
        auto delete_vertex  (node_t* v) -> void;

        template<class... Args>
        auto new_vertex  (Args&&... args) -> node_t*;

        template<class IndexMapOp>
        auto for_each_level (IndexMapOp op) -> void;

        template<class IndexMapOp>
        auto for_each_level (IndexMapOp op) const -> void;

        template<bool IsConst, class IndexMapOp>
        auto for_each_level_impl (IndexMapOp op) const -> void;

    private:
        unique_table_v       uniqueTables_;
        std::vector<node_t*> terminals_;
        std::vector<level_t> indexToLevel_;
        std::vector<index_t> levelToIndex_;
        op_cache_a           opCaches_;
        vertex_pool_t        pool_;
        bool                 needsGc_;
        std::size_t          nodeCount_;
        std::size_t          cacheRatio_;
        bool                 reorderEnabled_;

        [[no_unique_address]]
        Domain               domains_;
    };

    template<class Data, degree Degree, domain Domain>
    node_manager<Data, Degree, Domain>::node_manager
        ( std::size_t const vars
        , std::size_t const nodes ) requires(FixDomain) :
        node_manager (common_init(), vars, nodes, {})
    {
    }

    template<class Data, degree Degree, domain Domain>
    node_manager<Data, Degree, Domain>::node_manager
        ( std::size_t const vars
        , std::size_t const nodes
        , domains::mixed    ds ) requires(MixDomain) :
        node_manager (common_init(), vars, nodes, std::move(ds))
    {
    }

    template<class Data, degree Degree, domain Domain>
    node_manager<Data, Degree, Domain>::node_manager
        ( common_init
        , std::size_t const vars
        , std::size_t const nodes
        , Domain            ds ) :
        uniqueTables_   (vars),
        terminals_      ({}),
        indexToLevel_   (utils::fill_vector(vars, utils::identity)),
        levelToIndex_   (utils::fill_vector(vars, utils::identity)),
        opCaches_       ({}),
        pool_           (nodes),
        needsGc_        (false),
        nodeCount_      (0),
        domains_        (std::move(ds)),
        cacheRatio_     (4),
        reorderEnabled_ (false)
    {
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_terminal_node
        (uint_t const v) const -> node_t*
    {
        return v < terminals_.size() ? terminals_[v] : nullptr;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::terminal_node
        (uint_t const v) -> node_t*
    {
        if (v >= terminals_.size())
        {
            terminals_.resize(v + 1, nullptr);
        }

        if (not terminals_[v])
        {
            terminals_[v] = this->new_terminal(v);
        }

        return terminals_[v];
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::internal_node
        (index_t const i, sons_t&& sons) -> node_t*
    {
        if (this->is_redundant(i, sons))
        {
            return sons[0];
        }

        auto& indexMap      = uniqueTables_[i];
        // auto const hash     = this->hash(i, sons);
        auto const existing = indexMap.find(sons);
        // auto const existing = indexMap.find(hash, sons);
        if (existing)
        {
            return existing;
        }

        auto n = this->new_internal(i, sons);
        indexMap.insert(n);
        // indexMap.insert(hash, n);
        this->for_each_son(n, inc_ref_count);

        return n;
    }







    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::set_order
        (std::vector<index_t> lToI) -> void
    {
        utils::runtime_assert(this->get_vertex_count() == 0, "node_manager::set_order: Manager must be empty.");
        utils::runtime_assert(this->get_var_count() == lToI.size(), "node_manager::set_order: Level vector size must match var count.");
        utils::runtime_assert(utils::distinct(lToI), "node_manager::set_order: Indices must be unique.");

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

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_order
        () const -> std::vector<index_t> const&
    {
        return levelToIndex_;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::set_cache_ratio
        (std::size_t const denom) -> void
    {
        utils::runtime_assert(denom > 0, "node_manager::set_cache_ratio: Denominator must be positive.");
        cacheRatio_ = denom;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::set_pool_ratio
        (std::size_t const denom) -> void
    {
        utils::runtime_assert(denom > 0, "node_manager::set_pool_ratio: Denominator must be positive.");
        pool_.set_overflow_ratio(denom);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::set_reorder
        (bool const reorder) -> void
    {
        reorderEnabled_ = reorder;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::set_domains
        (std::vector<log_t> ds) -> void
    {
        utils::runtime_assert(ds.size() == this->get_var_count(), "node_manager::set_domains: Argument size must match variable count.");
        domains_ = std::move(ds);
    }



    

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_vertex_level
        (node_t* const v) const -> level_t
    {
        return this->get_level(v->get_index());
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_level
        (index_t const i) const -> level_t
    {
        return indexToLevel_[i];
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_index
        (level_t const l) const -> index_t
    {
        return levelToIndex_[l];
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_vertex_value
        (node_t* const v) const -> log_t
    {
        if (this->is_leaf_vertex(v))
        {
            return static_cast<log_t>(utils::index_of( std::begin(terminals_)
                                                     , std::end(terminals_), v ));
        }
        else
        {
            return log_val_traits<P>::nondetermined;
        }
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::is_leaf_vertex
        (node_t* const v) const -> bool
    {
        return this->is_leaf_index(v->get_index());
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::is_leaf_index
        (index_t const i) const -> bool
    {
        return i == this->leaf_index();
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::is_leaf_level
        (level_t const l) const -> bool
    {
        return l == this->leaf_level();
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_vertex_count
        (index_t const i) const -> std::size_t
    {
        return this->is_leaf_index(i)
            ? this->leaf_count()
            : uniqueTables_[i].size();
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_vertex_count
        () const -> std::size_t
    {
        return nodeCount_;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_var_count
        () const -> std::size_t
    {
        return uniqueTables_.size();
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_last_internal_level
        () const -> level_t
    {
        return static_cast<level_t>(this->get_var_count() - 1);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_domain
        (index_t const i) const -> log_t
    {
        return domains_.size() ? domains_[i] : P;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::has_domains
        () const -> bool
    {
        return !domains_.empty();
    }

    template<class Data, degree Degree, domain Domain>
    template<class Op>
    auto node_manager<Data, Degree, Domain>::cache_find
        (node_t* const l, node_t* const r) -> op_cache_iterator
    {
        auto& cache = opCaches_[op_id(Op())];
        return cache.find(l, r);
    }

    template<class Data, degree Degree, domain Domain>
    template<class Op>
    auto node_manager<Data, Degree, Domain>::cache_put
        (op_cache_iterator it, node_t* const l, node_t* const r, node_t* const res) -> void
    {
        auto& cache = opCaches_[op_id(Op())];
        cache.put(it, l, r, res);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::adjust_sizes
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
            c.adjust_capacity(nodeCount_ / cacheRatio_);
        }
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::collect_garbage
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

        for (auto i = 0u; i < terminals_.size(); ++i)
        {
            if (terminals_[i] && 0 == terminals_[i]->get_ref_count())
            {
                this->delete_vertex(std::exchange(terminals_[i], nullptr));
            }
        }
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::clear
        () -> void
    {
        this->for_each_vertex(std::bind_front(&vertex_pool_t::destroy, std::ref(pool_)));
        this->for_each_level([](auto& levelMap) { levelMap.clear(); });
        std::fill(std::begin(terminals_), std::end(terminals_), nullptr);
        nodeCount_ = 0;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::swap_vars
        (index_t const i) -> void
    {
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

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::sift_vars
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
            auto optimalCount = nodeCount_;

            // Sift down.
            while (level != this->get_last_internal_level())
            {
                move_var_down(index);
                ++level;
                if (nodeCount_ < optimalCount)
                {
                    optimalCount = nodeCount_;
                    optimalLevel = level;
                }
            }

            // Sift up.
            while (level != 0)
            {
                move_var_up(index);
                --level;
                if (nodeCount_ < optimalCount)
                {
                    optimalCount = nodeCount_;
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

    template<class Data, degree Degree, domain Domain>
    template<class VertexOp>
    auto node_manager<Data, Degree, Domain>::for_each_vertex
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

        for (auto const v : terminals_)
        {
            if (v)
            {
                op(v);
            }
        }
    }

    template<class Data, degree Degree, domain Domain>
    template<class VertexOp>
    auto node_manager<Data, Degree, Domain>::for_each_terminal_node
        (VertexOp op) const -> void
    {
        for (auto const v : terminals_)
        {
            if (v)
            {
                op(v);
            }
        }
    }







    template<class Data, degree Degree, domain Domain>
    template<std::size_t N>
    auto node_manager<Data, Degree, Domain>::is_redundant
        (index_t const i, sons_t const& sons) const -> bool
    {
        for (auto j = 1u; j < domains_[i]; ++j)
        {
            if (sons[j - 1] != sons[j])
            {
                return false;
            }
        }
        return true;
    }

    template<class Data, degree Degree, domain Domain>
    template<class NodeOp>
    auto node_manager<Data, Degree, Domain>::for_each_son
        (node_t* const node, NodeOp&& f) -> void
    {
        auto const i = node->get_index();
        for (auto j = 0u; j < domains_[i]; ++j)
        {
            f(node->get_son(j));
        }
    }











    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::inc_ref_count
        (node_t* const v) -> node_t*
    {
        v->inc_ref_count();
        return v;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::dec_ref_count
        (node_t* const v) -> void
    {
        v->dec_ref_count();
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::leaf_index
        () const -> index_t
    {
        return static_cast<index_t>(this->get_var_count());
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::leaf_level
        () const -> level_t
    {
        return static_cast<level_t>(this->get_var_count());
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::leaf_count
        () const -> std::size_t
    {
        auto const nulls = std::count( std::begin(terminals_)
                                     , std::end(terminals_)
                                     , nullptr );
        return terminals_.size() - static_cast<std::size_t>(nulls);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::clear_cache
        () -> void
    {
        for (auto& c : opCaches_)
        {
            c.clear();
        }
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::swap_vertex
        (node_t* const v) -> void
    {
        auto const index     = v->get_index();
        auto const nextIndex = this->get_index(1 + this->get_vertex_level(v));
        auto const vDomain   = this->get_domain(index);
        auto const sonDomain = this->get_domain(nextIndex);
        auto const oldSons   = utils::fill_vector(vDomain, std::bind_front(&node_t::get_son, v));
        auto const cofactors = utils::fill_array_n<P>(vDomain, [=](auto const sonIndex)
        {
            auto const son = v->get_son(sonIndex);
            return son->get_index() == nextIndex
                ? utils::fill_array_n<P>(sonDomain, std::bind_front(&node_t::get_son, son))
                : utils::fill_array_n<P>(sonDomain, utils::constant(son));
        });
        v->set_index(nextIndex);
        v->set_sons(utils::fill_array_n<P>(sonDomain, [=, this, &cofactors](auto const i)
        {
            return this->internal_node(index, utils::fill_array_n<P>(vDomain, [=, &cofactors](auto const j)
            {
                return cofactors[j][i];
            }));
        }));
        v->for_each_son(inc_ref_count);
        utils::for_all(oldSons, std::bind_front(&node_manager::dec_ref_try_gc, this));
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::delete_vertex
        (node_t* const v) -> void
    {
        --nodeCount_;
        pool_.destroy(v);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::dec_ref_try_gc
        (node_t* const v) -> void
    {
        v->dec_ref_count();

        if (v->get_ref_count() > 0 || this->is_leaf_vertex(v))
        {
            return;
        }

        v->for_each_son(std::bind_front(&node_manager::dec_ref_try_gc, this));
        uniqueTables_[v->get_index()].erase(v);
        this->delete_vertex(v);
    }

    template<class Data, degree Degree, domain Domain>
    template<class... Args>
    auto node_manager<Data, Degree, Domain>::new_vertex
        (Args&&... args) -> node_t*
    {
        ++nodeCount_;
        auto const v = pool_.try_create(std::forward<Args>(args)...);
        if (v)
        {
            return v;
        }

        needsGc_ = true;

        return pool_.force_create(std::forward<Args>(args)...);
    }

    template<class Data, degree Degree, domain Domain>
    template<class IndexMapOp>
    auto node_manager<Data, Degree, Domain>::for_each_level
        (IndexMapOp op) -> void
    {
        this->for_each_level_impl<false>(op);
    }

    template<class Data, degree Degree, domain Domain>
    template<class IndexMapOp>
    auto node_manager<Data, Degree, Domain>::for_each_level
        (IndexMapOp op) const -> void
    {
        this->for_each_level_impl<true>(op);
    }

    template<class Data, degree Degree, domain Domain>
    template<bool IsConst, class IndexMapOp>
    auto node_manager<Data, Degree, Domain>::for_each_level_impl
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
                return const_cast<node_manager&>(*this).uniqueTables_;
            }
        }();

        std::for_each_n(std::begin(levelToIndex_), this->get_var_count(), [&](auto const i)
        {
            op(indexToMapRef[i]);
        });
    }
}

#endif