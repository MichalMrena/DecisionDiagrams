#include <gmpxx.h>
#include <libteddy/inc/reliability.hpp>
#include <libteddy/inc/io.hpp>
#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <lib/nanobench/nanobench.h>
#include <iomanip>
#include <iostream>
#include "utils.hpp"

char const* const Sep = ";";
char const* const Eol = "\n";

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
    using duration_t = std::chrono::nanoseconds;

    if (printHeader)
    {
        std::cout
            << "replication"      << Sep
            << "varCount"         << Sep
            << "nodeCount"        << Sep
            << "freq-naive"       << Sep
            << "freq-log-naive"   << Sep
            << "freq-ours"        << Sep
            << "time-naive"       << Sep
            << "time-log-naive"   << Sep
            << "time-ours"        << Sep
            << "units"            << Eol;
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

        utils::clear(timeNaive);
        utils::tick(timeNaive);
        mpz_class sc  = manager.satisfy_count(1, diagram);
        mpz_class ds  = teddy::longint(1) << as_usize(varCount);
        mpf_class td1 = mpf_class(sc) / mpf_class(ds);
        ankerl::nanobench::doNotOptimizeAway(td1);
        utils::tock(timeNaive);

        utils::clear(timeLogNaive);
        utils::tick(timeLogNaive);
        double const lnsc = manager.satisfy_count_ln(diagram);
        double const lnds = static_cast<double>(varCount);
        double const td2  = std::pow(2, lnsc - lnds);
        ankerl::nanobench::doNotOptimizeAway(td2);
        utils::tock(timeLogNaive);

        utils::clear(timeOurs);
        utils::tick(timeOurs);
        double const td3 = manager.state_frequency(diagram, 1);
        ankerl::nanobench::doNotOptimizeAway(td3);
        utils::tock(timeOurs);

        // ; freq-naive
        std::cout << td1 << Sep;

        // ; freq-log-naive
        std::cout << td2 << Sep;

        // ; freq-ours
        std::cout << td3 << Sep;

        // ; time-naive
        std::cout << utils::duration_as<duration_t>(timeNaive) << Sep;

        // ; time-log-naive
        std::cout << utils::duration_as<duration_t>(timeLogNaive) << Sep;

        // ; time-ours
        std::cout << utils::duration_as<duration_t>(timeOurs) << Sep;

        // ; units
        std::cout << utils::unit_str(duration_t()) << Eol;
    }
}

auto run_compare()
{
    std::ranlux48 rng(59486);
    int const Replications = 200;
    auto const VarCounts = {10, 30, 60, 80, 90, 100};
    bool printHeader = true;
    for (int const varCount : VarCounts)
    {
        compare(varCount, 20, 5, Replications, printHeader, rng);
        printHeader = false;
    }
}

auto compare_pla(
    std::string const& path,
    int const replications,
    bool const printHeader,
    std::ostream& ost
)
{
    using namespace teddy;
    using duration_t = std::chrono::nanoseconds;

    std::optional<pla_file> fileOpt = pla_file::load_file(path, false);
    if (not fileOpt.has_value())
    {
        std::exit(1);
    }

    if (printHeader)
    {
        ost
            << "function"         << Sep
            << "nodeCount"        << Sep
            << "replication"      << Sep
            << "freq-naive"       << Sep
            << "freq-log-naive"   << Sep
            << "freq-ours"        << Sep
            << "time-naive"       << Sep
            << "time-log-naive"   << Sep
            << "time-ours"        << Sep
            << "units"            << Eol;
    }

    pla_file const& file = *fileOpt;
    bss_manager manager(file.get_variable_count(), 100'000);
    std::vector<bdd_t> const& diagrams = io::from_pla(manager, file);

    ost << std::fixed;
    ost << std::setprecision(4);

    utils::duration_measurement timeNaive;
    utils::duration_measurement timeLogNaive;
    utils::duration_measurement timeOurs;

    for (size_t funcIndex = 0; funcIndex < diagrams.size(); ++funcIndex)
    {
        bdd_t const diagram   = diagrams[funcIndex];
        int32 const varCount  = file.get_variable_count();
        int64 const nodeCount = manager.get_node_count(diagram);

        // TODO(michal): refactor duplicity
        for (int repl = 0; repl < replications; ++repl)
        {
            // ; function
            ost << funcIndex << Sep;

            // ; nodeCount
            ost << nodeCount << Sep;

            // ; replication
            ost << repl << Sep;

            utils::clear(timeNaive);
            utils::tick(timeNaive);
            mpz_class sc  = manager.satisfy_count(1, diagram);
            mpz_class ds  = teddy::longint(1) << as_usize(varCount);
            mpf_class td1 = mpf_class(sc) / mpf_class(ds);
            ankerl::nanobench::doNotOptimizeAway(td1);
            utils::tock(timeNaive);

            utils::clear(timeLogNaive);
            utils::tick(timeLogNaive);
            double const lnsc = manager.satisfy_count_ln(diagram);
            double const lnds = static_cast<double>(varCount);
            double const td2  = std::pow(2, lnsc - lnds);
            ankerl::nanobench::doNotOptimizeAway(td2);
            utils::tock(timeLogNaive);

            utils::clear(timeOurs);
            utils::tick(timeOurs);
            double const td3 = manager.state_frequency(diagram, 1);
            ankerl::nanobench::doNotOptimizeAway(td3);
            utils::tock(timeOurs);

            // if (td1 != td2)
            // {
            //     std::cerr << std::format("td1 /= td2: {} /= {}\n", td1.get_d(), td2);
            // }

            // if (td2 != td3)
            // {
            //     std::cerr << std::format("td2 /= td3: {} /= {}\n", td2, td3);
            // }

            // if (td1 != td3)
            // {
            //     std::cerr << std::format("td1 /= td3: {} /= {}\n", td1.get_d(), td3);
            // }

            // ; freq-naive
            ost << td1 << Sep;

            // ; freq-log-naive
            ost << td2 << Sep;

            // ; freq-ours
            ost << td3 << Sep;

            // ; time-naive
            ost << utils::duration_as<duration_t>(timeNaive) << Sep;

            // ; time-log-naive
            ost << utils::duration_as<duration_t>(timeLogNaive) << Sep;

            // ; time-ours
            ost << utils::duration_as<duration_t>(timeOurs) << Sep;

            // ; units
            ost << utils::unit_str(duration_t()) << Eol;
        }
    }
}

auto run_compare_pla()
{
    auto const files = {
        "5xp1.pla",
        "9sym.pla",
        "alu4.pla",
        "apex1.pla",
        "apex2.pla",
        // "apex3.pla",
        "apex4.pla",
        "apex5.pla",
        "b12.pla",
        "bw.pla",
        "clip.pla",
        "con1.pla",
        "cordic.pla",
        "cps.pla",
        "duke2.pla",
        "e64.pla",
        "ex1010.pla",
        "ex4p.pla",
        "ex5p.pla",
        "inc.pla",
        "misex1.pla",
        "misex2.pla",
        "misex3.pla",
        "misex3c.pla",
        // "o64.pla",
        "pdc.pla",
        "rd53.pla",
        "rd73.pla",
        "rd84.pla",
        "sao2.pla",
        "seq.pla",
        "spla.pla",
        "sqrt8.pla",
        "squar5.pla",
        "t481.pla",
        "table3.pla",
        "table5.pla",
        "vg2.pla",
        "xor5.pla",
    };

    std::string const idir = "/home/michal/data/IWLS93/pla/";
    std::string const odir = "/home/michal/repos/experiments/2024-informatics-sat_count/data_in/";

    for (const auto *const file : files)
    {
        std::ofstream ofile(odir + file + ".csv");
        std::string const path = idir + file;
        compare_pla(path, 100, true, ofile);
    }
}

auto main () -> int
{
    // TODO(michal): JSON config ako na projekte
    // run_compare();
    run_compare_pla();
}