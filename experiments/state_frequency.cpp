#include <libteddy/reliability.hpp>
#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <nanobench/nanobench.h>
#include <iomanip>
#include <iostream>
#include "utils.hpp"


auto compare (
    int const varCount,
    int const termCount,
    int const termSize,
    int const replCount,
    bool const printHeader,
    std::ranlux48& rng
) -> void
{
    using namespace teddy;
    using bdd_t = bss_manager::diagram_t;
    using duration_t = std::chrono::nanoseconds;

    char const* const Sep = "\t";
    char const* const Eol = "\n";

    if (printHeader)
    {
        std::cout
            << "replication"      << Sep
            << "varCount"         << Sep
            << "nodeCount"        << Sep
            << "freq-naive"       << Sep
            << "freq-log-naive"   << Sep
            << "freq-ours"        << Sep
            << "freq-naive"       << Sep
            << "time-naive["      << utils::unit_str(duration_t()) << "]" << Sep
            << "time-log-naive["  << utils::unit_str(duration_t()) << "]" << Sep
            << "time-ours["       << utils::unit_str(duration_t()) << "]" << Eol;
    }

    std::cout << std::fixed;
    std::cout << std::setprecision(4);

    utils::duration_measurement timeNaive;
    utils::duration_measurement timeLogNaive;
    utils::duration_measurement timeOurs;

    bss_manager manager(varCount, 100'000);
    for (int repl = 0; repl < replCount; ++repl)
    {
        // ; replication
        std::cout << repl << Sep;

        // ; varCount
        std::cout << varCount << Sep;

        tsl::minmax_expr const expression = tsl::make_minmax_expression(
            rng,
            varCount,
            termCount,
            termSize
        );

        bdd_t const diagram = tsl::make_diagram(expression, manager);

        // ; nodeCount
        std::cout << manager.get_node_count(diagram) << Sep;

        if (varCount < 63)
        {
            utils::tick(timeNaive);
            long long const sc = manager.satisfy_count(1, diagram);
            long long const ds = static_cast<long long>(1) << varCount;
            double const td1 = static_cast<double>(sc) / static_cast<double>(ds);
            ankerl::nanobench::doNotOptimizeAway(td1);
            utils::tock(timeNaive);

            // ; freq-naive
            std::cout << td1 << Sep;
        }
        else
        {
            // ; freq-naive
            std::cout << " - " << Sep;
        }

        utils::tick(timeLogNaive);
        double const lnsc = manager.satisfy_count_ln(diagram);
        double const lnds = static_cast<double>(varCount);
        double const td2  = std::pow(2, lnsc - lnds);
        ankerl::nanobench::doNotOptimizeAway(td2);
        utils::tock(timeLogNaive);

        utils::tick(timeOurs);
        double const td3 = manager.state_frequency(diagram, 1);
        ankerl::nanobench::doNotOptimizeAway(td3);
        utils::tock(timeOurs);

        // ; freq-log-naive
        std::cout << td2 << Sep;

        // ; freq-ours
        std::cout << td3 << Sep;

        // ; time-naive
        std::cout << utils::duration_as<duration_t>(timeNaive) << Sep;

        // ; time-log-naive
        std::cout << utils::duration_as<duration_t>(timeLogNaive) << Sep;

        // ; time-ours
        std::cout << utils::duration_as<duration_t>(timeOurs) << Eol;
    }
}

auto main () -> int
{
    std::ranlux48 rng(59486);
    int const Replications = 100;
    auto const VarCounts = {10, 30, 60, 80, 90, 100};
    bool printHeader = true;
    for (int const varCount : VarCounts)
    {
        compare(varCount, 20, 5, Replications, printHeader, rng);
        printHeader = false;
    }
}