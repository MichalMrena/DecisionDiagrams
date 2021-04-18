#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include "operators.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/more_math.hpp"
#include "../utils/less_functional.hpp"

#include <queue>
#include <functional>

namespace teddy
{
    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::vertex_count
        () const -> std::size_t
    {
        return manager_.get_vertex_count();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::vertex_count
        (mdd_t const& diagram) const -> std::size_t
    {
        auto count = 0u;
        this->traverse_pre(diagram, [&count](auto){ ++count; });
        return count;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::vertex_count
        (index_t const i) const -> std::size_t
    {
        return manager_.get_vertex_count(i);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::to_dot_graph
        (std::ostream& ost) const -> void
    {
        this->to_dot_graph_impl(ost, [this](auto const f)
        {
            manager_.for_each_vertex(f);
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::to_dot_graph
        (std::ostream& ost, mdd_t const& diagram) const -> void
    {
        this->to_dot_graph_impl(ost, [this, &diagram](auto const f)
        {
            this->traverse_pre(diagram, f);
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::satisfy_count
        (log_t const val, mdd_t& d) -> std::size_t
    {
        auto constexpr CanUseDataMember = std::is_floating_point_v<VertexData> || std::is_integral_v<VertexData>;
        using T = std::conditional_t<CanUseDataMember, VertexData, std::size_t>;

        // Returns reference to a data associated with given vertex.
        auto data = []()
        {
            if constexpr (CanUseDataMember)
            {
                // Simply return reference to data member.
                return [](auto const v) mutable -> decltype(auto)
                {
                    return (v->data);
                };
            }
            else
            {
                // Return reference to a data that is stored int the map.
                return [map = std::unordered_map<vertex_t*, T>()](auto const v) mutable -> T&
                {
                    return map[v];
                };
            }
        }();

        // Actual satisfy count algorithm.
        this->traverse_post(d, [this, val, &data](auto const v) mutable
        {
            if (manager_.is_leaf_vertex(v))
            {
                data(v) = manager_.get_vertex_value(v) == val ? 1 : 0;
            }
            else
            {
                data(v) = 0;
                auto const vLevel = manager_.get_vertex_level(v);
                v->for_each_son([=, this, &data](auto const son) mutable
                {
                    auto const sonLevel   = manager_.get_vertex_level(son);
                    auto const diffFactor = this->domain_product(vLevel + 1, sonLevel);
                    data(v) += data(son) * static_cast<T>(diffFactor);
                });
            }
        });

        auto const rootAlpha = static_cast<std::size_t>(data(d.get_root()));
        auto const rootLevel = manager_.get_vertex_level(d.get_root());
        return rootAlpha * this->domain_product(0, rootLevel);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dependency_set
        (mdd_t const& d) -> std::vector<index_t>
    {
        auto set = utils::vector<index_t>(manager_.get_var_count());
        auto is  = std::vector<index_t>(manager_.get_var_count(), false);

        this->traverse_pre(d, [&](auto const v)
        {
            auto const i = v->get_index();
            if (!manager_.is_leaf_index(i) && !is[i])
            {
                set.push_back(i);
                is[i] = true;
            }
        });

        set.shrink_to_fit();
        return set;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class GetIthVar>
    auto mdd_manager<VertexData, ArcData, P>::evaluate
        (mdd_t const& d, VariableValues const& vs) const -> log_t
    {
        static auto constexpr get_var = GetIthVar {};
        auto v = d.get_root();

        while (!manager_.is_leaf_vertex(v))
        {
            v = v->get_son(get_var(vs, v->get_index()));
        }

        return manager_.get_vertex_value(v);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class SetIthVar>
    auto mdd_manager<VertexData, ArcData, P>::satisfy_all
        (log_t const val, mdd_t const& d) const -> std::vector<VariableValues>
    {
        auto vals = std::vector<VariableValues>();
        this->satisfy_all_g<VariableValues>(val, d, std::back_inserter(vals));
        return vals;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class OutputIt, class SetIthVar>
    auto mdd_manager<VertexData, ArcData, P>::satisfy_all_g
        (log_t const val, mdd_t const& d, OutputIt out) const -> void
    {
        auto xs = VariableValues {};

        auto go = [=, this, &xs](auto&& go, auto const l, auto const v) mutable
        {
            auto const vertexValue = manager_.get_vertex_value(v);
            auto const vertexLevel = manager_.get_vertex_level(v);

            if (manager_.is_leaf_vertex(v) && val != vertexValue)
            {
                return;
            }
            else if (manager_.is_leaf_level(l) && val == vertexValue)
            {
                *out++ = xs;
                return;
            }
            else if (vertexLevel > l)
            {
                auto const index  = manager_.get_index(l);
                auto const domain = manager_.get_domain(index);
                for (auto iv = 0u; iv < domain; ++iv)
                {
                    SetIthVar {} (xs, index, iv);
                    go(go, l + 1, v);
                }
            }
            else
            {
                auto const index = v->get_index();
                v->for_each_son_i([=, &xs](auto const iv, auto const son) mutable
                {
                    SetIthVar {} (xs, index, iv);
                    go(go, l + 1, son);
                });
            }
        };

        go(go, level_t(0), d.get_root());
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_pre
        (mdd_t const& d, VertexOp&& op) const -> void
    {
        auto const go = [](auto&& go, auto const v, auto&& op)
        {
            v->toggle_mark();
            op(v);
            v->for_each_son([&go, v, &op](auto const son)
            {
                if (v->get_mark() != son->get_mark())
                {
                    go(go, son, op);
                }
            });
        };

        go(go, d.get_root(), op);
        go(go, d.get_root(), utils::no_op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_post
        (mdd_t const& d, VertexOp&& op) const -> void
    {
        auto const go = [](auto&& go, auto const v, auto&& op)
        {
            v->toggle_mark();
            v->for_each_son([&go, v, &op](auto const son)
            {
                if (v->get_mark() != son->get_mark())
                {
                    go(go, son, op);
                }
            });
            op(v);
        };

        go(go, d.get_root(), op);
        go(go, d.get_root(), utils::no_op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_level
        (mdd_t const& d, VertexOp&& op) const -> void
    {
        auto const go_level = [this](auto const v, auto&& op)
        {
            auto const cmp = [this](auto const l, auto const r)
            {
                return manager_.get_vertex_level(l) > manager_.get_vertex_level(r);
            };

            using compare_t     = decltype(cmp);
            using vertex_prio_q = std::priority_queue<vertex_t*, vertex_v, compare_t>;

            auto queue = vertex_prio_q(cmp);
            v->toggle_mark();
            queue.push(v);
            while (!queue.empty())
            {
                auto const current = queue.top();
                queue.pop();
                op(current);
                current->for_each_son([&queue, current](auto const son)
                {
                    if (son->get_mark() != current->get_mark())
                    {
                        queue.push(son);
                        son->toggle_mark();
                    }
                });
            }
        };

        auto const go_pre = [](auto&& go, auto const v, auto&& op)
        {
            v->toggle_mark();
            op(v);
            v->for_each_son([&go, v, &op](auto const son)
            {
                if (v->get_mark() != son->get_mark())
                {
                    go(go, son, op);
                }
            });
        };

        go_level(d.get_root(), op);
        go_pre(go_pre, d.get_root(), utils::no_op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexIterator>
    auto mdd_manager<VertexData, ArcData, P>::to_dot_graph_impl
        (std::ostream& ost, VertexIterator for_each_v) const -> void
    {
        using std::to_string;

        auto const make_label = [this](auto const v)
        {
            return manager_.is_leaf_vertex(v)
                       ? log_val_traits<P>::to_string(manager_.get_vertex_value(v))
                       : "x" + to_string(v->get_index());
        };

        auto labels       = std::vector<std::string>();
        auto rankGroups   = std::vector<std::vector<std::string>>(1 + manager_.get_var_count());
        auto arcs         = std::vector<std::string>();
        auto squareShapes = std::vector<std::string>();

        for_each_v([&](auto const v)
        {
            auto const index = v->get_index();
            labels.emplace_back(utils::concat(v->get_id() , " [label = \"" , make_label(v) , "\"];"));
            rankGroups[index].emplace_back(utils::concat(v->get_id() , ";"));
            v->for_each_son_i([&](auto const i, auto const son)
            {
                if constexpr (2 == P)
                {
                    auto const style = 0 == i ? "dashed" : "solid";
                    arcs.emplace_back(utils::concat(v->get_id() , " -> " , son->get_id() , " [style = " , style , "];"));
                }
                else
                {
                    arcs.emplace_back(utils::concat(v->get_id() , " -> " , son->get_id() , " [label = \"" , i , "\"];"));
                }
            });

            if (manager_.is_leaf_vertex(v))
            {
                squareShapes.emplace_back(to_string(v->get_id()));
            }
        });

        auto const ranks = utils::filter_fmap(rankGroups, utils::not_empty, [](auto const& level)
        {
            return utils::concat("{ rank = same; " , utils::concat_range(level, " "), " }");
        });

        ost << "digraph DD {"                                                         << '\n'
            << "    node [shape = square] " << utils::concat_range(squareShapes, " ") << ';'  << '\n'
            << "    node [shape = circle];"                                           << '\n' << '\n'
            << "    " << utils::concat_range(labels, "\n    ")                        << '\n' << '\n'
            << "    " << utils::concat_range(arcs,   "\n    ")                        << '\n' << '\n'
            << "    " << utils::concat_range(ranks,  "\n    ")                        << '\n'
            << "}"                                                                    << '\n';
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::domain_product
        (level_t const from, level_t const to) const -> std::size_t
    {
        if constexpr (2 == P)
        {
            return utils::two_pow(to - from);
        }
        else
        {
            if (manager_.has_domains())
            {
                auto const get_dom = [this](auto const l)
                {
                    return manager_.get_domain(manager_.get_index(l));
                };
                auto const ls = utils::range(from, to);
                return std::transform_reduce( std::begin(ls), std::end(ls)
                                            , 1u, std::multiplies<>(), get_dom );
            }
            else
            {
                return utils::int_pow(P, to - from);
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::fill_levels
        (mdd_t const& diagram) const -> vertex_vv
    {
        auto levels = std::vector<std::vector<vertex_t*>>(1 + manager_.get_var_count());

        this->traverse_pre(diagram, [&, this](auto const v)
        {
            levels[manager_.get_vertex_level(v)].emplace_back(v);
        });

        return levels;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class OutputIt, class SetIthVar>
    auto mdd_manager<VertexData, ArcData, P>::satisfy_all_step
        ( log_t const val, level_t const l, vertex_t* const v
        , VariableValues& xs, OutputIt& out ) const -> void
    {
        auto const vertexValue = manager_.get_vertex_value(v);
        auto const vertexLevel = manager_.get_vertex_level(v);

        if (manager_.is_leaf_level(l) && manager_.is_leaf_vertex(v) && val != vertexValue)
        {
            return;
        }
        else if (manager_.is_leaf_level(l) && manager_.is_leaf_vertex(v) && val == vertexValue)
        {
            *out++ = xs;
            return;
        }
        else if (vertexLevel > l)
        {
            auto const index  = manager_.get_index(l);
            auto const domain = manager_.get_domain(index);
            for (auto iv = 0u; iv < domain; ++iv)
            {
                SetIthVar {} (xs, index, iv);
                satisfy_all_step<VariableValues, OutputIt, SetIthVar>(val, l + 1, v, xs, out);
            }
        }
        else
        {
            auto const index = v->get_index();
            v->for_each_son_i([=, this, &out, &xs](auto const iv, auto const son)
            {
                SetIthVar {} (xs, index, iv);
                this->satisfy_all_step<VariableValues, OutputIt, SetIthVar>(val, l + 1, son, xs, out);
            });
        }
    }
}