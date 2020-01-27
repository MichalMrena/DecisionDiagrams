#include <iostream>
#include <stdexcept>

#include "bdd/bdd_tools.hpp"
#include "utils/io.hpp"
#include "utils/stopwatch.hpp"

using namespace mix::dd;
using namespace mix::utils;

using VertexDataType = empty_t;
using ArcDataType    = empty_t;
using bdd_t          = bdd<VertexDataType, ArcDataType>;
using diagram_tools  = bdd_tools<VertexDataType, ArcDataType>;

auto main() -> int
{
    stopwatch watch;

    // Create multiple diagrams from pla file.
    std::vector<bdd_t> bddsFromPla
    {
        diagram_tools::create_from_pla("apex4.pla")
    };

    // Create only the first diagram from pla file.
    bdd_t onlyFirstDiagram
    {
        diagram_tools::create_one_from_pla("apex4.pla")
    };

    // Move construct diagram.
    bdd_t otherDiagram
    {
        std::move(bddsFromPla.at(2))
    };

    // Create new diagram by merging two other diagrams.
    bdd_t mergerdDiagram
    {
        diagram_tools::merge(onlyFirstDiagram, otherDiagram, XOR {})
    };

    // Create diagram from expression using operators overloaded for merging and function x(i).
    bdd_t diagramFromExpression
    {
        (x(0) && x(1)) || (x(1) && x(2))
    };

    // Print dot representation of the diagram.
    std::cout << diagramFromExpression;

    // Print number of vertices in the graph.
    std::cout << diagramFromExpression.vertex_count() << '\n';

    // Copy construct diagram.
    bdd_t restrictedDiagram
    {
        diagramFromExpression
    };

    // They should be the same.
    if (diagramFromExpression != restrictedDiagram)
    {
        throw std::runtime_error {"Not good."};
    }

    // Restriction f|x2=1;
    diagram_tools::restrict_by(restrictedDiagram, 2, 1);

    // Check if it works.
    std::cout << restrictedDiagram;

    // Move construct diagram
    bdd_t negatedDiagram
    {
        std::move(restrictedDiagram)
    };

    // Negate diagram.
    diagram_tools::negate(negatedDiagram);

    // Check if it works.
    std::cout << negatedDiagram;

    printl("Done.");
    printl("Time taken: " + std::to_string(watch.elapsed_time().count()) + " ms");

    return 0;
}