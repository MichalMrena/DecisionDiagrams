#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include "operators.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/print.hpp"
#include "../utils/more_math.hpp"

#include <queue>

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
        auto is = utils::range(0u, vertexManager_.get_var_count());
        this->to_dot_graph_impl(ost, utils::map(is, [this](auto const i)
        {
            return this->vertexManager_.get_level_iterators(i);
        }));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::to_dot_graph
        (std::ostream& ost, mdd_t const& diagram) const -> void
    {
        auto levels = this->fill_levels(diagram); // TODO fill last level with leaves a print only one terminal if mdd is constant...
        this->to_dot_graph_impl(ost, utils::map(std::begin(levels), std::prev(std::end(levels)),
            [](auto&& level)
        {
            return std::make_pair(std::begin(level), std::end(level));
        }));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::satisfy_count
        (log_t const val, mdd_t& d) -> std::size_t
    {
        this->traverse_post(d, [this, val](auto const v)
        {
            if (this->vertexManager_.is_leaf(v))
            {
                v->data = this->vertexManager_.get_terminal_value(v) == val ? 1 : 0;
            }
            else
            {
                v->data = 0;
                for (auto i = 0u; i < P; ++i)
                {
                    auto const vLevel   = this->vertexManager_.get_level(v);
                    auto const sonLevel = this->vertexManager_.get_level(v->get_son(i));
                    v->data += v->get_son(i)->data * utils::int_pow(P, sonLevel - vLevel - 1);
                }
            }
        });

        auto const rootAlpha = d.get_root()->data;
        auto const rootLevel = vertexManager_.get_level(d.get_root());
        return rootAlpha * utils::int_pow(P, rootLevel);
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
            return this->vertexManager_.get_level(lhs) > vertexManager_.get_level(rhs);
        };

        using compare_t     = decltype(cmp);
        using vertex_prio_q = std::priority_queue<vertex_t*, vertex_v, compare_t>;

        auto queue = vertex_prio_q {std::move(cmp)};
        d.get_root()->toggle_mark();
        queue.push(d.get_root());
        while (!queue.empty())
        {
            auto const current = queue.top();
            queue.pop();
            op(current);
            for (auto i = log_t {0}; i < P; ++i)
            {
                auto const son = current->get_son(i);
                if (son && son->get_mark() != current->get_mark())
                {
                    queue.push(son);
                    son->toggle_mark();
                }
            }
        }

        this->traverse_pre(d.get_root(), utils::no_op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class LevelItPair>
    auto mdd_manager<VertexData, ArcData, P>::to_dot_graph_impl
        (std::ostream& ost, std::vector<LevelItPair> levels) const -> void
    {
        using std::to_string;
        using utils::concat;
        using utils::concat_range;
        using utils::EOL;

        auto labels       = std::vector<std::string>();
        auto arcs         = std::vector<std::string>();
        auto ranks        = std::vector<std::string>();
        auto squareShapes = std::vector<std::string>();
        auto make_label   = [this](auto const v)
        {
            using std::to_string;
            using traits_t = log_val_traits<P>;
            return this->vertexManager_.is_leaf(v) ? traits_t::to_string(vertexManager_.get_terminal_value(v))
                                             : "x" + to_string(v->get_index());
        };
        auto to_id = [](auto const v)
        {
            return reinterpret_cast<std::uintptr_t>(v);
        };

        for (auto [levelIt, levelEnd] : levels)
        {
            auto ranksLocal = std::vector<std::string> {"{rank = same;"};
            while (levelIt != levelEnd)
            {
                auto const v = *levelIt;
                labels.emplace_back(concat(to_id(v) , " [label = \"" , make_label(v) , "\"];"));
                ranksLocal.emplace_back(concat(to_id(v) , ";"));

                if (!vertexManager_.is_leaf(v))
                {
                    for (auto val = 0u; val < P; ++val)
                    {
                        if constexpr (2 == P)
                        {
                            auto const style = 0 == val ? "dashed" : "solid";
                            arcs.emplace_back(concat(to_id(v) , " -> " , to_id(v->get_son(val)) , " [style = " , style , "];"));
                        }
                        else
                        {
                            arcs.emplace_back(concat(to_id(v) , " -> " , to_id(v->get_son(val)) , " [label = \"" , val , "\"];"));
                        }
                    }
                }
                ++levelIt;
            }

            if (ranksLocal.size() > 1)
            {
                ranksLocal.emplace_back("}");
                ranks.emplace_back(concat_range(ranksLocal, " "));
            }
        }

        auto ranksLocal = std::vector<std::string> {"{rank = same;"};
        auto const valCount = log_val_traits<P>::valuecount;
        for (auto i = log_t {0}; i < valCount; ++i)
        {
            if (vertexManager_.has_terminal_vertex(i))
            {
                auto const leaf = vertexManager_.get_terminal_vertex(i);
                squareShapes.emplace_back(to_string(to_id(leaf)));
                labels.emplace_back(concat(to_id(leaf) , " [label = \"" , make_label(leaf) , "\"];"));
                ranksLocal.emplace_back(concat(to_id(leaf) , ";"));
            }
        }
        squareShapes.emplace_back(";");
        if (ranksLocal.size() > 1)
        {
            ranksLocal.emplace_back("}");
            ranks.emplace_back(concat_range(ranksLocal, " "));
        }

        ost << concat( "digraph D {"                                                      , EOL
                     , "    " , "node [shape = square] ", concat_range(squareShapes, " ") , EOL
                     , "    " , "node [shape = circle];"                                  , EOL , EOL
                     , "    " , concat_range(labels, concat(EOL, "    "))                 , EOL , EOL
                     , "    " , concat_range(arcs,   concat(EOL, "    "))                 , EOL , EOL
                     , "    " , concat_range(ranks,  concat(EOL, "    "))                 , EOL
                     , "}"                                                                , EOL );
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