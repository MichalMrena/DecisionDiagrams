#include <libteddy/details/pla_file.hpp>
#include <libteddy/reliability.hpp>
#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <libtsl/utilities.hpp>
#include <nanobench/nanobench.h>
#include <bitset>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <random>

#include "utils.hpp"

// CVS parameters
char const* const Sep = "\t";
char const* const Eol = "\n";

// Types
using time_unit = std::chrono::nanoseconds;
using bdd_t = teddy::bss_manager::diagram_t;

template<class F>
void for_each_bdd_vars(long long const varCount, F f)
{
    assert(varCount < 32);
    unsigned long long const Bound = 1ULL << varCount;
    for (unsigned long long vars = 0; vars < Bound; ++vars)
    {
        f(std::bitset<32>(vars));
    }
}

auto div (auto const nom, auto const denom) -> double
{
    return static_cast<double>(nom) / static_cast<double>(denom);
}

inline auto make_time_probability_matrix (
    int const varCount,
    std::ranlux48& rng
) -> std::vector<std::array<teddy::probs::prob_dist, 2>>
{
    std::vector<std::array<teddy::probs::prob_dist, 2>> probs;

    for (int i = 0; i < varCount; ++i)
    {
        std::uniform_real_distribution<double> distRate(0.2, 1.0);
        double const rate = distRate(rng);
        probs.push_back({
            teddy::probs::complemented_exponential(rate),
            teddy::probs::exponential(rate)
        });
    }

    return probs;
}

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

//           +------+
//         +-|  x0  |-+
//         | +------+ |
//     +---+          +----+
//     |   | +------+ |    |
//     |   +-|  x1  |-+    |
//     |     +------+      |
//   o-+                   |-o
//     | +------+ +------+ |
//     +-|  x2  |-|  x3  |-+
//       +------+ +------+
struct FixedDiagramGenerator
{
    auto operator() (teddy::bss_manager& m)
    {
        using namespace teddy::ops;
        auto& x     = m;
        auto top    = m.apply<OR>(x(0), x(1));
        auto bottom = m.apply<AND>(x(2), x(3));
        return m.apply<OR>(top, bottom);
    }
};

struct FixedProbsGenerator
{
    auto operator() (std::ranlux48&, int const)
    {
        using namespace teddy::probs;
        return std::vector<std::array<prob_dist, 2>>
        {
            {exponential(1 / 25.359), complemented_exponential(1 / 25.359)},
            {exponential(1 /  6.246), complemented_exponential(1 /  6.246)},
            {exponential(1 /  4.764), complemented_exponential(1 /  4.764)},
            {exponential(1 / 44.360), complemented_exponential(1 / 44.360)},
        };
    }
};

struct FixedSymprobsGenerator
{
    auto operator() (std::ranlux48&, int const)
    {
        using namespace teddy::symprobs;
        return std::vector<std::array<expression, 2>>
        {
            {exponential(1 / 25.359), complement(exponential(1 / 25.359))},
            {exponential(1 /  6.246), complement(exponential(1 /  6.246))},
            {exponential(1 /  4.764), complement(exponential(1 /  4.764))},
            {exponential(1 / 44.360), complement(exponential(1 / 44.360))},
        };
    }
};

class SPDiagramGenerator
{
public:
    SPDiagramGenerator(
        size_t const seed,
        int const varCount
    ) :
        rng_(seed),
        varCount_(varCount)
    {
    }

    auto operator() (teddy::bss_manager& manager)
    {
        auto const SPExpr = teddy::tsl::make_expression_tree(
            varCount_,
            rng_,
            rng_
        );
        return teddy::tsl::make_diagram(*SPExpr, manager);
    }

private:
    std::ranlux48 rng_;
    int varCount_;
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

struct RandomProbsGenerator
{
    auto operator() (std::ranlux48& rng, int const varCount)
    {
        return make_time_probability_matrix(varCount, rng);
    }
};

struct RandomSymprobsGenerator
{
    auto operator() (std::ranlux48& rng, int const varCount)
    {
        return teddy::symprobs::to_matrix(
            teddy::tsl::make_time_symprobability_vector(varCount, rng)
        );
    }
};

int get_node_count (GiNaC::ex ex)
{
    int count = 1;
    size_t n = ex.nops();
    if (n)
    {
        for (size_t i = 0; i < n; i++)
        {
            count += get_node_count(ex.op(i));
        }
    }
    return count;
}

template<class DiagramGenerator, class ProbsGenerator, class SymprobsGenerator>
auto evalute_system (
    int const diagramCount,
    int const replicationCount,
    int const timePointCount,
    int const varCount,
    bool const printHeader,
    DiagramGenerator& diagramGen,
    ProbsGenerator& probsGen,
    SymprobsGenerator& symprobsGen
) -> void
{
    using namespace teddy::utils;

    // Time parameters
    double const TimeZero  = 1;
    double const TimeDelta = 0.01;

    int constexpr ProbsSeed = 5343584;
    std::ranlux48 probRng1(ProbsSeed);
    std::ranlux48 probRng2(ProbsSeed);

    if (printHeader)
    {
        std::cout << "diagram-id"       << Sep
                  << "replication-id"   << Sep
                  << "variable-count"   << Sep
                  << "diagram-nodes"    << Sep
                  << "time-pt-count"    << Sep
                  << "basic-prob-init[" << unit_str(time_unit()) << "]" << Sep
                  << "basic-prob-eval[" << unit_str(time_unit()) << "]" << Sep
                  << "sym-prob-init["   << unit_str(time_unit()) << "]" << Sep
                  << "tree-nodes"       << Sep
                  << "sym-prob-eval["   << unit_str(time_unit()) << "]" << Eol;
    }

    for (int diagramId = 0; diagramId < diagramCount; ++diagramId)
    {
        teddy::bss_manager manager(varCount, 1'000'000);
        bdd_t diagram = diagramGen(manager);
        long long const nodeCount = manager.get_node_count(diagram);
        for (int repl = 0; repl < replicationCount; ++repl)
        {
            // ; diagram-id
            std::cout << diagramId << Sep;

            // ; replication-id
            std::cout << repl << Sep;

            // ; variable-count
            std::cout << varCount << Sep;

            // ; node-count
            std::cout << nodeCount << Sep;

            // ; time-pt-count
            std::cout << timePointCount << Sep;

            // Basic approach
            {
                auto probs = probsGen(probRng1, varCount);

                duration_measurement timeBasic;
                tick(timeBasic);
                double t = TimeZero;
                for (int i = 0; i < timePointCount; ++i)
                {
                    double const A = manager.calculate_availability(
                        1,
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
                auto symprobs = symprobsGen(probRng2, varCount);

                duration_measurement timeSymbolicInit;
                duration_measurement timeSymbolicEval;
                tick(timeSymbolicInit);
                teddy::symprobs::expression Aexpr =
                    manager.symbolic_availability(
                        1,
                        symprobs,
                        diagram
                    );
                tock(timeSymbolicInit);

                std::cout << "===";
                Aexpr.to_matlab(std::cout);
                std::cout << "===";

                // ; sym-prob-init
                std::cout << duration_as<time_unit>(timeSymbolicInit) << Sep;

                // ; tree-nodes
                auto expr = Aexpr.as_underlying_unsafe();
                std::cout << get_node_count(expr) << Sep;

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

auto print_header () -> void
{
    using namespace teddy::utils;
    std::cout << "diagram-id"       << Sep
              << "replication-id"   << Sep
              << "variable-count"   << Sep
              << "diagram-nodes"    << Sep
              << "time-pt-count"    << Sep
              << "basic-prob-init[" << unit_str(time_unit()) << "]" << Sep
              << "basic-prob-eval[" << unit_str(time_unit()) << "]" << Sep
              << "sym-prob-init["   << unit_str(time_unit()) << "]" << Sep
              << "tree-nodes"       << Sep
              << "sym-prob-eval["   << unit_str(time_unit()) << "]" << Eol;
}

auto analyze_pla (
    std::string const& path,
    bool const printHeader,
    std::ranlux48& probRng1,
    std::ranlux48& probRng2,
    int const replicationCount,
    int const timePointCount
) -> void
{
    using namespace teddy::utils;

    auto fileOpt = teddy::pla_file::load_file(path);
    if (not fileOpt)
    {
        std::cerr << "Failed to load PLA file -- " << path << "\n";
        std::exit(1);
    }

    double const TimeZero      = 1;
    double const TimeDelta     = 0.01;
    int const inputCount       = fileOpt->get_variable_count();
    int const lineCount        = (int)fileOpt->get_line_count();
    int const functionCount    = fileOpt->get_function_count();
    int const varCount         = inputCount + lineCount + functionCount;

    PLADiagramGenerator diagramGen(*fileOpt);
    RandomProbsGenerator probsGen;
    RandomSymprobsGenerator symprobsGen;

    if (printHeader)
    {
        std::cout
            << "pla-file"         << Sep
            << "replication-id"   << Sep
            << "variable-count"   << Sep
            << "time-pt-count"    << Sep
            << "diagram-nodes"    << Sep
            << "tree-nodes"       << Sep
            << "basic-prob-eval[" << unit_str(time_unit()) << "]" << Sep
            << "sym-prob-init["   << unit_str(time_unit()) << "]" << Sep
            << "sym-prob-eval["   << unit_str(time_unit()) << "]" << Eol;
    }

    std::ios_base::fmtflags const outFlags = std::cout.flags();

    teddy::bss_manager manager(varCount, 1'000'000);
    bdd_t diagram = diagramGen(manager);
    for (int repl = 0; repl < replicationCount; ++repl)
    {
        // ; pla-file
        std::cout << std::filesystem::path(path).stem() << Sep;

        // ; replication-id
        std::cout << repl << Sep;

        // ; variable-count
        std::cout << varCount << Sep;

        // ; time-pt-count
        std::cout << timePointCount << Sep;

        duration_measurement timeBasic;
        duration_measurement timeSymbolicInit;
        duration_measurement timeSymbolicEval;

        long long diagramNodeCount = 0;
        long long exprNodeCount    = 0;

        auto probs    = probsGen(probRng1, varCount);
        auto symprobs = symprobsGen(probRng2, varCount);

        for_each_bdd_vars(inputCount, [&](auto const& vars)
        {
            std::vector<teddy::var_cofactor> cofactoredVars;
            cofactoredVars.reserve(1LL << inputCount);
            for (int i = 0; i < inputCount; ++i)
            {
                cofactoredVars.push_back({i, vars[(size_t)i]});
            }
            bdd_t sf = manager.get_cofactor(diagram, cofactoredVars);

            diagramNodeCount += manager.get_node_count(sf);

            // Basic approach
            {
                tick(timeBasic);
                double t = TimeZero;
                for (int i = 0; i < timePointCount; ++i)
                {
                    double const A = manager.calculate_availability(
                        1,
                        teddy::probs::eval_at(probs, t),
                        sf
                    );
                    ankerl::nanobench::doNotOptimizeAway(A);
                    t += TimeDelta;
                }
                tock(timeBasic);
            }

            // Symbolic approach
            {
                tick(timeSymbolicInit);
                teddy::symprobs::expression Aexpr =
                    manager.symbolic_availability(
                        1,
                        symprobs,
                        sf
                    );
                tock(timeSymbolicInit);

                exprNodeCount += get_node_count(Aexpr.as_underlying_unsafe());

                tick(timeSymbolicEval);
                double t = TimeZero;
                for (int i = 0; i < timePointCount; ++i)
                {
                    double const A = Aexpr.evaluate(t);
                    ankerl::nanobench::doNotOptimizeAway(A);
                    t += TimeDelta;
                }
                tock(timeSymbolicEval);
            }
        });

        long long const denom = 1 << inputCount;

        // Four places for counts
        std::cout << std::fixed;
        std::cout << std::setprecision(4);

        // ; diagram-nodes
        std::cout << div(diagramNodeCount, denom) << Sep;

        // ; tree-nodes
        std::cout << div(exprNodeCount, denom) << Sep;

        // No places for durations
        std::cout << std::setprecision(0);

        // ; basic-prob-eval
        std::cout << div(duration_as<time_unit>(timeBasic), denom) << Sep;

        // ; sym-prob-init
        std::cout << div(duration_as<time_unit>(timeSymbolicInit), denom) << Sep;

        // ; sym-prob-eval
        std::cout << div(duration_as<time_unit>(timeSymbolicEval), denom) << Eol;

        // Reset flags
        std::cout.flags(outFlags);
    }
}

auto run_analyze_pla () -> void
{
    auto const files =
    {
        "/home/michal/data/IWLS93/pla/con1.pla",
        "/home/michal/data/IWLS93/pla/xor5.pla",
        "/home/michal/data/IWLS93/pla/rd53.pla",
        "/home/michal/data/IWLS93/pla/squar5.pla",
        "/home/michal/data/IWLS93/pla/sqrt8.pla",
    };

    int const ReplicationCount = 1;
    int const ProbsSeed        = 5343584;
    auto const TimePoinCounts  = {10, 100, 1'000, 10'000};
    std::ranlux48 probRng1(ProbsSeed);
    std::ranlux48 probRng2(ProbsSeed);

    bool printHeader = true;
    for (auto const& file : files)
    {
        for (int const timePointCount : TimePoinCounts)
        {
            analyze_pla(
                file,
                printHeader,
                probRng1,
                probRng2,
                ReplicationCount,
                timePointCount
            );
            printHeader = false;
        }
    }
}

auto main() -> int
{
    run_analyze_pla();

    // SystemType const SystemType = SystemType::FIXED;

    // if (SystemType == SystemType::FIXED)
    // {
    //     int const ReplicationCount = 1;
    //     int const DiagramCount     = 1;
    //     int const VarCount         = 4;
    //     // auto const TimePointCounts = {10, 100, 1000, 10'000};
    //     auto const TimePointCounts = {10};
    //     bool printHeader           = true;
    //     FixedDiagramGenerator diagramGen;
    //     FixedProbsGenerator probsGen;
    //     FixedSymprobsGenerator symprobsGen;
    //     for (int const timePointCount : TimePointCounts)
    //     {
    //         evalute_system(
    //             DiagramCount,
    //             ReplicationCount,
    //             timePointCount,
    //             VarCount,
    //             printHeader,
    //             diagramGen,
    //             probsGen,
    //             symprobsGen
    //         );
    //         printHeader = false;
    //     }
    // }

    // if (SystemType == SystemType::SERIES_PARALLEL)
    // {
    //     int const ReplicationCount = 10;
    //     int const DiagramCount     = 20;
    //     int const ExprSeed         = 5343584;
    //     int const TimePointCount   = 10;
    //     auto const VarCounts       = {10, 20, 30, 40};
    //     bool printHeader           = true;
    //     RandomProbsGenerator probsGen;
    //     RandomSymprobsGenerator symprobsGen;
    //     for (int const varCount : VarCounts)
    //     {
    //         SPDiagramGenerator diagram(ExprSeed, varCount);
    //         evalute_system(
    //             DiagramCount,
    //             ReplicationCount,
    //             TimePointCount,
    //             varCount,
    //             printHeader,
    //             diagram,
    //             probsGen,
    //             symprobsGen
    //         );
    //         printHeader = false;
    //     }
    // }

    // if (SystemType == SystemType::PLA)
    // {
    //     auto const path = "/home/michal/data/IWLS93/pla/con1.pla";
    //     auto fileOpt = teddy::pla_file::load_file(path);
    //     if (not fileOpt)
    //     {
    //         std::cerr << "Failed to load PLA file.\n";
    //         return 1;
    //     }
    //     int const ReplicationCount = 10;
    //     int const DiagramCount     = 1;
    //     int const TimePointCount   = 10;
    //     int const inputCount       = fileOpt->get_variable_count();
    //     int const lineCount        = (int)fileOpt->get_line_count();
    //     int const functionCount    = fileOpt->get_function_count();
    //     int const varCount         = inputCount + lineCount + functionCount;
    //     PLADiagramGenerator diagramGen(*fileOpt);
    //     RandomProbsGenerator probsGen;
    //     RandomSymprobsGenerator symprobsGen;
    //     evalute_system(
    //         DiagramCount,
    //         ReplicationCount,
    //         TimePointCount,
    //         varCount,
    //         true,
    //         diagramGen,
    //         probsGen,
    //         symprobsGen
    //     );
    // }
}
