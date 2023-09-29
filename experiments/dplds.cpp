#include <libteddy/reliability.hpp>
#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <nanobench/nanobench.h>
#include <iostream>
#include <random>

auto dpld_naive(auto& manager, auto const& diagram, int const index)
{
    auto const lhsCofactor = manager.get_cofactor(diagram, index, 0);
    auto const rhsCofactor = manager.get_cofactor(diagram, index, 1);
    auto const lhs = manager.transform(lhsCofactor, [](int const val)
    {
        return val < 1;
    });
    auto const rhs = manager.transform(rhsCofactor, [](int const val)
    {
        return val >= 1;
    });
    return manager.template apply<teddy::ops::AND>(lhs, rhs);
}

auto main() -> int
{
    auto constexpr StateCount = 3;
    auto constexpr VarCount   = 15;
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

    ankerl::nanobench::Bench bench;

    // for (auto i = 0; i < VarCount; ++i)
    // {
    //     auto const d1 = manager.dpld({i, 0, 1}, teddy::dpld::type_3_increase(1), diagram);
    //     auto const d2 = dpld_naive(manager, diagram, i);
    //     if (d1.equals(d2))
    //     {
    //         std::cout << "ok" << "\n";
    //     }
    //     else
    //     {
    //         std::cout << "!!! not good" << "\n";
    //     }
    // }

    bench.title("DPLDs");
    bench.warmup(100);
    bench.relative(true);
    bench.runWithBeforeHook(
        "dpld-new",
        [&manager, &diagram]()
        {
            for (auto i = 0; i < VarCount; ++i)
            {
                manager.dpld({i, 0, 1}, teddy::dpld::type_3_increase(1), diagram);
            }
        },
        [&manager]()
        {
            manager.clear_cache();
        }
    );
    bench.runWithBeforeHook(
        "dpld-old",
        [&manager, &diagram]()
        {
            for (auto i = 0; i < VarCount; ++i)
            {
                dpld_naive(manager, diagram, i);
            }
        },
        [&manager]()
        {
            manager.clear_cache();
        }
    );
    bench.render(ankerl::nanobench::templates::csv(), std::cout);
}