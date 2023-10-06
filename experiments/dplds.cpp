#include <libteddy/reliability.hpp>
#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <libtsl/utilities.hpp>
#include <algorithm>
#include <chrono>
#include <nanobench/nanobench.h>
#include <iostream>
#include <random>

auto dpld_type3_naive(auto& manager, auto const& diagram, int const index)
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

auto dpld_type2_naive(auto& manager, auto const& diagram, int const index)
{
    auto const lhs = manager.get_cofactor(diagram, index, 0);
    auto const rhs = manager.get_cofactor(diagram, index, 1);
    return manager.template apply<teddy::ops::LESS>(lhs, rhs);
}

char const* unit_str(std::chrono::nanoseconds) { return "ns"; }
char const* unit_str(std::chrono::microseconds){ return "µs"; }
char const* unit_str(std::chrono::milliseconds){ return "ms"; }

auto main() -> int
{
    namespace ch = std::chrono;
    using time_unit = ch::nanoseconds;

    char const* const Sep         = "\t";
    char const* const Eol         = "\n";
    int constexpr DiagramCount    = 10;
    int constexpr ReplCount       = 10;
    int constexpr StateCount      = 3;
    int constexpr DerivativeCount = 10;
    int constexpr DerivativeType  = 3;
    int constexpr Seed            = 18'234;
    // int constexpr VarCount        = 1'000;
    int constexpr VarCount        = 20;
    int constexpr TermCount       = 35;
    int constexpr TermSize        = 7;

    std::mt19937_64 exprRng(Seed);
    auto indices = teddy::tsl::fill_vector(VarCount, teddy::tsl::Identity);
    std::shuffle(indices.begin(), indices.end(), exprRng);

    std::cout << "diagram-id"  << Sep
              << "node-count"  << Sep
              << "naive-dpld[" << unit_str(time_unit()) << "]" << Sep
              << "new-dpld["   << unit_str(time_unit()) << "]" << Sep
              << "relative"    << Eol;

    for (int diagramId = 0; diagramId < DiagramCount; ++diagramId)
    {
        auto const expr = teddy::tsl::make_minmax_expression(
            exprRng,
            VarCount,
            TermCount,
            TermSize
        );
        // auto const expr = teddy::tsl::make_expression_tree(
        //     VarCount,
        //     exprRng,
        //     exprRng
        // );
        teddy::mss_manager<StateCount> manager(VarCount, 1'000'000);
        auto const diagram = teddy::tsl::make_diagram(expr, manager);
        auto const nodeCount = manager.get_node_count(diagram);
        for (int repl = 0; repl < ReplCount; ++repl)
        {
            std::cout << diagramId << Sep
                      << nodeCount << Sep;

            time_unit timeNaive = time_unit::zero();
            time_unit timeNew   = time_unit::zero();

            // naive
            {
                auto const start = ch::high_resolution_clock::now();
                for (int i = 0; i < DerivativeCount; ++i)
                {
                    if constexpr (DerivativeType == 3)
                    {
                        ankerl::nanobench::doNotOptimizeAway(
                            dpld_type3_naive(
                                manager,
                                diagram,
                                indices[(size_t)i]
                            )
                        );
                    }

                    if constexpr (DerivativeType == 2)
                    {
                        ankerl::nanobench::doNotOptimizeAway(
                            dpld_type2_naive(
                                manager,
                                diagram,
                                indices[(size_t)i]
                            )
                        );
                    }
                }
                auto const end = ch::high_resolution_clock::now();
                auto const elapsed = ch::duration_cast<time_unit>(
                    end - start
                );
                timeNaive = elapsed;
                std::cout << elapsed.count() << Sep;
            }

            manager.clear_cache();

            // new
            {
                auto const start = ch::high_resolution_clock::now();
                for (int i = 0; i < DerivativeCount; ++i)
                {
                    if constexpr (DerivativeType == 3)
                    {
                        ankerl::nanobench::doNotOptimizeAway(
                            manager.dpld(
                                {indices[(size_t)i], 0, 1},
                                teddy::dpld::type_3_increase(1),
                                diagram
                            )
                        );
                    }

                    if constexpr (DerivativeType == 2)
                    {
                        ankerl::nanobench::doNotOptimizeAway(
                            manager.dpld(
                                {indices[(size_t)i], 0, 1},
                                teddy::dpld::type_2_increase(),
                                diagram
                            )
                        );
                    }
                }
                auto const end = ch::high_resolution_clock::now();
                auto const elapsed = ch::duration_cast<time_unit>(
                    end - start
                );
                timeNew = elapsed;
                std::cout << elapsed.count() << Sep;
            }
            double const relDiff =
                static_cast<double>(std::min(timeNaive, timeNew).count()) /
                static_cast<double>(std::max(timeNaive, timeNew).count());
            std::cout << relDiff << Eol;
        }
    }
}