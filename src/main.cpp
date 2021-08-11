#include "teddy/teddy.hpp"

#include <iostream>

using namespace teddy;

auto main () -> int
{
    // class diagram_manager<Data, Degree, Domains>
    // class bin_diagram_manager<Data> : diagram_manager<Data, degrees::nary<2>,  domains::nary<2>>

    // class bdd_manager       : bin_diagram_manager<void>
        // TODO conditional_t pre P == 2 dediť z bin_diagram_manager
    // class mdd_manager<P>    : diagram_manager<void, degrees::nary<P>,  domains::nary<P>>
    // class imdd_manager      : diagram_manager<void, degrees::mixed,    domains::mixed>
    // class ifmdd_manager<PM> : diagram_manager<void, degrees::nary<PM>, domains::mixed>

        // TODO conditional_t pre P == 2 dediť z bin_diagram_manager
    // class hom_mss_reliability<P> : diagram_manager<double, degrees::nary<P>, domains::nary<P>>
    // class het_mss_reliability    : diagram_manager<double, degrees::mixed,   domains::mixed>
    // class bss_reliability        : hom_mss_reliability<2>

    // bdd_manager()
    // mdd_manager<P>()
    // imdd_manager(domains)
    // ifmdd_manager<P>(domains)

    // TODO bit flag in-use, set when vertex is moved back into pool,
    // vertices can be removed from cache based on this flag
    // unused vertices can be chained using the next member

    // pla_test_speed(1);
    // pla_sanity_check();
    // test_mdd_random<3>(10, order_e::Random, domain_e::Nonhomogenous);
    // test_mdd_vector(10);
    // test_bss();
    // test_mss();

    using node_t = node<int, degrees::fixed<2>>;

    [[maybe_unused]]
    auto internal = node_t(1);
    [[maybe_unused]]
    auto terminal = node_t(1, {&internal, &internal});

    std::cout << "Done." << '\n';
    return 0;
}
