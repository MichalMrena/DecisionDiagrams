#include "teddy/teddy.hpp"
#include "teddy/teddy_reliability.hpp"
#include <iostream>

#include <string>
#include <cstdio>

using namespace teddy;

auto pla_sanity_check()
{
    using namespace std::string_literals;

    auto const plaDir = "/home/michal/Downloads/pla/"s;
    // auto const files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla"};
    // auto const files = {"14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    auto const files = {"table3.pla"s};

    for (auto const& fileName : files)
    {
        auto const file = pla_file::load_file(plaDir + fileName);
        if (not file)
        {
            std::cout << "Failed to load '" << plaDir + fileName << "'.\n";
            continue;
        }
        auto manager    = bdd_manager((*file).variable_count(), 500);
        auto const ds   = manager.from_pla(*file, fold_type::Tree);
        auto sum        = 0ul;
        for (auto& d : ds)
        {
            sum += manager.node_count(d);
        }
        std::cout << fileName << " [" << sum << "] " << std::endl;
    }
}

auto example1()
{
    using namespace teddy;
    auto manager = mdd_manager<4>(4, 1'000);
    auto& x = manager;
    auto g = manager.apply<ops::MULTIPLIES<4>>(x(0), x(1));
    auto h = manager.apply<ops::MULTIPLIES<4>>(x(2), x(3));
    auto f = manager.apply<ops::PLUS<4>>(g, h);
    auto sc = manager.satisfy_count(3, f);
    if (g.equals(h))
    {
        std::puts("This should not happen");
    }
    auto sa = manager.satisfy_all<std::array<uint_t, 4>>(2, f);
    auto ds = manager.dependency_set(f);
    std::puts("Some number:");
    std::puts(std::to_string(sc + sa.size() + ds.size()).c_str());
}

auto example2()
{
    using namespace teddy;
    auto const vector = std::vector<teddy::uint_t>
        { 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1
        , 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2 };
    auto domains = std::vector<uint_t>({2, 3, 2, 3});
    auto const ps = std::vector<std::vector<double>>
    ({
        {0.1, 0.9, 0.0},
        {0.2, 0.6, 0.2},
        {0.3, 0.7, 0.0},
        {0.1, 0.6, 0.3}
    });
    auto manager = teddy::ifmss_manager<3>(4, 1'000, {2, 3, 2, 3});
    auto sf = manager.from_vector(vector);
    std::puts("Availability");
    std::puts(std::to_string(manager.availability(1, ps, sf)).c_str());

    std::puts("Unavailability");
    std::puts(std::to_string(manager.unavailability(1, ps, sf)).c_str());

    auto dpbds = manager.dpbds_i_3({1, 0}, 2, sf);
    auto sis = manager.structural_importances(dpbds);
    auto i = 0;
    for (auto const si : sis)
    {
        auto out = "i = " + std::to_string(i) + ", SI = " + std::to_string(si);
        std::puts(out.c_str());
    }
}

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

    // pla_sanity_check();

    // auto const vector = std::vector<teddy::uint_t>
    //     { 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1
    //     , 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2 };
    // auto domains = std::vector<teddy::uint_t>({2, 3, 2, 3});
    // auto manager = teddy::ifmss_manager<3>(4, 1000, domains);
    // auto sf = manager.from_vector(vector);

    // auto cf1 = manager.cofactor(sf, 0, 1);
    // auto cf0 = manager.cofactor(sf, 0, 0);

    // manager.to_dot_graph(std::cout, sf);
    // manager.to_dot_graph(std::cout, cf1);
    // manager.to_dot_graph(std::cout, cf0);

    example1();
    example2();

    std::cout << "Done." << '\n';
    return 0;
}