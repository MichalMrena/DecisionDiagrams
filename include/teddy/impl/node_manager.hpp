#ifndef MIX_DD_NODE_MANAGER_HPP
#define MIX_DD_NODE_MANAGER_HPP

#include "debug.hpp"
#include "hash_tables.hpp"
#include "node.hpp"
#include "node_pool.hpp"
#include "operators.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace teddy
{
    namespace domains
    {
        struct mixed
        {
            std::vector<uint_t> ds_;

            mixed (std::vector<uint_t> ds) :
                ds_ (std::move(ds))
            {
            };

            auto operator[] (uint_t const i) const
            {
                return ds_[i];
            }
        };

        template<uint_t N>
        struct fixed
        {
            static_assert(N > 1);
            constexpr auto operator[] (uint_t const) const
            {
                return N;
            }

            constexpr auto operator() () const
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

    template<class F>
    concept uint_to_bool = requires (F f, uint_t x)
    {
        { f(x) } -> std::convertible_to<bool>;
    };

    template<class F>
    concept uint_to_uint = requires (F f, uint_t x)
    {
        { f(x) } -> std::convertible_to<uint_t>;
    };

    template<class Data, degree Degree, domain Domain>
    class node_manager
    {
    public:
        using node_t      = node<Data, Degree>;
        using sons_t      = typename node_t::sons_t;
        using op_cache_t  = apply_cache<Data, Degree>;
        using op_cache_it = typename op_cache_t::iterator;
        struct common_init {};

    public:
        node_manager ( std::size_t vars
                     , std::size_t nodes
                     , std::vector<index_t> order )
                     requires(domains::is_fixed<Domain>()());

        node_manager ( std::size_t vars
                     , std::size_t nodes
                     , std::vector<index_t> order
                     , domains::mixed )
                     requires(domains::is_mixed<Domain>()());

        node_manager (node_manager&&) noexcept = default;
        node_manager (node_manager const&) = delete;

        auto set_cache_ratio  (std::size_t) -> void;
        auto set_pool_ratio   (std::size_t) -> void;
        auto set_auto_reorder (bool)        -> void;

    private:
        node_manager ( common_init
                     , std::size_t vars
                     , std::size_t nodes
                     , std::vector<index_t> order
                     , Domain );

    public:
        auto get_terminal_node (uint_t) const      -> node_t*;
        auto terminal_node     (uint_t)            -> node_t*;
        auto internal_node     (index_t, sons_t&&) -> node_t*;
        auto get_level         (index_t) const     -> level_t;
        auto get_level         (node_t*) const     -> level_t;
        auto get_leaf_level    ()        const     -> level_t;
        auto get_index         (level_t) const     -> index_t;
        auto get_domain        (index_t) const     -> uint_t;
        auto get_node_count    (index_t) const     -> std::size_t;
        auto get_node_count    (node_t*) const     -> std::size_t;
        auto get_node_count    () const            -> std::size_t;
        auto get_var_count     () const            -> std::size_t;
        auto get_order         () const          -> std::vector<index_t> const&;
        auto get_domains       () const            -> std::vector<uint_t>;
        auto collect_garbage   ()                  -> void;

        auto to_dot_graph (std::ostream&) const -> void;
        auto to_dot_graph (std::ostream&, node_t*) const -> void;

        auto domain_product (level_t, level_t) const -> std::size_t;

        template<utils::i_gen F>
        auto make_sons (index_t, F&&) -> sons_t;

        template<class NodeOp>
        auto for_each_son (node_t*, NodeOp&&) const -> void;

        template<class NodeOp>
        auto for_each_node (NodeOp&&) const -> void;

        template<class Op>
        auto cache_find (node_t*, node_t*) -> op_cache_it;

        template<class Op>
        auto cache_put (op_cache_it, node_t*, node_t*, node_t*) -> void;

        template<class NodeOp>
        auto traverse_pre (node_t*, NodeOp&&) const -> void;

        template<class NodeOp>
        auto traverse_post (node_t*, NodeOp&&) const -> void;

            auto swap_vars        (index_t i)         -> void;
            auto sift_vars        ()                  -> void;

        auto is_valid_var_value (index_t, uint_t) const -> bool;

        static auto inc_ref_count (node_t*) -> void;
        static auto dec_ref_count (node_t*) -> void;

    private:
        using unique_table_t = unique_table<Data, Degree>;
        using unique_table_v = std::vector<unique_table_t>;
        using op_cache_a     = std::array<op_cache_t, OpCount>;

    private:
        auto node_hash    (index_t, sons_t const&) const -> std::size_t;
        auto node_equal   (node_t*, sons_t const&) const -> bool;
        auto is_redundant (index_t, sons_t const&) const -> bool;

        auto adjust_tables () -> void;
        auto adjust_caches () -> void;

        template<class... Args>
        auto new_node (Args&&...) -> node_t*;
        auto delete_node (node_t*) -> void;

        template<class ForEachNode>
        auto to_dot_graph_common (std::ostream&, ForEachNode&&) const -> void;

        static auto check_distinct (std::vector<index_t> const&) -> bool;


            auto swap_vertex    (node_t* v) -> void;
            auto dec_ref_try_gc (node_t* v) -> void;

    private:
        std::array<op_cache_t, OpCount> opCaches_;
        node_pool<Data, Degree>         pool_;
        std::vector<unique_table_t>     uniqueTables_;
        std::vector<node_t*>            terminals_;
        std::vector<level_t>            indexToLevel_;
        std::vector<index_t>            levelToIndex_;
        [[no_unique_address]] Domain    domains_;
        std::size_t                     nodeCount_;
        std::size_t                     cacheRatio_;
        std::size_t                     lastGcNodeCount_;
        std::size_t                     nextTableAdjustment_;
        bool                            reorderEnabled_;
    };

    template<class Data, degree D>
    auto node_value (node<Data, D>* const n) -> uint_t
    {
        return n->is_terminal() ? n->get_value() : Nondetermined;
    }

    template<class Data, degree D>
    auto id_inc_ref_count (node<Data, D>* const n) -> node<Data, D>*
    {
        n->inc_ref_count();
        return n;
    }

    template<class Data, degree D>
    auto id_set_marked (node<Data, D>* const n) -> node<Data, D>*
    {
        n->set_marked();
        return n;
    }

    template<class Data, degree D>
    auto id_set_notmarked (node<Data, D>* const n) -> node<Data, D>*
    {
        n->set_notmarked();
        return n;
    }

    template<class Data, degree Degree, domain Domain>
    node_manager<Data, Degree, Domain>::node_manager
        ( std::size_t const    vars
        , std::size_t const    nodes
        , std::vector<index_t> order )
        requires(domains::is_fixed<Domain>()()) :
        node_manager ( common_init()
                     , vars
                     , nodes
                     , std::move(order)
                     , {} )
    {
    }

    template<class Data, degree Degree, domain Domain>
    node_manager<Data, Degree, Domain>::node_manager
        ( std::size_t const    vars
        , std::size_t const    nodes
        , std::vector<index_t> order
        , domains::mixed       ds )
        requires(domains::is_mixed<Domain>()()) :
        node_manager ( common_init()
                     , vars
                     , nodes
                     , std::move(order)
                     , std::move(ds) )
    {
    }

    template<class Data, degree Degree, domain Domain>
    node_manager<Data, Degree, Domain>::node_manager
        ( common_init
        , std::size_t const    vars
        , std::size_t const    nodes
        , std::vector<index_t> order
        , Domain               ds ) :
        opCaches_            ({}),
        pool_                (nodes),
        uniqueTables_        (vars),
        terminals_           ({}),
        indexToLevel_        (vars),
        levelToIndex_        (std::move(order)),
        domains_             (std::move(ds)),
        nodeCount_           (0),
        cacheRatio_          (4),
        lastGcNodeCount_     (pool_.main_pool_size()),
        nextTableAdjustment_ (230),
        reorderEnabled_      (false)
    {
        assert(levelToIndex_.size() == this->get_var_count());
        assert(check_distinct(levelToIndex_));
        if constexpr ( domains::is_mixed<Domain>()()
                   and degrees::is_fixed<Degree>()() )
        {
            for (auto const d : domains_.ds_)
            {
                assert(d <= Degree()());
            }
        }

        // Create reverse mapping from (level_t -> index_t)
        // to (index_t -> level_t).
        auto level = 0u;
        for (auto const index : levelToIndex_)
        {
            indexToLevel_[index] = level++;
        }
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::set_cache_ratio
        (std::size_t const denom) -> void
    {
        assert(denom > 0);
        cacheRatio_ = denom;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::set_pool_ratio
        (std::size_t const denom) -> void
    {
        assert(denom > 0);
        pool_.set_overflow_ratio(denom);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::set_auto_reorder
        (bool const reorder) -> void
    {
        reorderEnabled_ = reorder;
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
        if constexpr (domains::is_fixed<Domain>()())
        {
            assert(v < Domain()());
        }

        if (v >= terminals_.size())
        {
            terminals_.resize(v + 1, nullptr);
        }

        if (not terminals_[v])
        {
            terminals_[v] = this->new_node(v);
        }

        return id_set_marked(terminals_[v]);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::internal_node
        (index_t const i, sons_t&& sons) -> node_t*
    {
        // všetky čo odtiaľto vylezú by sa mali omarkovať
        // neskôr sa určite stanú synom alebo koreňom, tak budú odmarkované.

        if (this->is_redundant(i, sons))
        {
            return id_set_marked(sons[0]);
        }

        auto const eq = std::bind_front(&node_manager::node_equal, this);

        auto& table         = uniqueTables_[i];
        auto const hash     = this->node_hash(i, sons);
        auto const existing = table.find(sons, hash, eq);
        if (existing)
        {
            this->for_each_son(existing, id_set_notmarked<Data, Degree>);
            return id_set_marked(existing);
        }

        auto n = this->new_node(i, std::move(sons));
        table.insert(n, hash);
        this->for_each_son(n, id_inc_ref_count<Data, Degree>);

        // už je syn niekoho, tak by sa mal dať v pohode odmarkovať
        this->for_each_son(n, id_set_notmarked<Data, Degree>);

        return id_set_marked(n);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_level
        (index_t const i) const -> level_t
    {
        return indexToLevel_[i];
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_level
        (node_t* const n) const -> level_t
    {
        return n->is_terminal()
                   ? this->get_leaf_level()
                   : this->get_level(n->get_index());
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_leaf_level
        () const -> level_t
    {
        return static_cast<level_t>(this->get_var_count());
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_index
        (level_t const l) const -> index_t
    {
        assert(l < levelToIndex_.size());
        return levelToIndex_[l];
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_domain
        (index_t const i) const -> uint_t
    {
        assert(i < this->get_var_count());
        return domains_[i];
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_node_count
        (index_t const i) const -> std::size_t
    {
        assert(i < this->get_var_count());
        return uniqueTables_[i].size();
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_node_count
        (node_t* const n) const -> std::size_t
    {
        auto count = 0ul;
        this->traverse_pre(n, [&count](auto const){ ++count; });
        return count;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_node_count
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
    auto node_manager<Data, Degree, Domain>::get_order
        () const -> std::vector<index_t> const&
    {
        return levelToIndex_;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::get_domains
        () const -> std::vector<uint_t>
    {
        auto ds = std::vector<uint_t>();
        for (auto k = 0u; k < this->get_var_count(); ++k)
        {
            ds.emplace_back(domains_[k]);
        }
        return ds;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::collect_garbage
        () -> void
    {
        debug::out("Collecting garbage. Node count before = ");
        debug::out(nodeCount_);
        debug::out(".");

        for (auto level = 0u; level < this->get_var_count(); ++level)
        {
            auto& table    = uniqueTables_[levelToIndex_[level]];
            auto const end = std::end(table);
            auto it        = std::begin(table);

            while (it != end)
            {
                auto const n = *it;
                if (0 == n->get_ref_count() and not n->is_marked())
                {
                    this->for_each_son(n, dec_ref_count);
                    it = table.erase(it);
                    this->delete_node(n);
                }
                else
                {
                    ++it;
                }
            }
        }

        for (auto& t : terminals_)
        {
            if (t and 0 == t->get_ref_count() and not t->is_marked())
            {
                this->delete_node(t);
                t = nullptr;
            }
        }

        for (auto& cache : opCaches_)
        {
            cache.rm_unused();
        }

        debug::out("Node count after = ");
        debug::out(nodeCount_);
        debug::outl(". ");
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::to_dot_graph
        (std::ostream& ost) const -> void
    {
        this->to_dot_graph_common(ost, [this](auto&& f)
        {
            this->for_each_node(f);
        });
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::to_dot_graph
        (std::ostream& ost, node_t* const n) const -> void
    {
        this->to_dot_graph_common(ost, [this, n](auto&& f)
        {
            this->traverse_pre(n, f);
        });
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::domain_product
        (level_t const from, level_t const to) const -> std::size_t
    {
        if constexpr (domains::is_fixed<Domain>()())
        {
            return utils::int_pow(Domain()(), to - from);
        }
        else
        {
            auto product = 1u;
            for (auto l = from; l < to; ++ l)
            {
                product *= domains_[levelToIndex_[l]];
            }
            return product;
        }
    }

    template<class Data, degree Degree, domain Domain>
    template<utils::i_gen F>
    auto node_manager<Data, Degree, Domain>::make_sons
        (index_t const i, F&& f) -> sons_t
    {
        auto ss = node_t::container(domains_[i], Degree());
        for (auto k = uint_t {0}; k < domains_[i]; ++k)
        {
            ss[k] = std::invoke(f, k);
        }
        return ss;
    }

    template<class Data, degree Degree, domain Domain>
    template<class NodeOp>
    auto node_manager<Data, Degree, Domain>::for_each_son
        (node_t* const node, NodeOp&& f) const -> void
    {
        auto const i = node->get_index();
        for (auto k = 0u; k < domains_[i]; ++k)
        {
            std::invoke(f, node->get_son(k));
        }
    }

    template<class Data, degree Degree, domain Domain>
    template<class NodeOp>
    auto node_manager<Data, Degree, Domain>::for_each_node
        (NodeOp&& f) const -> void
    {
        for (auto const& table : uniqueTables_)
        {
            for (auto const n : table)
            {
                std::invoke(f, n);
            }
        }

        for (auto const n : terminals_)
        {
            if (n)
            {
                std::invoke(f, n);
            }
        }
    }

    template<class Data, degree Degree, domain Domain>
    template<class Op>
    auto node_manager<Data, Degree, Domain>::cache_find
        (node_t* const l, node_t* const r) -> op_cache_it
    {
        // TODO return pair (node, iterator)
        auto& cache = opCaches_[op_id(Op())];
        auto const ret = cache.find(l, r);
        if (ret->matches(l, r))
        {
            id_set_marked(ret->result);
        }
        return ret;
    }

    template<class Data, degree Degree, domain Domain>
    template<class Op>
    auto node_manager<Data, Degree, Domain>::cache_put
        ( op_cache_it it
        , node_t* const l
        , node_t* const r
        , node_t* const res ) -> void
    {
        auto& cache = opCaches_[op_id(Op())];
        cache.put(it, l, r, res);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::is_valid_var_value
        (index_t const i, uint_t const v) const -> bool
    {
        return v < domains_[i];
    }

    template<class Data, degree Degree, domain Domain>
    template<class NodeOp>
    auto node_manager<Data, Degree, Domain>::traverse_pre
        (node_t* const node, NodeOp&& op) const -> void
    {
        auto const go = [this](auto&& go_, auto const n, auto&& op_)
        {
            n->toggle_marked();
            std::invoke(op_, n);
            if (n->is_internal())
            {
                this->for_each_son(n, [&go_, n, &op_](auto const son)
                {
                    if (n->is_marked() != son->is_marked())
                    {
                        go_(go_, son, op_);
                    }
                });
            }
        };

        go(go, node, op);
        go(go, node, [](auto const){});
    }

    template<class Data, degree Degree, domain Domain>
    template<class NodeOp>
    auto node_manager<Data, Degree, Domain>::traverse_post
        (node_t* const node, NodeOp&& op) const -> void
    {
        auto const go = [this](auto&& go_, auto const n, auto&& op_)
        {
            n->toggle_marked();
            if (n->is_internal())
            {
                this->for_each_son(n, [&go_, n, &op_](auto const son)
                {
                    if (n->is_marked() != son->is_marked())
                    {
                        go_(go_, son, op_);
                    }
                });
            }
            std::invoke(op_, n);
        };

        go(go, node, op);
        go(go, node, [](auto const){});
    }

















    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::inc_ref_count
        (node_t* const v) -> void
    {
        v->inc_ref_count();
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::dec_ref_count
        (node_t* const v) -> void
    {
        v->dec_ref_count();
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::node_hash
        (index_t const i, sons_t const& ss) const -> std::size_t
    {
        auto result = std::size_t(0);
        for (auto k = 0u; k < domains_[i]; ++k)
        {
            auto const hash = std::hash<node_t*>()(ss[k]);
            result ^= hash + 0x9e3779b9 + (result << 6) + (result >> 2);
        }
        return result;
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::node_equal
        (node_t* const n, sons_t const& ss) const -> bool
    {
        auto const i = n->get_index();
        for (auto k = 0u; k < domains_[i]; ++k)
        {
            if (n->get_son(k) != ss[k])
            {
                return false;
            }
        }
        return true;
    }

    template<class Data, degree Degree, domain Domain>
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
    auto node_manager<Data, Degree, Domain>::adjust_tables
        () -> void
    {
        auto const hash = std::bind_front(&node_manager::node_hash, this);
        for (auto& t : uniqueTables_)
        {
            t.adjust_capacity(hash);
        }
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::adjust_caches
        () -> void
    {
        for (auto& c : opCaches_)
        {
            c.adjust_capacity(nodeCount_ / cacheRatio_);
        }
    }

    template<class Data, degree Degree, domain Domain>
    template<class... Args>
    auto node_manager<Data, Degree, Domain>::new_node
        (Args&&... args) -> node_t*
    {
        auto constexpr as_double = [](auto const x)
        {
            return static_cast<double>(x);
        };

        ++nodeCount_;
        if (pool_.available_nodes() == 0)
        {
            // TODO tento magic number 0.2 dobre premyslieť
            auto const dogc = as_double(lastGcNodeCount_)
                            > 0.2 * as_double(pool_.main_pool_size());
            if (dogc)
            {
                auto const beforeGc = pool_.available_nodes();
                this->collect_garbage();
                if (reorderEnabled_)
                {
                    this->sift_vars();
                }
                auto const afterGc = pool_.available_nodes();
                lastGcNodeCount_ = afterGc - beforeGc;
                if (lastGcNodeCount_ == 0)
                {
                    pool_.grow();
                }
            }
            else
            {
                pool_.grow();
            }
        }

        if (nodeCount_ >= nextTableAdjustment_)
        {
            assert(nodeCount_ == nextTableAdjustment_);
            this->adjust_tables();
            this->adjust_caches(); // TODO otazka je ci toto neriesit samostatne, skontrolovat ake su tie load factory
            nextTableAdjustment_ *= 2;
        }

        return pool_.create(std::forward<Args>(args)...);
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::delete_node
        (node_t* const n) -> void
    {
        --nodeCount_;
        n->set_unused();
        pool_.destroy(n);
    }

    template<class Data, degree Degree, domain Domain>
    template<class ForEachNode>
    auto node_manager<Data, Degree, Domain>::to_dot_graph_common
        (std::ostream& ost, ForEachNode&& for_each_node) const -> void
    {
        auto const make_label = [](auto const n)
        {
            return n->is_terminal()
                       ? std::to_string(n->get_value())
                       : "x" + std::to_string(n->get_index());
        };

        auto const get_id_str = [](auto const n)
        {
            return std::to_string(reinterpret_cast<std::uintptr_t>(n));
        };

        auto const output_range = [](auto&& ostr, auto&& xs, auto&& sep)
        {
            auto const end = std::end(xs);
            auto it = std::begin(xs);
            while (it != end)
            {
                ostr << *it;
                ++it;
                if (it != end)
                {
                    ostr << sep;
                }
            }
        };

        auto const levelCount = 1 + this->get_var_count();
        auto labels       = std::vector<std::string>();
        auto rankGroups   = std::vector<std::vector<std::string>>(levelCount);
        auto arcs         = std::vector<std::string>();
        auto squareShapes = std::vector<std::string>();

        for_each_node([&, this](auto const n)
        {
            // Create label.
            auto const level = this->get_level(n);
            labels.emplace_back( get_id_str(n)
                               + R"( [label = ")"
                               + make_label(n)
                               + R"("];)" );

            if (n->is_terminal())
            {
                squareShapes.emplace_back(get_id_str(n));
                rankGroups.back().emplace_back(get_id_str(n) + ";");
                return;
            }

            // Add to same level.
            rankGroups[level].emplace_back(get_id_str(n) + ";");

            // Add arcs.
            this->for_each_son(n, [&, k = 0](auto const son) mutable
            {
                if constexpr (std::is_same_v<Degree, degrees::fixed<2>>)
                {
                    arcs.emplace_back( get_id_str(n)
                                     + " -> "
                                     + get_id_str(son)
                                     + " [style = "
                                     + (0 == k ? "dashed" : "solid")
                                     + "];" );
                }
                else
                {
                    arcs.emplace_back( get_id_str(n)
                                     + " -> "
                                     + get_id_str(son)
                                     + R"( [label = )"
                                     + std::to_string(k)
                                     + "];" );
                }
                ++k;
            });
        });

        // Finally, output everything into the output stream.
        ost << "digraph DD {" << '\n';
        ost << "    node [shape = square] ";
        output_range(ost, squareShapes, " ");
        ost << ";\n";
        ost << "    node [shape = circle];" << "\n\n";

        ost << "    ";
        output_range(ost, labels, "\n    ");
        ost << "\n\n";
        ost << "    ";
        output_range(ost, arcs, "\n    ");
        ost << "\n\n";

        for (auto const& rs : rankGroups)
        {
            if (not rs.empty())
            {
                ost << "    { rank = same; ";
                output_range(ost, rs, " ");
                ost << " }" << '\n';
            }
        }
        ost << '\n';
        ost << "}" << '\n';
    }

    template<class Data, degree Degree, domain Domain>
    auto node_manager<Data, Degree, Domain>::check_distinct
        (std::vector<index_t> const& is) -> bool
    {
        if (is.empty())
        {
            return true;
        }
        auto const me = std::ranges::max(is);
        auto in = std::vector<bool>(me + 1, false);
        for (auto const i : is)
        {
            if (in[i])
            {
                return false;
            }
            in[i] = true;
        }
        return true;
    }














            template<class Data, degree Degree, domain Domain>
            auto node_manager<Data, Degree, Domain>::swap_vertex
                (node_t* const) -> void
            {
                // auto const index     = v->get_index();
                // auto const nextIndex = this->get_index(1 + this->get_vertex_level(v));
                // auto const vDomain   = this->get_domain(index);
                // auto const sonDomain = this->get_domain(nextIndex);
                // auto const oldSons   = utils::fill_vector(vDomain, std::bind_front(&node_t::get_son, v));
                // auto const cofactors = utils::fill_array_n<P>(vDomain, [=](auto const sonIndex)
                // {
                //     auto const son = v->get_son(sonIndex);
                //     return son->get_index() == nextIndex
                //         ? utils::fill_array_n<P>(sonDomain, std::bind_front(&node_t::get_son, son))
                //         : utils::fill_array_n<P>(sonDomain, utils::constant(son));
                // });
                // v->set_index(nextIndex);
                // v->set_sons(utils::fill_array_n<P>(sonDomain, [=, this, &cofactors](auto const i)
                // {
                //     return this->internal_node(index, utils::fill_array_n<P>(vDomain, [=, &cofactors](auto const j)
                //     {
                //         return cofactors[j][i];
                //     }));
                // }));
                // v->for_each_son(inc_ref_count);
                // utils::for_all(oldSons, std::bind_front(&node_manager::dec_ref_try_gc, this));
            }


            template<class Data, degree Degree, domain Domain>
            auto node_manager<Data, Degree, Domain>::dec_ref_try_gc
                (node_t* const) -> void
            {
                // v->dec_ref_count();

                // if (v->get_ref_count() > 0 || this->is_leaf_vertex(v))
                // {
                //     return;
                // }

                // v->for_each_son(std::bind_front(&node_manager::dec_ref_try_gc, this));
                // uniqueTables_[v->get_index()].erase(v);
                // this->delete_node(v);
            }



        template<class Data, degree Degree, domain Domain>
        auto node_manager<Data, Degree, Domain>::swap_vars
            (index_t const) -> void
        {
            // auto const iLevel    = this->get_level(i);
            // auto const nextIndex = this->get_index(1 + iLevel);
            // auto tmpTable        = unique_table_t(std::move(uniqueTables_[i]));
            // for (auto const v : tmpTable)
            // {
            //     this->swap_vertex(v);
            // }
            // uniqueTables_[i].adjust_capacity();
            // uniqueTables_[nextIndex].merge(tmpTable);

            // std::swap(levelToIndex_[iLevel], levelToIndex_[1 + iLevel]);
            // ++indexToLevel_[i];
            // --indexToLevel_[nextIndex];
        }

        template<class Data, degree Degree, domain Domain>
        auto node_manager<Data, Degree, Domain>::sift_vars
            () -> void
        {
            // using count_pair = struct { index_t index; std::size_t count; };

            // auto const get_sift_order = [this]()
            // {
            //     auto counts = utils::fill_vector(this->get_var_count(), [this](auto const i)
            //     {
            //         return count_pair {i, this->get_node_count(i)};
            //     });
            //     std::sort(std::begin(counts), std::end(counts), [](auto&& l, auto&& r){ return l.count > r.count; });
            //     return counts;
            // };

            // auto const move_var_down = [this](auto const index)
            // {
            //     this->swap_vars(index);
            // };

            // auto const move_var_up = [this](auto const index)
            // {
            //     auto const level     = this->get_level(index);
            //     auto const prevIndex = this->get_index(level - 1);
            //     this->swap_vars(prevIndex);
            // };

            // auto const place_variable = [&, this](auto const index)
            // {
            //     auto level        = this->get_level(index);
            //     auto optimalLevel = level;
            //     auto optimalCount = nodeCount_;

            //     // Sift down.
            //     while (level != this->get_last_internal_level())
            //     {
            //         move_var_down(index);
            //         ++level;
            //         if (nodeCount_ < optimalCount)
            //         {
            //             optimalCount = nodeCount_;
            //             optimalLevel = level;
            //         }
            //     }

            //     // Sift up.
            //     while (level != 0)
            //     {
            //         move_var_up(index);
            //         --level;
            //         if (nodeCount_ < optimalCount)
            //         {
            //             optimalCount = nodeCount_;
            //             optimalLevel = level;
            //         }
            //     }

            //     // Restore optimal position.
            //     while (level != optimalLevel)
            //     {
            //         move_var_down(index);
            //         ++level;
            //     }
            // };

            // auto const siftOrder = get_sift_order();
            // for (auto pair : siftOrder)
            // {
            //     place_variable(pair.index);
            // }
        }
}

#endif