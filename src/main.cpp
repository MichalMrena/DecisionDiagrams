#include <iostream>

#include "lib/utils/stopwatch.hpp"
#include "lib/utils/print.hpp"
#include "lib/mdd_manager.hpp"
#include "lib/bdd_manager.hpp"

#include <bitset>
#include <cassert>

using namespace mix::dd;
using namespace mix::utils;

auto reliability_test()
{
    auto manager = bdd_manager<double, void>(5);
    auto& m = manager;
    auto& x = manager;

    auto sf = m.apply(m.apply(m.apply(x(0), AND(), x(1)), OR(), m.apply(x(2), AND(), x(3))), OR(), x(4));
    auto const ps = std::vector {0.9, 0.8, 0.7, 0.9, 0.9};
    auto dpbds = m.dpbds(sf);

    manager.calculate_probabilities(sf, ps);
    auto const A    = manager.get_availability();
    auto const U    = manager.get_unavailability();
    auto const SIs  = m.structural_importances(dpbds);
    auto const BIs  = m.birnbaum_importances(dpbds, ps);
    auto const CIs  = m.criticality_importances(BIs, ps, U);
    auto const FIs  = m.fussell_vesely_importances(dpbds, ps, U);
    auto const MCVs = m.mcvs<std::bitset<5>>(std::move(dpbds));

    printl(concat("A = "   , A));
    printl(concat("U = "   , U));
    printl(concat("SI "    , concat_range(SIs, " ")));
    printl(concat("BI "    , concat_range(BIs, " ")));
    printl(concat("CI "    , concat_range(CIs, " ")));
    printl(concat("FI "    , concat_range(FIs, " ")));
    printl(concat("MCVs: " , concat_range(MCVs, ", ")));
}

auto pla_test()
{
    auto constexpr plaDir = "/mnt/c/Users/mrena/Desktop/pla_files/IWLS93/pla/";
    auto files = {"12-adder_col.pla"};

    auto load_pla = [plaDir](auto&& fileName)
    {
        auto const filePath = mix::utils::concat(plaDir , fileName);
        auto file           = pla_file::load_file(filePath);
        auto manager        = bdd_manager<double, void>(file.variable_count());
        auto const ds       = manager.from_pla(file, fold_e::left);
        auto sum            = 0u;
        for (auto& d : ds)
        {
            sum += manager.vertex_count(d);
        }
        std::cout << fileName << " [" << sum << "] " << std::endl;
    };

    for (auto fileName : files)
    {
        auto et = avg_run_time(1, std::bind(load_pla, fileName));
        printl(concat(fileName , " -> " , et , "ms [" , "-" , "]"));
    }
}

auto basic_test()
{
    using manager_t = mdd_manager<double, void, 2>;
    auto manager    = manager_t(100);

    auto zero = manager.just_val(0);
    auto one  = manager.just_val(1);
    auto x1   = manager.just_var(1);
    auto x2   = manager.just_var(2);
    auto x3   = manager.just_var(3);
    auto prod = manager.apply(x1, AND(), x2);

    manager.to_dot_graph(std::cout);

}

auto main() -> int
{
    auto watch = stopwatch();

    // basic_test();
    // pla_test();
    reliability_test();

    auto const timeTaken = watch.elapsed_time().count();
    printl("Done.");
    printl("Time taken: " + std::to_string(timeTaken) + " ms");

    return 0;
}