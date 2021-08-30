#ifndef MIX_DD_DIAGRAM_MANAGER_HPP
#define MIX_DD_DIAGRAM_MANAGER_HPP

#include "node_manager.hpp"
#include "diagram.hpp"
#include "utils.hpp"
#include <concepts>

namespace teddy
{
    template<class Vars>
    concept var_values = requires (Vars vs, index_t i)
    {
        { vs[i] } -> std::convertible_to<uint_t>;
    };

    template<class Data, degree Degree, domain Domain>
    class diagram_manager
    {
    public:
        using diagram_t = diagram<Data, Degree>;
        using node_t    = typename diagram<Data, Degree>::node_t;

    public:
        auto constant (uint_t) -> diagram_t;

        auto variable (index_t) -> diagram_t;

        template<bin_op Op>
        auto apply (diagram_t const&, diagram_t const&) -> diagram_t;

        template<var_values Vars>
        auto evaluate (diagram_t const&, Vars const&) const -> uint_t;

        auto node_count () const -> std::size_t;

        auto node_count (diagram_t const&) const -> std::size_t;

        auto to_dot_graph (std::ostream&) const -> void;

        auto to_dot_graph (std::ostream&, diagram_t const&) const -> void;

    protected:
        diagram_manager ( std::size_t vars
                        , std::size_t nodes
                        , std::vector<index_t> order )
                        requires(domains::is_fixed<Domain>()());

        diagram_manager ( std::size_t vars
                        , std::size_t nodes
                        , domains::mixed
                        , std::vector<index_t> order )
                        requires(domains::is_mixed<Domain>()());

    private:
        template<class ForEachNode>
        auto to_dot_graph_common (std::ostream&, ForEachNode&&) const -> void;

        template<class NodeOp>
        auto traverse_pre (diagram_t const&, NodeOp&&) const -> void;

    public:
        diagram_manager (diagram_manager const&) = delete;
        diagram_manager (diagram_manager&&)      = default;
        auto operator=  (diagram_manager const&) -> diagram_manager& = delete;
        auto operator=  (diagram_manager&&)      -> diagram_manager& = default;


    private:
        node_manager<Data, Degree, Domain> nodes_;
    };

    namespace detail
    {
        inline auto default_or_fwd
            (std::size_t const n, std::vector<index_t>& is)
        {
            return is.empty()
                       ? utils::fill_vector(n, utils::identity)
                       : std::vector<index_t>(std::move(is));
        }
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::constant
        (uint_t const v) -> diagram_t
    {
        return diagram_t(nodes_.terminal_node(v));
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::variable
        (index_t const i) -> diagram_t
    {
        return diagram_t(nodes_.internal_node(i, nodes_.make_sons(i,
            [=](auto const v)
        {
            return nodes_.terminal_node(v);
        })));
    }

    template<class Data, degree Degree, domain Domain>
    template<bin_op Op>
    auto diagram_manager<Data, Degree, Domain>::apply
        (diagram_t const& d1, diagram_t const& d2) -> diagram_t
    {
        auto const go = [this](auto&& go_, auto l, auto r)
        {
            auto const cacheIt = nodes_.template cache_find<Op>(l, r);
            if (cacheIt->matches(l, r))
            {
                return cacheIt->result;
            }

            auto const lhsVal = node_value(l);
            auto const rhsVal = node_value(r);
            auto const opVal  = Op()(lhsVal, rhsVal);
            auto u = static_cast<node_t*>(nullptr);

            if (opVal != Nondetermined)
            {
                u = nodes_.terminal_node(opVal);
            }
            else
            {
                auto const lhsLevel = nodes_.get_level(l);
                auto const rhsLevel = nodes_.get_level(r);
                auto const topLevel = std::min(lhsLevel, rhsLevel);
                auto const topNode  = topLevel == lhsLevel ? l : r;
                auto const topIndex = topNode->get_index();
                auto sons = nodes_.make_sons(topIndex, [=, &go_](auto const k)
                {
                    auto const fst = lhsLevel == topLevel ? l->get_son(k) : l;
                    auto const snd = rhsLevel == topLevel ? r->get_son(k) : r;
                    return go_(go_, fst, snd);
                });

                u = nodes_.internal_node(topIndex, std::move(sons));
            }

            nodes_.template cache_put<Op>(cacheIt, l, r, u);
            return u;
        };

        auto const r = go(go, d1.get_root(), d2.get_root());
        auto d       = diagram_t(r);
        nodes_.adjust_sizes();
        return d;
    }

    template<class Data, degree Degree, domain Domain>
    template<var_values Vars>
    auto diagram_manager<Data, Degree, Domain>::evaluate
        (diagram_t const& d, Vars const& vs) const -> uint_t
    {
        auto n = d.get_root();

        while (not n->is_terminal())
        {
            auto const i = n->get_index();
            nodes_.is_valid(n->get_index(), vs[i]);
            n = n->get_son(vs[i]);
        }

        return n->get_value();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::node_count
        () const -> std::size_t
    {
        return nodes_.get_node_count();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::node_count
        (diagram_t const& d) const -> std::size_t
    {
        auto count = 0ul;
        this->traverse_pre(d, [&count](auto const){ ++count; });
        return count;
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::to_dot_graph
        (std::ostream& ost) const -> void
    {
        this->to_dot_graph_common(ost, [this](auto&& f)
        {
            nodes_.for_each_node(f);
        });
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::to_dot_graph
        (std::ostream& ost, diagram_t const& d) const -> void
    {
        this->to_dot_graph_common(ost, [this, &d](auto&& f)
        {
            this->traverse_pre(d, f);
        });
    }

    template<class Data, degree Degree, domain Domain>
    diagram_manager<Data, Degree, Domain>::diagram_manager
        ( std::size_t          vars
        , std::size_t          nodes
        , std::vector<index_t> order )
        requires(domains::is_fixed<Domain>()()) :
        nodes_ ( vars
               , nodes
               , detail::default_or_fwd(vars, order) )
    {
    }

    template<class Data, degree Degree, domain Domain>
    diagram_manager<Data, Degree, Domain>::diagram_manager
        ( std::size_t          vars
        , std::size_t          nodes
        , domains::mixed       ds
        , std::vector<index_t> order )
        requires(domains::is_mixed<Domain>()()) :
        nodes_ ( vars
               , nodes
               , detail::default_or_fwd(vars, order)
               , std::move(ds) )
    {
    }

    template<class Data, degree Degree, domain Domain>
    template<class ForEachNode>
    auto diagram_manager<Data, Degree, Domain>::to_dot_graph_common
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

        auto const output_range = [](auto&& ost, auto&& xs, auto&& sep)
        {
            auto const end = std::end(xs);
            auto it = std::begin(xs);
            while (it != end)
            {
                ost << *it;
                ++it;
                if (it != end)
                {
                    ost << sep;
                }
            }
        };

        auto const levelCount = 1 + nodes_.get_var_count();
        auto labels       = std::vector<std::string>();
        auto rankGroups   = std::vector<std::vector<std::string>>(levelCount);
        auto arcs         = std::vector<std::string>();
        auto squareShapes = std::vector<std::string>();

        for_each_node([&, this](auto const n)
        {
            // Create label.
            auto const level = nodes_.get_level(n);
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
            nodes_.for_each_son(n, [&, k = 0](auto const son) mutable
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
    template<class NodeOp>
    auto diagram_manager<Data, Degree, Domain>::traverse_pre
        (diagram_t const& d, NodeOp&& op) const -> void
    {
        auto const go = [this](auto&& go_, auto const n, auto&& op_)
        {
            n->toggle_marked();
            std::invoke(op_, n);
            if (n->is_internal())
            {
                nodes_.for_each_son(n, [&go_, n, &op_](auto const son)
                {
                    if (n->is_marked() != son->is_marked())
                    {
                        go_(go_, son, op_);
                    }
                });
            }
        };

        go(go, d.get_root(), op);
        go(go, d.get_root(), [](auto const){});
    }
}

#endif