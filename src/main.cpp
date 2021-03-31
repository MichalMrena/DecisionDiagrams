#include <iostream>
#include <iomanip>

#include "lib/utils/stopwatch.hpp"
#include "lib/utils/print.hpp"
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
    // auto const files = {"10-adder_col.pla"};

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
}

auto pla_test_speed()
{
    auto constexpr plaDir = "/mnt/c/Users/mrena/Desktop/pla_files/IWLS93/pla/";
    auto const files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    // auto const files = {"14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    // auto const files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla"};
    // auto const files = {"15-adder_col.pla"};

    auto const build_diagrams = [](auto const& plaFileRef)
    {
        auto manager  = bdd_manager<void, void>(plaFileRef.get().variable_count());
        auto const ds = manager.from_pla(plaFileRef.get(), fold_e::left);
    };

    for (auto fileName : files)
    {
        auto const filePath = mix::utils::concat(plaDir , fileName);
        auto const plaFile  = pla_file::load_file(filePath);
        auto const elapsed  = avg_run_time(1, std::bind_front(build_diagrams, std::cref(plaFile)));
        printl(concat(fileName , " -> " , elapsed , "ms"));
    }
}

auto sift()
{
    auto m  = make_bdd_manager(8);
    register_manager(m);
    auto& x = m;
    auto f  = (x(0) && x(4)) || (x(1) && x(5)) || (x(2) && x(6)) || (x(3) && x(7));
    std::cout << "before: " << m.vertex_count(f) << '\n';
    m.collect_garbage();
    m.sift_variables();
    std::cout << "after: " << m.vertex_count(f) << '\n';
    auto fr = m.reduce(f);
    std::cout << "after reduce: " << m.vertex_count(fr) << '\n';
    m.to_dot_graph(std::cout, f);
}

auto example()
{
    auto m  = mdd_manager<void, void, 3>(3);
    auto& x = m;
    register_manager(m);
    auto f  = m.apply<MAX>(m.apply<MIN>(x(0), x(1)), x(2));
    m.to_dot_graph(std::cout, f);
    auto sc = m.satisfy_count(1, f);
}

auto main () -> int
{
    // pla_sanity_check();
    // pla_test_speed();

    // xor hash combine pre apply cache?
    // je symetrický, takže by dobre riešil komutatívnosť
    // stačilo by potom upraviť porvnávanie kľúča aby bralo do úvahy komutatívnosť

    // satisfy_count to go step

    // zbaviť sa nodomain

    // move a forward makrá, see. https://foonathan.net/2020/09/move-forward/

   test_mdd_random<3>(10, order_e::Random, domain_e::Nonhomogenous);
   test_mdd_vector(10);
   test_bss();
   test_mss();

//  sift();
    // example();

    std::cout << "Done." << '\n';
    return 0;
}
