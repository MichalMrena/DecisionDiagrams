#include <libteddy/teddy.hpp>
#include <libteddy/teddy_reliability.hpp>
#include <iostream>

auto example_basic_usage () -> void
{
    // 4 variables, 1000 pre-allocated nodes (see memory management):
    teddy::bdd_manager manager(4, 1'000);

    // create diagram for a single variable (indices start at 0):
    auto x0 = manager.variable(0);

    // we recommend using auto, but if you want you can use an alias:
    using diagram_t = teddy::bdd_manager::diagram_t;
    diagram_t x1 = manager.variable(1);

    // operator() serves the same purpose as .variable call
    // it is useful to create a reference to the manager with name x
    auto& x = manager;
    diagram_t x2 = x(2);

    // diagrams for multiple variables can be created at once:
    std::vector<diagram_t> xs = manager.variables({0, 1, 2, 3, 4});

    // diagram_t is cheap handle type, multiple diagrams can point
    // to a same node, to test whether they do use .equals:
    assert(&x1 != &xs[1] && x1.equals(xs[1]));

    using namespace teddy::ops; // (to simplify operator names)
    // finally, to create a diagram for the function f
    // we use the apply function:
    diagram_t f1 = manager.apply<AND>(xs[0], xs[1]);
    diagram_t f2 = manager.apply<AND>(xs[2], xs[3]);
    diagram_t f  = manager.apply<OR>(f1, f2);

    // now that we have diagram for the funtion f, we can test its properties
    // e.g. evaluate it for give variable assignment
    unsigned int val = manager.evaluate(f, std::array {1, 1, 0, 1});
    // val will contain either 0 or 1 (1 in this case)

    // we can see how the diagram looks like by printing its dot representation
    // into a file or console and visualizing it using e.g. graphviz
    manager.to_dot_graph(std::cout, f); // console
    std::ofstream ofst("f.dot");
    manager.to_dot_graph(ofst, f); // file

    // to calculate number of different variable assignments for which the
    // function evaluates to 1 we can use .satisfy_count:
    std::size_t sc = manager.satisfy_count(f);

    // we can also enumerate all variable assignments for which the
    // the function evaluates to 1:
    std::vector<std::array<unsigned int, 4>> sa
        = manager.satisfy_all<std::array<unsigned int, 4>>(f);
}

auto example_reliability () -> void
{
    // First, we need to create a diagram for the structure function.
    // We use the truth vector of the function:
    std::vector<unsigned int> vector
        { 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1
        , 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2 };

    // The truth vector describes nonhomogenous system, so we also need
    // number of states for each component:
    std::vector<unsigned int> domains({2, 3, 2, 3});
    teddy::ifmss_manager<3> manager(4, 1'000, domains);
    auto sf = manager.from_vector(vector);

    // You can use different combinations of std::vector, std::array or
    // similar containers. We chose vector of arrays here to hold
    // component state probabilities.
    std::vector<std::array<double, 3>> ps
    ({
        {0.1, 0.9, 0.0},
        {0.2, 0.6, 0.2},
        {0.3, 0.7, 0.0},
        {0.1, 0.6, 0.3}
    });

    // To calculate system availability or unavailability for a given
    // system state (1) is as simple as:
    double A = manager.availability(1, ps, sf);
    double U = manager.unavailability(1, ps, sf);

    // We can also simply enumerate all Minimal Cut Vectors for a given system
    // state (1). We just need to specify a type that variables will be stored
    // into. In this case we used the std::array:
    std::vector<std::array<unsigned int, 4>> MCVs
        = manager.mcvs<std::array<unsigned int, 4>>(sf, 1);

    // Importance measures are defined in terms of logic derivatives. Since there are different types derivatives the calculation of the derivatives is separated from the calculation of importance measures.

    // In order to calculace Structural Importance we first need to calculate the derivative.
    auto dpld = manager.idpld_type_3_decrease({1, 0}, 1, sf, 2);

    // Now, to calculate Structural Importance of the second compontnt, we use the derivative.
    auto SI = manager.structural_importance(dpld);
}

auto main () -> int
{
    example_basic_usage();
    example_reliability();
}