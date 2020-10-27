#include "lib/utils/stopwatch.hpp"
#include "lib/utils/print.hpp"
#include "lib/mdd_manager.hpp"

#include <cassert>

using namespace mix::dd;
using namespace mix::utils;

auto main() -> int
{
    auto watch = stopwatch();

    using manager_t = mdd_manager<double, void, 2>;
    auto manager    = manager_t(100);
    auto creator    = manager.creator();
    auto tools      = manager.tools();

    auto zero  = creator.just_val(0);
    auto zero_ = creator.just_val(0);
    auto one   = creator.just_val(1);
    auto one_  = creator.just_val(1);
    auto x1    = creator.just_var(1);
    auto x1_   = creator.just_var(1);
    auto x2    = creator.just_var(2);
    auto x2_   = creator.just_var(2);

    assert(zero == zero_);
    assert(one == one_);
    assert(x1 == x1_);
    assert(x2 == x2_);

    tools.to_dot_graph(std::cout, x1);
    tools.to_dot_graph(std::cout, zero);

    auto const timeTaken = watch.elapsed_time().count();
    printl("Done.");
    printl("Time taken: " + std::to_string(timeTaken) + " ms");

    return 0;
}