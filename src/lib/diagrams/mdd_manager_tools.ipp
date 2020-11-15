#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include "operators.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/print.hpp"
#include "../utils/more_math.hpp"

#include <queue>
#include <functional>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::vertex_count
        (mdd_t const& diagram) const -> std::size_t
    {
        auto count = 0;
        this->traverse_pre(diagram, [&count](auto){ ++count; });
        return count;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::to_dot_graph
        (std::ostream& ost) const -> void
    {
        this->to_dot_graph_impl(ost, [this](auto const f)
        {
            this->vertexManager_.for_each_vertex(f);
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
        this->traverse_post(d, [=](auto const v)
        {
            if (this->vertexManager_.is_leaf(v))
            {
                v->data = this->vertexManager_.get_terminal_value(v) == val ? 1 : 0;
            }
            else
            {
                v->data = 0;
                auto const vLevel = this->vertexManager_.get_level(v);
                v->for_each_son([=](auto const son)
                {
                    auto const sonLevel = this->vertexManager_.get_level(son);
                    v->data += son->data * utils::int_pow(P, sonLevel - vLevel - 1);
                });
            }
        });

        auto const rootAlpha = d.get_root()->data;
        auto const rootLevel = vertexManager_.get_level(d.get_root());
        return rootAlpha * utils::int_pow(P, rootLevel);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class OutputIt, class SetVarVal>
    auto mdd_manager<VertexData, ArcData, P>::satisfy_all
        (log_t const val, mdd_t const& d, OutputIt out) const -> void
    {
        auto xs = VariableValues {};
        this->satisfy_all_step(val, 0, d.get_root(), xs, out);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_pre
        (mdd_t const& d, VertexOp&& op) const -> void
    {
        this->traverse_pre(d.get_root(), std::forward<VertexOp>(op));
        this->traverse_pre(d.get_root(), utils::no_op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_post
        (mdd_t const& d, VertexOp&& op) const -> void
    {
        this->traverse_post(d.get_root(), std::forward<VertexOp>(op));
        this->traverse_post(d.get_root(), utils::no_op);
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

        this->traverse_pre(d.get_root(), utils::no_op);
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
                    ? traits_t::to_string(this->vertexManager_.get_terminal_value(v))
                    : "x" + to_string(v->get_index());
        };

        auto to_id = [](auto const v)
        {
            return reinterpret_cast<std::uintptr_t>(v);
        };

        auto labels       = string_v();
        auto rankGroups   = string_vv(1 + this->get_var_count());
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

        auto const ranks = utils::map(rankGroups, [](auto const& level)
        {
            return concat("{ rank = same;" , concat_range(level, " "), " }");
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
        auto levels = vertex_vv(1 + vertexManager_.get_var_count());

        this->traverse_pre(diagram, [&, this](auto const v)
        {
            levels[vertexManager_.get_level(v)].emplace_back(v);
        });

        return levels;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class OutputIt, class SetVarVal>
    auto mdd_manager<VertexData, ArcData, P>::satisfy_all_step
        ( log_t const val, index_t const cLevel, vertex_t* const v
        , VariableValues& xs, OutputIt& out ) const -> void
    {
        auto const set_var   = SetVarVal {};
        auto const vertexVal = vertexManager_.get_terminal_value(v);
        auto const vLevel    = vertexManager_.get_level(v);

        if (vertexManager_.is_leaf(v) && val != vertexVal)
        {
            return;
        }
        else if (vertexManager_.is_leaf(v) && val == vertexVal)
        {
            *out++ = xs;
            return;
        }
        else if (vLevel > cLevel)
        {
            for (auto i = 0u; i < P; ++i)
            {
                set_var(xs, v->get_index(), i);
                satisfy_all_step(val, cLevel + 1, v, xs, out);
            }
        }
        else
        {
            for (auto i = 0u; i < P; ++i)
            {
                set_var(xs, v->get_index(), i);
                satisfy_all_step(val, cLevel + 1, v->get_son(i), xs, out);
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_pre
        (vertex_t* const v, VertexOp&& op) const -> void
    {
        v->toggle_mark();
        op(v);

        for (auto i = 0u; i < P; ++i)
        {
            auto const son = v->get_son(i);
            if (son && v->get_mark() != son->get_mark())
            {
                this->traverse_pre(son, std::forward<VertexOp>(op));
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_post
        (vertex_t* const v, VertexOp&& op) const -> void
    {
        v->toggle_mark();

        for (auto i = 0u; i < P; ++i)
        {
            auto const son = v->get_son(i);
            if (son && v->get_mark() != son->get_mark())
            {
                this->traverse_post(son, std::forward<VertexOp>(op));
            }
        }

        op(v);
    }
}