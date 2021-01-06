#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include "operators.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/print.hpp"
#include "../utils/more_math.hpp"
#include "../utils/more_functional.hpp"

#include <queue>
#include <functional>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::vertex_count
        () const -> std::size_t
    {
        return vertexManager_.vertex_count();
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
        return vertexManager_.vertex_count(i);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::to_dot_graph
        (std::ostream& ost) const -> void
    {
        this->to_dot_graph_impl(ost, [this](auto const f)
        {
            vertexManager_.for_each_vertex(f);
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
        this->traverse_post(d, [this, val](auto const v)
        {
            if (vertexManager_.is_leaf(v))
            {
                v->data = vertexManager_.get_vertex_value(v) == val ? 1 : 0;
            }
            else
            {
                v->data = 0;
                auto const vLevel = vertexManager_.get_level(v);
                v->for_each_son([=, this](auto const son)
                {
                    auto const sonLevel = vertexManager_.get_level(son);
                    auto const diffFactor = utils::int_pow(P, sonLevel - vLevel - 1);
                    v->data += son->data * static_cast<double>(diffFactor);
                });
            }
        });

        auto const rootAlpha = static_cast<std::size_t>(d.get_root()->data);
        auto const rootLevel = vertexManager_.get_level(d.get_root());
        return rootAlpha * utils::int_pow(P, rootLevel);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class GetIthVal>
    auto mdd_manager<VertexData, ArcData, P>::evaluate
        (mdd_t const& d, VariableValues const& vars) const -> log_t
    {
        auto const get_var = GetIthVal {};
        auto v = d.get_root();

        while (!vertexManager_.is_leaf(v))
        {
            v = v->get_son(get_var(vars, v->get_index()));
        }

        return vertexManager_.get_vertex_value(v);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class OutputIt, class SetVarVal>
    auto mdd_manager<VertexData, ArcData, P>::satisfy_all
        (log_t const val, mdd_t const& d, OutputIt out) const -> void
    {
        auto xs = VariableValues {};
        this->satisfy_all_step<VariableValues, OutputIt, SetVarVal>(val, 0, d.get_root(), xs, out);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_pre
        (mdd_t const& d, VertexOp&& op) const -> void
    {
        this->traverse_pre_step(d.get_root(), std::forward<VertexOp>(op));
        this->traverse_pre_step(d.get_root(), utils::no_op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_post
        (mdd_t const& d, VertexOp&& op) const -> void
    {
        this->traverse_post_step(d.get_root(), std::forward<VertexOp>(op));
        this->traverse_post_step(d.get_root(), utils::no_op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_level
        (mdd_t const& d, VertexOp&& op) const -> void
    {
        auto cmp = [this](auto const lhs, auto const rhs)
        {
            return vertexManager_.get_level(lhs) > vertexManager_.get_level(rhs);
        };

        using compare_t     = decltype(cmp);
        using vertex_prio_q = std::priority_queue<vertex_t*, vertex_v, compare_t>;

        auto queue = vertex_prio_q(cmp);
        d.get_root()->toggle_mark();
        queue.push(d.get_root());
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

        this->traverse_pre_step(d.get_root(), utils::no_op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexIterator>
    auto mdd_manager<VertexData, ArcData, P>::to_dot_graph_impl
        (std::ostream& ost, VertexIterator for_each_v) const -> void
    {
        using std::to_string;
        using utils::concat;
        using utils::concat_range;
        using utils::EOL;
        using string_v  = std::vector<std::string>;
        using string_vv = std::vector<string_v>;

        auto make_label = [this](auto const v)
        {
            using traits_t = log_val_traits<P>;
            return this->vertexManager_.is_leaf(v)
                    ? traits_t::to_string(this->vertexManager_.get_vertex_value(v))
                    : "x" + to_string(v->get_index());
        };

        auto to_id = [](auto const v)
        {
            return reinterpret_cast<std::uintptr_t>(v);
        };

        auto labels       = string_v();
        auto rankGroups   = string_vv(1 + this->var_count());
        auto arcs         = string_v();
        auto squareShapes = string_v();

        for_each_v([&](auto const v)
        {
            auto const index = v->get_index();
            labels.emplace_back(concat(to_id(v) , " [label = \"" , make_label(v) , "\"];"));
            rankGroups[index].emplace_back(concat(to_id(v) , ";"));
            v->for_each_son_i([&](auto const i, auto const son) mutable
            {
                if constexpr (2 == P)
                {
                    auto const style = 0 == i ? "dashed" : "solid";
                    arcs.emplace_back(concat(to_id(v) , " -> " , to_id(son) , " [style = " , style , "];"));
                }
                else
                {
                    arcs.emplace_back(concat(to_id(v) , " -> " , to_id(son) , " [label = \"" , i , "\"];"));
                }
            });

            if (vertexManager_.is_leaf(v))
            {
                squareShapes.emplace_back(to_string(to_id(v)));
            }
        });

        auto const ranks = utils::map_if(rankGroups, utils::not_empty, [](auto const& level)
        {
            return concat("{ rank = same; " , concat_range(level, " "), " }");
        });

        ost << "digraph DD {"                                                         << EOL
            << "    node [shape = square] " << concat_range(squareShapes, " ") << ';' << EOL
            << "    node [shape = circle];"                                           << EOL << EOL
            << "    " << concat_range(labels, concat(EOL, "    "))                    << EOL << EOL
            << "    " << concat_range(arcs,   concat(EOL, "    "))                    << EOL << EOL
            << "    " << concat_range(ranks,  concat(EOL, "    "))                    << EOL
            << "}"                                                                    << EOL;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::fill_levels
        (mdd_t const& diagram) const -> vertex_vv
    {
        auto levels = vertex_vv(1 + vertexManager_.var_count());

        this->traverse_pre(diagram, [&, this](auto const v)
        {
            levels[vertexManager_.get_level(v)].emplace_back(v);
        });

        return levels;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class OutputIt, class SetVarVal>
    auto mdd_manager<VertexData, ArcData, P>::satisfy_all_step
        ( log_t const val, level_t const l, vertex_t* const v
        , VariableValues& xs, OutputIt& out ) const -> void
    {
        auto const vertexVal = vertexManager_.get_vertex_value(v);

        if (vertexManager_.is_leaf(v) && val != vertexVal)
        {
            return;
        }
        else if (vertexManager_.is_leaf(v) && val == vertexVal)
        {
            *out++ = xs;
            return;
        }
        else
        {
            auto const goDown = vertexManager_.get_level(v) <= l;
            v->for_each_son_i([=, this, &out, &xs](auto const iSon, auto const son)
            {
                auto const next = goDown ? son : v;
                SetVarVal {} (xs, vertexManager_.get_index(l), iSon);
                satisfy_all_step<VariableValues, OutputIt, SetVarVal>(val, l + 1, next, xs, out);
            });
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_pre_step
        (vertex_t* const v, VertexOp&& op) const -> void
    {
        v->toggle_mark();
        op(v);
        v->for_each_son([this, &op, v](auto const son)
        {
            if (v->get_mark() != son->get_mark())
            {
                this->traverse_pre_step(son, op);
            }
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_post_step
        (vertex_t* const v, VertexOp&& op) const -> void
    {
        v->toggle_mark();
        v->for_each_son([this, &op, v](auto const son)
        {
            if (v->get_mark() != son->get_mark())
            {
                this->traverse_post_step(son, op);
            }
        });
        op(v);
    }
}