#include <libteddy/reliability.hpp>
#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <libtsl/utilities.hpp>
#include <nanobench/nanobench.h>
#include <chrono>
#include <iostream>
#include <random>

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
    int constexpr ReplCount       = 1;
    int constexpr TimePointCount  = 10;
    // int constexpr StateCount      = 2;
    int constexpr Seed            = 5343584;
    // int constexpr VarCount        = 1'000;
    int constexpr VarCount        = 20;
    int constexpr TermCount       = 35;
    int constexpr TermSize        = 7;

    std::ranlux48 exprRng(Seed);

    std::cout << "diagram-id"       << Sep
              << "node-count"       << Sep
              << "basic-prob-init[" << unit_str(time_unit()) << "]" << Sep
              << "basic-prob["      << unit_str(time_unit()) << "]" << Sep
              << "sym-prob-init["   << unit_str(time_unit()) << "]" << Sep
              << "sym-prob["        << unit_str(time_unit()) << "]" << Eol;

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
        teddy::bss_manager manager(VarCount, 1'000'000);
        auto const diagram = teddy::tsl::make_diagram(expr, manager);
        auto const nodeCount = manager.get_node_count(diagram);
        for (int repl = 0; repl < ReplCount; ++repl)
        {
            std::cout << diagramId << Sep
                      << nodeCount << Sep;

            time_unit timeBasic = time_unit::zero();
            time_unit timeSymbolic = time_unit::zero();

            // basic
            {
                auto probs = teddy::tsl::make_time_probability_vector(
                    VarCount,
                    exprRng
                );
                auto const start = ch::high_resolution_clock::now();
                double t = 0;
                for (int i = 0; i < TimePointCount; ++i)
                {
                    double const A = manager.calculate_availability(
                        teddy::probs::at_time(probs, t),
                        diagram
                    );
                    ankerl::nanobench::doNotOptimizeAway(A);
                    t += 1;
                }
                auto const end = ch::high_resolution_clock::now();
                auto const elapsed = ch::duration_cast<time_unit>(
                    end - start
                );
                timeBasic = elapsed;
                std::cout << 0 << Sep; // no init here
                std::cout << timeBasic.count() << Sep;
            }

            // symbolic
            {
                auto probs = teddy::symprobs::as_matrix(
                    teddy::tsl::make_time_symprobability_vector(
                        VarCount,
                        exprRng
                    )
                );

                auto start = ch::high_resolution_clock::now();
                teddy::symprobs::expression Aexpr =
                    manager.symbolic_availability(
                        1,
                        probs,
                        diagram
                    );
                auto end = ch::high_resolution_clock::now();
                time_unit timeMkExpr = ch::duration_cast<time_unit>(
                    end - start
                );

                start = ch::high_resolution_clock::now();
                double t = 0;
                for (int i = 0; i < TimePointCount; ++i)
                {
                    double const A = Aexpr.evaluate(t);
                    ankerl::nanobench::doNotOptimizeAway(A);
                    t += 1;
                }
                end = ch::high_resolution_clock::now();
                auto const elapsed = ch::duration_cast<time_unit>(
                    end - start
                );
                timeSymbolic = elapsed;
                std::cout << timeMkExpr.count() << Sep;
                std::cout << timeSymbolic.count()    << Eol;
            }
        }
    }
}