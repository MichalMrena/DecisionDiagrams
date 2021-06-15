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
    auto const files = {"15-adder_col.pla"};

    auto const build_diagrams = [](auto const& plaFileRef)
    {
        auto manager  = bdd_manager<void, void>(plaFileRef.get().variable_count(), 2'000'000);
        auto const ds = manager.from_pla(plaFileRef.get(), fold_e::tree);
    };

    for (auto fileName : files)
    {
        auto const filePath = concat(plaDir , fileName);
        auto const plaFile  = pla_file::load_file(filePath);
        auto const elapsed  = avg_run_time(n, std::bind_front(build_diagrams, std::cref(plaFile)));
        printl(concat(fileName , " -> " , elapsed , "ms"));
    }
}

auto main () -> int
{
    // satisfy_count to go step
    // change namespace to teddy
    // TODO union on forwardStar_ / value_
    // merge vertexdata a arcdata do jedneho parametra

    // pla_test_speed(1);
    pla_sanity_check();
    // test_mdd_random<3>(10, order_e::Random, domain_e::Nonhomogenous);
    // test_mdd_vector(10);
    // test_bss();
    // test_mss();

    std::cout << "Done." << '\n';
    return 0;
}
