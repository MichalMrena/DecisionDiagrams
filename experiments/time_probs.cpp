#include <libteddy/impl/pla_file.hpp>
#include <libteddy/inc/reliability.hpp>
#include <libteddy/inc/io.hpp>
#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <libtsl/utilities.hpp>
#include <lib/nanobench/nanobench.h>
#include <bitset>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <random>

#include "utils.hpp"

// CVS parameters
char const* const Sep = ";";
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

auto fdiv (auto const nom, auto const denom) -> double
{
    return static_cast<double>(nom) / static_cast<double>(denom);
}

inline auto make_time_probability_matrix (
    int const varCount,
    std::ranlux48& rng
) -> std::vector<std::array<teddy::probs::prob_dist, 2>>
{
    using namespace teddy::probs;
    std::vector<std::array<prob_dist, 2>> probs;
    for (int i = 0; i < varCount; ++i)
    {
        std::uniform_real_distribution<double> distRate(0.2, 1.0);
        double const rate = distRate(rng);
        probs.push_back(std::array<prob_dist, 2>{
            prob_dist(complemented_exponential(rate)),
            prob_dist(exponential(rate))
        });
    }
    return probs;
}

auto sf_from_pla (
    teddy::pla_file const& file,
    teddy::bss_manager& manager
) -> teddy::bss_manager::diagram_t
{
    auto const& plaLines          = file.get_lines();
    long long const inputCount    = file.get_variable_count();
    long long const lineCount     = file.get_line_count();
    long long const functionCount = file.get_function_count();

    using namespace teddy::ops;
    using std::size_t;
    // using bdd_t = teddy::bdd_manager::diagram_t;

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

/**
 *           +------+
 *         +-|  x0  |-+
 *         | +------+ |
 *     +---+          +----+
 *     |   | +------+ |    |
 *     |   +-|  x1  |-+    |
 *     |     +------+      |
 *   o-+                   |-o
 *     | +------+ +------+ |
 *     +-|  x2  |-|  x3  |-+
 *       +------+ +------+
 */
struct FixedSystem
{
    static auto make_bdd (teddy::bss_manager& m)
    {
        using namespace teddy::ops;
        auto& x     = m;
        auto top    = m.apply<OR>(x(0), x(1));
        auto bottom = m.apply<AND>(x(2), x(3));
        return m.apply<OR>(top, bottom);
    }

    static auto make_probs ()
    {
        using namespace teddy::probs;
        return std::vector<std::array<prob_dist, 2>>
        {
        {exponential(1.0 / 25'359.0), complemented_exponential(1.0 / 25'359.0)},
        {exponential(1.0 /  6'246.0), complemented_exponential(1.0 /  6'246.0)},
        {exponential(1.0 /  4'764.0), complemented_exponential(1.0 /  4'764.0)},
        {exponential(1.0 / 44'360.0), complemented_exponential(1.0 / 44'360.0)},
        };
    }

    static auto make_symprobs ()
    {
        using namespace teddy::symprobs;
        return std::vector<std::array<expression, 2>>
        {
        {exponential(1.0 / 25'359.0), complement(exponential(1.0 / 25'359.0))},
        {exponential(1.0 /  6'246.0), complement(exponential(1.0 /  6'246.0))},
        {exponential(1.0 /  4'764.0), complement(exponential(1.0 /  4'764.0))},
        {exponential(1.0 / 44'360.0), complement(exponential(1.0 / 44'360.0))},
        };
    }

    static auto describe ()
    {
        using namespace teddy;
        teddy::bss_manager manager(4, 1'000);
        bdd_t sf = FixedSystem::make_bdd(manager);

        GiNaC::realsymbol p1("p_1");
        GiNaC::realsymbol p2("p_2");
        GiNaC::realsymbol p3("p_3");
        GiNaC::realsymbol p4("p_4");

        GiNaC::realsymbol q1("q_1");
        GiNaC::realsymbol q2("q_2");
        GiNaC::realsymbol q3("q_3");
        GiNaC::realsymbol q4("q_4");

        std::vector<std::array<symprobs::expression, 2>> symprobs
        {
            {symprobs::expression(q1), symprobs::expression(p1)},
            {symprobs::expression(q2), symprobs::expression(p2)},
            {symprobs::expression(q3), symprobs::expression(p3)},
            {symprobs::expression(q4), symprobs::expression(p4)},
        };

        symprobs::expression expr = manager.symbolic_availability(
            1,
            symprobs,
            sf
        );

        expr.to_latex(std::cout);
    }
};

int get_node_count (GiNaC::ex ex)
{
    int count = 1;
    std::size_t n = ex.nops();
    if (n)
    {
        for (std::size_t i = 0; i < n; i++)
        {
            count += get_node_count(ex.op(i));
        }
    }
    return count;
}

auto analyze_fixed (
    int const replicationCount,
    int const timePointCount,
    bool const printHeader
) -> void
{
    using namespace teddy;
    using namespace teddy::ops;
    using namespace teddy::utils;
    teddy::bss_manager manager(4, 1'000);
    bdd_t sf      = FixedSystem::make_bdd(manager);
    auto probs    = FixedSystem::make_probs();
    auto symprobs = FixedSystem::make_symprobs();
    auto expr     = manager.symbolic_availability(1, symprobs, sf);

    if (printHeader)
    {
        // Structure function BDD
        // std::cout << "DOT:" << "\n";
        // io::to_dot(manager, std::cout, sf);
        // std::cout << "\n";

        // // Reliability expression
        // std::cout << "Expression:" << "\n";
        // expr.to_matlab(std::cout);
        // std::cout << "\n";

        // CSV header
        // std::cout << "replication_id"   << Sep
        //           << "variable_count"   << Sep
        //           << "diagram_nodes"    << Sep
        //           << "tree_nodes"       << Sep
        //           << "time_pt_count"    << Sep
        //           << "basic_prob_eval"  << Sep
        //           << "sym_prob_init"    << Sep
        //           << "sym_prob_eval"    << Eol;
    }

    // Time parameters
    double const TimeZero  = 1;
    double const TimeDelta = 0.01;

    long long const diagramNodes = manager.get_node_count(sf);
    long long const treeNodes    = get_node_count(expr.as_underlying_unsafe());
    for (int repl = 0; repl < replicationCount; ++repl)
    {
        // ; replication-id
        // std::cout << repl << Sep;

        // ; variable-count
        // std::cout << manager.get_var_count() << Sep;

        // ; diagram-nodes
        // std::cout << diagramNodes << Sep;

        // ; tree-nodes
        // std::cout << treeNodes << Sep;

        // ; time-pt-count
        // std::cout << timePointCount << Sep;

        // Basic approach
        {
            duration_measurement timeBasic;
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

            // ; basic-prob-eval
            // std::cout << duration_as<time_unit>(timeBasic) << Sep;
        }

        // Symbolic approach
        {
            duration_measurement timeSymbolicInit;
            duration_measurement timeSymbolicEval;
            tick(timeSymbolicInit);
            teddy::symprobs::expression Aexpr =
                manager.symbolic_availability(
                    1,
                    symprobs,
                    sf
                );
            tock(timeSymbolicInit);

            // ; sym-prob-init
            // std::cout << duration_as<time_unit>(timeSymbolicInit) << Sep;

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
            // std::cout << duration_as<time_unit>(timeSymbolicEval) << Eol;
        }
    }

}

auto analyze_random_sp (
    int const diagramCount,
    int const replicationCount,
    int const timePointCount,
    int const varCount,
    bool const printHeader,
    std::ranlux48& exprRng,
    std::ranlux48& probRng1,
    std::ranlux48& probRng2
) -> void
{
    using namespace teddy::utils;

    // Time parameters
    double const TimeZero  = 1;
    double const TimeDelta = 0.01;

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
        auto expr = teddy::tsl::make_expression_tree(varCount, exprRng, exprRng);
        bdd_t diagram = teddy::tsl::make_diagram(*expr, manager);
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
            auto probs = make_time_probability_matrix(varCount, probRng1);

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

            // Symbolic approach
            auto symprobs = teddy::symprobs::to_matrix(
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
                    symprobs,
                    diagram
                );
            tock(timeSymbolicInit);

            // ; sym-prob-init
            std::cout << duration_as<time_unit>(timeSymbolicInit) << Sep;

            // ; tree-nodes
            std::cout << get_node_count(Aexpr.as_underlying_unsafe()) << Sep;

            tick(timeSymbolicEval);
            t = TimeZero;
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
    bdd_t diagram = sf_from_pla(*fileOpt, manager);
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

        auto probs = make_time_probability_matrix(varCount, probRng1);
        auto symprobs = teddy::symprobs::to_matrix(
                teddy::tsl::make_time_symprobability_vector(
                    varCount,
                    probRng2
                )
            );

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
        std::cout << fdiv(diagramNodeCount, denom) << Sep;

        // ; tree-nodes
        std::cout << fdiv(exprNodeCount, denom) << Sep;

        // No places for durations
        std::cout << std::setprecision(0);

        // ; basic-prob-eval
        std::cout << fdiv(duration_as<time_unit>(timeBasic), denom) << Sep;

        // ; sym-prob-init
        std::cout << fdiv(duration_as<time_unit>(timeSymbolicInit), denom) << Sep;

        // ; sym-prob-eval
        std::cout << fdiv(duration_as<time_unit>(timeSymbolicEval), denom) << Eol;

        // Reset flags
        std::cout.flags(outFlags);
    }
}

auto run_analyze_fixed () -> void
{
    int const ReplicationCount = 1000;
    // auto const TimePoinCounts  = {10, 100, 1'000, 10'000};
    auto const TimePoinCounts  = {10'000};
    bool printHeader = true;
    for (int const timePointCount : TimePoinCounts)
    {
        analyze_fixed(ReplicationCount, timePointCount, printHeader);
        printHeader = false;
    }
}

auto run_analyzed_random_sp () -> void
{
    int const DiagramCount = 10;
    int const ReplicationCount = 10;
    int const TimePointCount = 10;
    auto const VarCounts = {10, 20, 30, 40};
    std::ranlux48 exprRng(144);
    std::ranlux48 probRng1(314);
    std::ranlux48 probRng2(278);
    bool printHeader = true;
    for (int const varCount : VarCounts)
    {
        analyze_random_sp(
            DiagramCount,
            ReplicationCount,
            TimePointCount,
            varCount,
            printHeader,
            exprRng,
            probRng1,
            probRng2
        );
        printHeader = false;
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

    int const ReplicationCount = 100;
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
    FixedSystem::describe();
    run_analyze_fixed();
    // run_analyze_pla();
    // run_analyzed_random_sp();
}
