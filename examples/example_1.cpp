#include <libteddy/core.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

int main()
{
    // 4 variables, 1000 pre-allocated nodes (see memory management)
    teddy::bdd_manager manager(4, 1'000);

    // Alias for the type of the diagram, or you can just use auto.
    using diagram_t = teddy::bdd_manager::diagram_t;

    // Create diagram for a single variable (indices start at 0).
    diagram_t x0 = manager.variable(0);
    diagram_t x1 = manager.variable(1);

    // operator() serves the same purpose as .variable call.
    // It is convenient to create a reference to the manager with name x.
    teddy::bdd_manager& x = manager;
    diagram_t x2 = x(2);

    // Diagrams for multiple variables can be created at once.
    std::vector<diagram_t> xs = manager.variables({0, 1, 2, 3});

    // diagram_t is cheap handle type, multiple diagrams can point
    // to the same node, to test whether they do use .equals.
    assert(x1.equals(xs[1]));

    // (to simplify operator names)
    using namespace teddy::ops;
    // Finally, to create a diagram for the function:
    // f(x) = (x0 and x1) or (x2 and x3)
    // we use the apply function.
    diagram_t f1 = manager.apply<AND>(xs[0], xs[1]);
    diagram_t f2 = manager.apply<AND>(xs[2], xs[3]);
    diagram_t f  = manager.apply<OR>(f1, f2);

    // Now that we have diagram for the funtion f, we can test its properties
    // e.g., evaluate it for give variable assignment.
    const int val = manager.evaluate(f, std::array {1, 1, 0, 1});
    assert(val == 1);

    // We can see how the diagram looks like by printing its dot representation
    // into a file or console and visualizing it using e.g. graphviz.
    manager.to_dot_graph(std::cout, f);
    std::ofstream ofst("f.dot");
    manager.to_dot_graph(ofst, f);

    // To calculate number of different variable assignments for which the
    // function evaluates to 1 we can use .satisfy_count.
    long long sc = manager.satisfy_count(1, f);

    // We can also enumerate all variable assignments for which the
    // the function evaluates to 1.
    std::vector<std::array<int, 4>> sa
        = manager.satisfy_all<std::array<int, 4>>(1, f);
}