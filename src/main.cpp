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

using namespace mix::dd;
using namespace mix::utils;
using namespace mix::dd::test;

auto pla_sanity_check()
{
    auto constexpr plaDir = "/mnt/c/Users/mrena/Desktop/pla_files/IWLS93/pla/";
    auto const files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    // auto const files = {"14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    // auto const files = {"13-adder_col.pla"};
    // auto const files = {"inc.pla"};

    for (auto const fileName : files)
    {
        auto const filePath = mix::utils::concat(plaDir , fileName);
        auto const file     = pla_file::load_file(filePath);
        auto manager        = bdd_manager<void, void>(file.variable_count());
        auto const ds       = manager.from_pla(file, fold_e::tree);
        auto sum            = 0ul;
        for (auto& d : ds)
        {
            sum += manager.vertex_count(d);
        }
        std::cout << fileName << " [" << sum << "] " << std::endl;
    }

    // auto const filePath = mix::utils::concat(plaDir , "inc.pla");
    // auto const file     = pla_file::load_file(filePath);
    // pla_file::save_to_file("test_pla_out.pla", file);
}

auto main () -> int
{
    // satisfy_count to go step

    // change namespace to teddy

    // pla_sanity_check();
    test_mdd_random<3>(10, order_e::Random, domain_e::Nonhomogenous);
    test_mdd_vector(10);
    test_bss();
    test_mss();

    std::cout << "Done." << '\n';
    return 0;
}
