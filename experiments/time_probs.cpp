#include <libteddy/details/pla_file.hpp>
#include <libteddy/reliability.hpp>
#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <libtsl/utilities.hpp>
#include <nanobench/nanobench.h>
#include <iostream>
#include <random>

#include "utils.hpp"

//                +------+
//              +-|  x1  |-+
//              | +------+ | +------+
//            +-+          +-|  x3  |----------+
//            | | +------+ | +------+          |   +------+
//            | +-|  x2  |-+                   | +-|  x8  |-+
//   +------+ |   +------+                     | | +------+ |
// o-|  x0  |-+                                +-+          +-o
//   +------+ |                     +------+   | | +------+ |
//            |                   +-|  x6  |-+ | +-|  x9  |-+
//            | +------+ +------+ | +------+ | |   +------+
//            +-|  x4  |-|  x5  |-+          +-+
//              +------+ +------+ | +------+ |
//                                +-|  x7  |-+
//                                  +------+
struct FixedDiagramGenerator
{
    auto operator() (teddy::bss_manager& m)
    {
        using namespace teddy::ops;
        auto& x    = m;
        auto fst   = x(0);
        auto snd_1 = m.apply_n<AND>(m.apply<OR>(x(1), x(2)), x(3));
        auto snd_2 = m.apply_n<AND>(x(4), x(5), m.apply<OR>(x(6), x(7)));
        auto snd   = m.apply<OR>(snd_1, snd_2);
        auto thd   = m.apply<OR>(x(8), x(9));
        return m.apply_n<AND>(fst, snd, thd);
    }
};

class SPDiagramGenerator
{
public:
    SPDiagramGenerator(
        size_t const seed,
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

class PLADiagramGenerator
{
public:
    PLADiagramGenerator(teddy::pla_file file) :
        file_(std::move(file))
    {
    }

    auto operator() (teddy::bss_manager& manager)
    {
        auto const& plaLines          = file_.get_lines();
        long long const inputCount    = file_.get_variable_count();
        long long const lineCount     = file_.get_line_count();
        long long const functionCount = file_.get_function_count();

        using namespace teddy::ops;
        using bdd_t = teddy::bss_manager::diagram_t;
        using std::size_t;

        std::vector<bdd_t> inputs;
        std::vector<bdd_t> andGates;
        std::vector<bdd_t> orGates;

        {
            int i = 0;
            // Variables for inputs
            for (int k = 0; k < inputCount; ++k)
            {
                inputs.push_back(manager.variable(i++));
            }

            // Variables for and gates
            for (int k = 0; k < lineCount; ++k)
            {
                andGates.push_back(manager.variable(i++));
            }

            // Variables for or gates
            for (int k = 0; k < functionCount; ++k)
            {
                orGates.push_back(manager.variable(i++));
            }
        }

        int nextOrVar = 0;
        bdd_t failedAnd = manager.constant(0);
        bdd_t failedOr  = manager.constant(1);

        // BDDs for output functions
        std::vector<bdd_t> functions;
        // BDDs for output functions including unreliable gates
        std::vector<bdd_t> functionsRel;
        functions.reserve(static_cast<size_t>(functionCount));
        functionsRel.reserve(static_cast<size_t>(functionCount));

        for (int fi = 0; fi < functionCount; ++fi)
        {
            int nextAndVar = -1;
            std::vector<bdd_t> products;
            std::vector<bdd_t> productsRel;
            products.reserve(static_cast<size_t>(lineCount));
            productsRel.reserve(static_cast<size_t>(lineCount));

            for (size_t li = 0; li < static_cast<size_t>(lineCount); ++li)
            {
                ++nextAndVar;
                if (plaLines[li].fVals_.get(fi) != 1)
                {
                    continue;
                }

                teddy::bool_cube const& cube = plaLines[li].cube_;
                bdd_t product = manager.constant(1);
                for (int i = 0; i < cube.size(); ++i)
                {
                    if (cube.get(i) == 1)
                    {
                        product = manager.apply<AND>(
                            product,
                            inputs[static_cast<size_t>(i)]
                        );
                    }
                    else if (cube.get(i) == 0)
                    {
                        product = manager.apply<AND>(
                            product,
                            manager.negate(inputs[static_cast<size_t>(i)])
                        );
                    }
                }

                bdd_t productRel = manager.apply<OR>(
                    manager.apply<AND>(andGates[static_cast<size_t>(nextAndVar)], product),
                    manager.apply<AND>(manager.negate(andGates[static_cast<size_t>(nextAndVar)]), failedAnd)
                );

                products.push_back(product);
                productsRel.push_back(productRel);
            }

            if (products.empty())
            {
                products.push_back(manager.constant(0));
            }

            bdd_t sum = manager.constant(0);
            for (bdd_t const& product : products)
            {
                sum = manager.apply<OR>(sum, product);
            }

            bdd_t sumRel = manager.apply<OR>(
                manager.apply<AND>(orGates[static_cast<size_t>(nextOrVar)], sum),
                manager.apply<AND>(manager.negate(orGates[static_cast<size_t>(nextOrVar)]), failedOr)
            );
            ++nextOrVar;

            functions.push_back(sum);
            functionsRel.push_back(sumRel);
        }

        // BDD for output structure functions
        std::vector<bdd_t> structureFunctions;
        structureFunctions.reserve(static_cast<size_t>(functionCount));
        for (size_t i = 0; i < static_cast<size_t>(functionCount); ++i)
        {
            structureFunctions.push_back(
                manager.apply<EQUAL_TO>(functions[i], functionsRel[i])
            );
        }

        bdd_t structureFunction = manager.tree_fold<AND>(structureFunctions);
        return structureFunction;
    }

private:
    teddy::pla_file file_;
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

    // Time parameters
    double const TimeZero  = 1;
    double const TimeDelta = 1;

    int constexpr ProbsSeed = 5343584;
    std::ranlux48 probRng1(ProbsSeed);
    std::ranlux48 probRng2(ProbsSeed);

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
                    probRng1
                );

                duration_measurement timeBasic;
                tick(timeBasic);
                double t = TimeZero;
                for (int i = 0; i < timePointCount; ++i)
                {
                    double const A = manager.calculate_availability(
                        teddy::probs::eval_at(probs, t),
                        diagram
                    );
                    ankerl::nanobench::doNotOptimizeAway(A);
                    t += TimeDelta;
                }
                tock(timeBasic);

                // ; basic-prob-init
                std::cout << 0 << Sep;

                // ; basic-prob-eval
                std::cout << duration_as<time_unit>(timeBasic) << Sep;
            }

            // Symbolic approach
            {
                auto probs = teddy::symprobs::to_matrix(
                    teddy::tsl::make_time_symprobability_vector(
                        varCount,
                        probRng2
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
                double t = TimeZero;
                for (int i = 0; i < timePointCount; ++i)
                {
                    double const A = Aexpr.evaluate(t);
                    ankerl::nanobench::doNotOptimizeAway(A);
                    t += TimeDelta;
                }
                tock(timeSymbolicEval);

                // ; sym-prob-eval
                std::cout << duration_as<time_unit>(timeSymbolicEval) << Eol;
            }
        }
    }
}

enum class SystemType
{
    FIXED,
    SERIES_PARALLEL,
    PLA
};

auto main() -> int
{
    SystemType const systemType = SystemType::PLA;
    int constexpr ReplicationCount = 10;
    int constexpr TimePointCount   = 10;

    if (systemType == SystemType::FIXED)
    {
        int constexpr DiagramCount = 1;
        int constexpr VarCount     = 10;
        FixedDiagramGenerator gen;
        evalute_system(
            DiagramCount,
            ReplicationCount,
            TimePointCount,
            VarCount,
            gen
        );
    }

    if (systemType == SystemType::SERIES_PARALLEL)
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

    if (systemType == SystemType::PLA)
    {
        auto const path = "/home/michal/data/IWLS93/pla/con1.pla";
        auto fileOpt = teddy::pla_file::load_file(path);
        if (not fileOpt)
        {
            std::cerr << "Failed to load PLA file.\n";
            return 1;
        }
        int constexpr DiagramCount = 1;
        int const inputCount    = fileOpt->get_variable_count();
        int const lineCount     = (int)fileOpt->get_line_count();
        int const functionCount = fileOpt->get_function_count();
        int const varCount      = inputCount + lineCount + functionCount;
        PLADiagramGenerator gen(*fileOpt);
        evalute_system(
            DiagramCount,
            ReplicationCount,
            TimePointCount,
            varCount,
            gen
        );
    }
}