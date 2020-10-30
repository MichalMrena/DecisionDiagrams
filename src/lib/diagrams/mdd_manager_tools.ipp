#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include "operators.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/print.hpp"

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
        auto is = utils::range(0u, manager_.get_var_count());
        this->to_dot_graph_impl(ost, utils::map(is, [this](auto const i)
        {
            return this->manager_.get_level_iterators(i);
        }));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::to_dot_graph
        (std::ostream& ost, mdd_t const& diagram) const -> void
    {
        auto levels = this->fill_levels(diagram);
        this->to_dot_graph_impl(ost, utils::map(std::begin(levels), std::prev(std::end(levels)),
            [](auto&& level)
        {
            return std::make_pair(std::begin(level), std::end(level));
        }));
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_pre
        (mdd_t const& diagram, VertexOp&& op) const -> void
    {
        this->traverse_pre(diagram.get_root(), std::forward<VertexOp>(op));
        this->traverse_pre(diagram.get_root(), utils::no_op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_manager<VertexData, ArcData, P>::traverse_level
        (mdd_t const& diagram, VertexOp&& op) const -> void
    {
        auto cmp = [this](auto const lhs, auto const rhs)
        {
            return this->manager_.get_level(lhs) > manager_.get_level(rhs);
        };

        using compare_t     = decltype(cmp);
        using vertex_prio_q = std::priority_queue<vertex_t*, vertex_v, compare_t>;

        auto queue = vertex_prio_q(std::move(cmp));
        while (!queue.empty())
        {
            auto const current = queue.top();
            queue.pop();
            op(current);
            current->toggle_mark();
            for (auto i = log_t {0}; i < P; ++i)
            {
                auto const son = current->get_son(i);
                if (son && son->get_mark() != current->get_mark())
                {
                    queue.push(son);
                }
            }
        }

        this->traverse_pre(diagram.get_root(), utils::no_op);
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
            return this->manager_.is_leaf(v) ? traits_t::to_string(manager_.get_value(v))
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
                labels.emplace_back(concat(to_id(v) , " [label = " , make_label(v) , "];"));
                ranksLocal.emplace_back(concat(to_id(v) , ";"));

                if (!manager_.is_leaf(v))
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
            if (manager_.has_terminal_vertex(i))
            {
                auto const leaf = manager_.get_terminal_vertex(i);
                squareShapes.emplace_back(to_string(to_id(leaf)));
                labels.emplace_back(concat(to_id(leaf) , " [label = " , make_label(leaf) , "];"));
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
        auto levels = vertex_vv(1 + manager_.get_var_count());

        this->traverse_pre(diagram, [&, this](auto const v)
        {
            levels[manager_.get_level(v)].emplace_back(v);
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
}