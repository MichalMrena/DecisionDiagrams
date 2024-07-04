#ifndef LIBTEDDY_DETAILS_IO_HPP
#define LIBTEDDY_DETAILS_IO_HPP

#include <libteddy/details/diagram_manager.hpp>

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace teddy::details
{
struct io_impl
{
    template<class Data, class Degree, class Domain, class ForEachNode>
    static auto to_dot_graph_common (
        diagram_manager<Data, Degree, Domain> const& manager,
        std::ostream& ost,
        ForEachNode forEach
    ) -> void
    {
        using manager_t       = diagram_manager<Data, Degree, Domain>;
        using node_t          = typename manager_t::node_t;

        auto const make_label = [] (node_t* const node)
        {
            if (node->is_terminal())
            {
                using namespace std::literals::string_literals;
                int32 const val = node->get_value();
                return val == Undefined ? "*"s : std::to_string(val);
            }

            return "x" + std::to_string(node->get_index());
        };

        auto const get_id_str = [] (node_t* const n) {
            return std::to_string(reinterpret_cast<std::intptr_t>(n));
        }; // NOLINT

        auto const output_range
            = [] (auto& ostr, auto const& range, auto const sep)
        {
            auto const endIt = end(range);
            auto rangeIt     = begin(range);
            while (rangeIt != endIt)
            {
                ostr << *rangeIt;
                ++rangeIt;
                if (rangeIt != endIt)
                {
                    ostr << sep;
                }
            }
        };

        auto const levelCount = as_usize(1 + manager.get_var_count());
        std::vector<std::string> labels;
        std::vector<std::vector<std::string>> rankGroups(levelCount);
        std::vector<std::string> arcs;
        std::vector<std::string> squareShapes;

        forEach(
            [&] (node_t* const node)
            {
                // Create label.
                int32 const level = manager.nodes_.get_level(node);
                labels.emplace_back(
                    get_id_str(node) + R"( [label = ")" + make_label(node)
                    + R"("];)"
                );

                if (node->is_terminal())
                {
                    squareShapes.emplace_back(get_id_str(node));
                    rankGroups.back().emplace_back(get_id_str(node) + ";");
                    return;
                }

                // Add to same level.
                rankGroups[as_uindex(level)].emplace_back(
                    get_id_str(node) + ";"
                );

                // Add arcs.
                int32 const domain = manager.nodes_.get_domain(node);
                for (int32 k = 0; k < domain; ++k)
                {
                    node_t* const son = node->get_son(k);
                    if constexpr (std::is_same_v<Degree, degrees::fixed<2>>)
                    {
                        arcs.emplace_back(
                            get_id_str(node) + " -> " + get_id_str(son)
                            + " [style = " + (0 == k ? "dashed" : "solid")
                            + "];"
                        );
                    }
                    else
                    {
                        arcs.emplace_back(
                            get_id_str(node) + " -> " + get_id_str(son)
                            + R"( [label = )" + std::to_string(k) + "];"
                        );
                    }
                }
            }
        );

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

        for (std::vector<std::string> const& ranks : rankGroups)
        {
            if (not ranks.empty())
            {
                ost << "    { rank = same; ";
                output_range(ost, ranks, " ");
                ost << " }" << '\n';
            }
        }
        ost << '\n';
        ost << "}" << '\n';
    }
};
} // namespace teddy::details

#endif