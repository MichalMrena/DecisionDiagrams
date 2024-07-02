#include <libteddy/core.hpp>
#include <libteddy/io.hpp>
#include <libteddy/reliability.hpp>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

auto example_basic_usage () -> void
{
    // 4 variables, 1000 pre-allocated nodes (see memory management)
    teddy::bdd_manager manager(4, 1'000);

    // Alias for the type of the diagram, or you can just use auto.
    using bdd_t = teddy::bdd_manager::diagram_t;

    // Create diagram for a single variable (indices start at 0).
    bdd_t x0 = manager.variable(0);
    bdd_t x1 = manager.variable(1);

    // operator() serves the same purpose as .variable call.
    // It is convenient to create a reference to the manager with name x.
    teddy::bdd_manager& x = manager;
    bdd_t x2 = x(2);

    // Diagrams for multiple variables can be created at once.
    std::vector<bdd_t> xs = manager.variables({0, 1, 2, 3});

    // diagram_t is cheap handle type, multiple diagrams can point
    // to the same node, to test whether they do use .equals.
    assert(x1.equals(xs[1]));

    // (to simplify operator names)
    using namespace teddy::ops;
    // Finally, to create a diagram for the function:
    // f(x) = (x0 and x1) or (x2 and x3)
    // we use the apply function.
    bdd_t f1 = manager.apply<AND>(xs[0], xs[1]);
    bdd_t f2 = manager.apply<AND>(xs[2], xs[3]);
    bdd_t f  = manager.apply<OR>(f1, f2);

    // Now that we have diagram for the funtion f, we can test its properties
    // e.g., evaluate it for give variable assignment.
    const int val = manager.evaluate(f, std::array {1, 1, 0, 1});
    std::cout << "evaluate([1,1,0,1]) = " << val << "\n";

    // We can see how the diagram looks like by printing its dot representation
    // into a file or console and visualizing it using e.g. graphviz.
    teddy::io::to_dot(manager, std::cout, f);
    std::ofstream ofst("f.dot");
    teddy::io::to_dot(manager, ofst, f);

    // To calculate number of different variable assignments for which the
    // function evaluates to 1 we can use .satisfy_count.
    const teddy::longint sc = manager.satisfy_count(1, f);
    std::cout << "Satisfy-count(1) = " << sc << "\n";

    // We can also enumerate all variable assignments for which the
    // the function evaluates to 1.
    std::vector<std::array<int, 4>> sa
        = manager.satisfy_all<std::array<int, 4>>(1, f);
}

auto example_reliability () -> void
{
    // First, we need to create a diagram for the structure function.
    // We can use the truth vector of the function.
    std::vector<int> vector(
        {0,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,2,2,2,2,2,1,2,2,2,2,2}
    );

    // The truth vector describes a nonhomogeneous system, so we also need.
    // The number of states of each component.
    std::vector<int> domains({2, 3, 2, 3});

    // 4 components, 1000 pre-allocated nodes (see memory management)
    teddy::ifmss_manager<3> manager(4, 1'000, domains);

    // Alias for the type of the diagram, or you can just use auto.
    using mdd_t = teddy::ifmss_manager<3>::diagram_t;
    mdd_t sf = teddy::io::from_vector(manager, vector);

    // We can use different combinations of std::vector, std::array or similar containers.
    // We chose vector of arrays here to hold component state probabilities.
    std::vector<std::array<double, 3>> ps
    ({
        {0.1, 0.9, 0.0},
        {0.2, 0.6, 0.2},
        {0.3, 0.7, 0.0},
        {0.1, 0.6, 0.3}
    });

    // To calculate system availability or unavailability for a given.
    // System state (1) is as simple as:
    const double A = manager.calculate_availability(1, ps, sf);
    const double U = manager.calculate_unavailability(1, ps, sf);
    std::cout << "A = " << A << "\n";
    std::cout << "U = " << U << "\n";

    // We can also simply enumerate all Minimal Cut Vectors for a given system
    // state (1). We just need to specify a type that will store the vectors.
    // In this case, we used std::array:
    std::vector<std::array<int, 4>> MCVs = manager.mcvs<std::array<int, 4>>(sf, 1);

    // Importance measures are defined in terms of logic derivatives.
    // Since there are different types of derivatives the calculation of
    // the derivatives is separated from the calculation of importance measures.

    // To calculate Structural Importance we first need to calculate
    // the derivative.
    mdd_t dpbd = manager.dpld({2, 1, 0}, teddy::dpld::type_3_decrease(1), sf);

    // Now, to calculate the Structural Importance of the second component,
    // we use the derivative.
    const double SI_2 = manager.structural_importance(dpbd);
    std::cout << "SI_2 = " << SI_2 << "\n";
}

auto main () -> int
{
    example_basic_usage();
    example_reliability();
}