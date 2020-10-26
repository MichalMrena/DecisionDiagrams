#include "lib/utils/stopwatch.hpp"
#include "lib/utils/print.hpp"
#include "lib/mdd_tools.hpp"

#include "lib/diagrams/vertex_manager.hpp"

#include <cassert>

using namespace mix::dd;
using namespace mix::utils;

auto main() -> int
{
    auto watch = stopwatch();

    using manager_t = vertex_manager<double, void, 2>;
    auto manager     = manager_t(100);
    auto const zero  = manager.terminal_vertex(0);
    auto const zero_ = manager.terminal_vertex(0);
    auto const one   = manager.terminal_vertex(1);
    auto const one_  = manager.terminal_vertex(1);
    auto const x1    = manager.internal_vertex(1, {zero, one});
    auto const x1_   = manager.internal_vertex(1, {zero, one});
    auto const x2    = manager.internal_vertex(2, {zero, x1});
    auto const x2_   = manager.internal_vertex(2, {zero, x1});

    assert(zero == zero_);
    assert(one == one_);
    assert(x1 == x1_);
    assert(x2 == x2_);

    auto otherManager = manager_t {manager};
    auto otherZero    = otherManager.terminal_vertex(0);
    auto otherOne     = otherManager.terminal_vertex(1);
    auto otherx1      = otherManager.internal_vertex(1, {otherZero, otherOne});
    auto otherx2      = otherManager.internal_vertex(2, {otherZero, otherx1});

    assert(zero != otherZero);
    assert(one != otherOne);
    assert(x1 != otherx1);
    assert(x2 != otherx2);

    auto const timeTaken = watch.elapsed_time().count();
    printl("Done.");
    printl("Time taken: " + std::to_string(timeTaken) + " ms");

    return 0;
}