#include <iostream>
#include <iomanip>

#include "other/stopwatch.hpp"
#include "other/print.hpp"
#include "lib/utils/object_pool.hpp"
#include "lib/mdd_manager.hpp"
#include "lib/bdd_manager.hpp"

#include "test/test_mdd.hpp"
#include "test/test_reliability.hpp"

#include <bitset>
#include <cassert>
#include <random>
#include <map>

using namespace teddy;
using namespace teddy::utils;
using namespace teddy::test;

auto pla_sanity_check()
{
    auto constexpr plaDir = "/mnt/c/Users/mrena/Desktop/pla_files/IWLS93/pla/";
    // auto const files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    // auto const files = {"14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    auto const files = {"13-adder_col.pla"};
    // auto const files = {"inc.pla"};

    for (auto const fileName : files)
    {
        auto const filePath = teddy::utils::concat(plaDir , fileName);
        auto const file     = pla_file::load_file(filePath);
        auto manager        = bdd_manager<void, void>(file.variable_count());
        auto const ds       = manager.from_pla(file, fold_e::right);
        auto sum            = 0ul;
        for (auto& d : ds)
        {
            sum += manager.vertex_count(d);
        }
        std::cout << fileName << " [" << sum << "] " << std::endl;
    }

    // auto const filePath = teddy::utils::concat(plaDir , "inc.pla");
    // auto const file     = pla_file::load_file(filePath);
    // pla_file::save_to_file("test_pla_out.pla", file);
}

auto pla_test_speed(auto const n)
{
    auto constexpr plaDir = "/mnt/c/Users/mrena/Desktop/pla_files/IWLS93/pla/";
    // auto const files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla"};
    // auto const files = {"14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    // auto const files = {"15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla"};
    auto const files = {"16-adder_col.pla"};

    auto const build_diagrams = [](auto const& plaFileRef)
    {
        auto manager  = bdd_manager<void, void>(plaFileRef.get().variable_count(), 2'000'000);
        auto const ds = manager.from_pla(plaFileRef.get(), fold_e::tree);
    };

    for (auto fileName : files)
    {
        auto const filePath = concat(plaDir , fileName);
        auto const plaFile  = pla_file::load_file(filePath);
        auto const elapsed  = avg_run_time(static_cast<std::size_t>(n), std::bind_front(build_diagrams, std::cref(plaFile)));
        printl(concat(fileName , " -> " , elapsed , "ms"));
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

    [[maybe_unused]]
    auto n = node<int, degrees::fixed<2>>(1);

    std::cout << "Done." << '\n';
    return 0;
}
