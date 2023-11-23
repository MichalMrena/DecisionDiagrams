#include <libteddy/reliability.hpp>
#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <libtsl/utilities.hpp>
#include <nanobench/nanobench.h>
#include <iostream>
#include <random>

#include "utils.hpp"

struct FixedDiagramGenerator
{

};

class SPDiagramGenerator
{
public:
    SPDiagramGenerator(
        std::size_t const seed,
        int const varCount,
        int const termCount,
        int const termSize
    ) :
        rng_(seed),
        varCount_(varCount),
        termCount_(termCount),
        termSize_(termSize)
    {
    }

    auto operator() (teddy::bss_manager& manager)
    {
        auto const SPExpr = teddy::tsl::make_minmax_expression(
            rng_,
            varCount_,
            termCount_,
            termSize_
        );
        return teddy::tsl::make_diagram(SPExpr, manager);
    }

private:
    std::ranlux48 rng_;
    int varCount_;
    int termCount_;
    int termSize_;
};

struct PLADiagramGenerator
{

};

template<class DiagramGenerator>
auto evalute_system (
    int const diagramCount,
    int const replicationCount,
    int const timePointCount,
    int const varCount,
    DiagramGenerator& gen
) -> void
{
    using namespace teddy::utils;
    using time_unit = std::chrono::nanoseconds;
    using bdd_t = teddy::bss_manager::diagram_t;

    // CVS parameters
    char const* const Sep = "\t";
    char const* const Eol = "\n";

    int constexpr ProbsSeed = 5343584;
    std::ranlux48 probRng(ProbsSeed);

    std::cout << "diagram-id"       << Sep
              << "replication-id"   << Sep
              << "node-count"       << Sep
              << "basic-prob-init[" << unit_str(time_unit()) << "]" << Sep
              << "basic-prob-eval[" << unit_str(time_unit()) << "]" << Sep
              << "sym-prob-init["   << unit_str(time_unit()) << "]" << Sep
              << "sym-prob-eval["   << unit_str(time_unit()) << "]" << Eol;

    for (int diagramId = 0; diagramId < diagramCount; ++diagramId)
    {
        teddy::bss_manager manager(varCount, 1'000'000);
        bdd_t diagram = gen(manager);
        long long const nodeCount = manager.get_node_count(diagram);
        for (int repl = 0; repl < replicationCount; ++repl)
        {
            // ; diagram-id
            std::cout << diagramId << Sep;

            // ; replication-id
            std::cout << repl << Sep;

            // ; node-count
            std::cout << nodeCount << Sep;

            // Basic approach
            {
                auto probs = teddy::tsl::make_time_probability_vector(
                    varCount,
                    probRng
                );

                duration_measurement timeBasic;
                tick(timeBasic);
                double t = 0;
                for (int i = 0; i < timePointCount; ++i)
                {
                    double const A = manager.calculate_availability(
                        teddy::probs::eval_at(probs, t),
                        diagram
                    );
                    ankerl::nanobench::doNotOptimizeAway(A);
                    t += 1;
                }
                tock(timeBasic);

                // ; basic-prob-init
                std::cout << 0 << Sep;

                // ; basic-prob-eval
                std::cout << duration_as<time_unit>(timeBasic) << Sep;
            }

            // Symbolic approach
            {
                auto probs = teddy::symprobs::as_matrix(
                    teddy::tsl::make_time_symprobability_vector(
                        varCount,
                        probRng
                    )
                );

                duration_measurement timeSymbolicInit;
                duration_measurement timeSymbolicEval;
                tick(timeSymbolicInit);
                teddy::symprobs::expression Aexpr =
                    manager.symbolic_availability(
                        1,
                        probs,
                        diagram
                    );
                tock(timeSymbolicInit);

                // ; sym-prob-init
                std::cout << duration_as<time_unit>(timeSymbolicInit) << Sep;

                tick(timeSymbolicEval);
                double t = 0;
                for (int i = 0; i < timePointCount; ++i)
                {
                    double const A = Aexpr.evaluate(t);
                    ankerl::nanobench::doNotOptimizeAway(A);
                    t += 1;
                }
                tock(timeSymbolicEval);

                // ; sym-prob-eval
                std::cout << duration_as<time_unit>(timeSymbolicEval) << Eol;
            }
        }
    }
}

auto main() -> int
{
    int constexpr ReplicationCount = 1;
    int constexpr TimePointCount   = 10;

    {
        int constexpr DiagramCount = 10;
        int constexpr ExprSeed     = 5343584;
        int constexpr VarCount     = 20;
        int constexpr TermCount    = 35;
        int constexpr TermSize     = 7;
        SPDiagramGenerator gen(ExprSeed, VarCount, TermCount, TermSize);
        evalute_system(
            DiagramCount,
            ReplicationCount,
            TimePointCount,
            VarCount,
            gen
        );
    }

    {

    }

    {

    }
}