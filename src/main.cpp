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

    using diagram_t = teddy::ifmss_manager<3>::diagram_t;
    auto dpbds = std::vector<diagram_t>();
    for (auto i = 0u; i < manager.get_var_count(); ++i)
    {
        dpbds.push_back(manager.idpbd_type_3_decrease({1, 0}, 1, sf, i));
    }
    auto sis = std::vector<double>();
    for (auto i = 0u; i < manager.get_var_count(); ++i)
    {
        sis.push_back(manager.structural_importance(dpbds[i]));
    }
    auto i = 0;
    for (auto const si : sis)
    {
        auto out = "i = " + std::to_string(i) + ", SI = " + std::to_string(si);
        std::puts(out.c_str());
        ++i;
    }
}

auto example_rel_doc()
{
    // First, we need to create a diagram for the structure function.
    // We use the truth vector of the function:
    std::vector<teddy::uint_t> vector
        { 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1
        , 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2 };

    // The truth vector describes nonhomogenous system, so we also need
    // number of states for each component:
    std::vector<uint_t> domains({2, 3, 2, 3});
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
    auto dpbd = manager.idpbd_type_3_decrease({1, 0}, 1, sf, 2);

    // Now, to calculate Structural Importance of the second compontnt, we use the derivative.
    auto SI = manager.structural_importance(dpbd);
}

auto main () -> int
{
    // TODO Fibonacci hashing + power of 2 size:
    // see: https://www.boost.org/doc/libs/develop/libs/unordered/doc/html/unordered.html#rationale_number_of_buckets

    // TODO Iterovanie hash tabulky, zretazenie cez next pointer, koniec bucketu by sa flagol cez priznakovy najmenej vyznamny bit

    // pla_test_speed(1);
    // pla_sanity_check();
    // test_mdd_random<3>(10, order_e::Random, domain_e::Nonhomogenous);
    // test_mdd_vector(10);
    // test_bss();
    // test_mss();

    // pla_sanity_check();

    example1();
    example2();
    example_rel_doc();

    std::cout << "Done." << '\n';
    return 0;
}
