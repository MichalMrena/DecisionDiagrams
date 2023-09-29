#include <libteddy/reliability.hpp>
#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <nanobench/nanobench.h>
#include <iostream>
#include <random>

auto main() -> int
{
    auto constexpr StateCount = 3;
    auto constexpr VarCount   = 20;
    auto constexpr TermCount  = 35;
    auto constexpr TermSize   = 7;
    auto constexpr Seed       = 18'234;
    auto exprRng    = std::mt19937_64(Seed);
    auto const expr = teddy::tsl::make_minmax_expression(
        exprRng,
        VarCount,
        TermCount,
        TermSize
    );
    auto manager = teddy::mss_manager<StateCount>(VarCount, 1'000'000);
    auto const diagram = teddy::tsl::make_diagram(expr, manager);
    std::cout << manager.get_node_count(diagram) << "\n";
}