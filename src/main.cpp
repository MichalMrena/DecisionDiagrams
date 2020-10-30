#include "lib/utils/stopwatch.hpp"
#include "lib/utils/print.hpp"
#include "lib/mdd_manager.hpp"

#include <cassert>

using namespace mix::dd;
using namespace mix::utils;

// auto pla_test()
// {
//     auto constexpr plaDir = "/mnt/c/Users/mrena/Desktop/pla_files/IWLS93/pla/";
//     auto files = {"15-adder_col.pla"};

//     auto load_pla = [](auto&& filePath)
//     {
//         auto file     = pla_file::load_file(filePath);
//         auto manager  = bdd_manager<double, void>(file.variable_count());
//         auto creator  = manager.creator();
//         auto tools    = manager.tools();
//         auto const ds = creator.from_pla(file, fold_e::left);
//         auto sum = 0u;
//         for (auto& d : ds)
//         {
//             sum += tools.vertex_count(d);
//         }
//         std::cout << filePath << " [" << sum << "] " << std::endl;
//     };

//     for (auto fileName : files)
//     {
//         auto et = avg_run_time(1, std::bind(load_pla, mix::utils::concat(plaDir , fileName)));
//         printl(concat(fileName , " -> " , et , "ms [" , "-" , "]"));
//     }
// }

auto main() -> int
{
    auto watch = stopwatch();

    // pla_test();

    using manager_t = mdd_manager<double, void, 2>;
    auto manager    = manager_t(100);

    auto zero = manager.just_val(0);
    auto one  = manager.just_val(1);
    auto x1   = manager.just_var(1);
    auto x2   = manager.just_var(2);
    auto x3   = manager.just_var(3);
    auto prod = manager.apply(x1, AND(), x2);

    manager.to_dot_graph(std::cout);

    auto const timeTaken = watch.elapsed_time().count();
    printl("Done.");
    printl("Time taken: " + std::to_string(timeTaken) + " ms");

    return 0;
}