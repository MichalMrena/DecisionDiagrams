#include <iostream>

#include "lib/utils/stopwatch.hpp"
#include "lib/utils/print.hpp"
#include "lib/mdd_manager.hpp"
#include "lib/bdd_manager.hpp"

#include <bitset>
#include <cassert>

using namespace mix::dd;
using namespace mix::utils;

auto bss_reliability_test()
{
    auto manager = bdd_manager<double, void>(5);
    auto& m = manager;
    auto& x = manager;

    auto sf = m.apply(m.apply(m.apply(x(0), AND(), x(1)), OR(), m.apply(x(2), AND(), x(3))), OR(), x(4));
    auto const ps = std::vector {0.9, 0.8, 0.7, 0.9, 0.9};
    auto dpbds = m.dpbds(sf);

    manager.calculate_probabilities(ps, sf);
    auto const A    = manager.get_availability();
    auto const U    = manager.get_unavailability();
    auto const SIs  = m.structural_importances(dpbds);
    auto const BIs  = m.birnbaum_importances(ps, dpbds);
    auto const CIs  = m.criticality_importances(BIs, ps, U);
    auto const FIs  = m.fussell_vesely_importances(dpbds, ps, U);
    auto const MCVs = m.mcvs<std::bitset<5>>(std::move(dpbds));

    printl(concat("A = "   , A));
    printl(concat("U = "   , U));
    printl(concat("SI "    , concat_range(SIs, " ")));
    printl(concat("BI "    , concat_range(BIs, " ")));
    printl(concat("CI "    , concat_range(CIs, " ")));
    printl(concat("FI "    , concat_range(FIs, " ")));
    printl(concat("MCVs: " , concat_range(MCVs, ", ")));
}

namespace
{
    auto const serial23 = [](auto const lhs, auto const rhs)
    {
        using log_t = typename log_val_traits<3>::type;
        auto constexpr table = std::array<std::array<log_t, 3>, 2>
        {{
            {0, 0, 0},
            {0, 1, 2},
        }};
        return table.at(lhs).at(rhs);
    };

    auto const parallel23 = [](auto const lhs, auto const rhs)
    {
        using log_t = typename log_val_traits<3>::type;
        auto constexpr table = std::array<std::array<log_t, 3>, 2>
        {{
            {0, 1, 2},
            {1, 1, 2},
        }};
        return table.at(lhs).at(rhs);
    };

    auto const parallel33 = [](auto const lhs, auto const rhs)
    {
        using log_t = typename log_val_traits<3>::type;
        auto constexpr table = std::array<std::array<log_t, 3>, 3>
        {{
            {0, 1, 1},
            {1, 2, 2},
            {1, 2, 2},
        }};
        return table.at(lhs).at(rhs);
    };

    struct serial23_t : public bin_op<decltype(serial23), 3, domain_e::nonhomogenous, log_val_traits<3>::undefined> {};
    struct parallel23_t : public bin_op<decltype(parallel23), 3, domain_e::nonhomogenous, log_val_traits<3>::undefined> {};
    struct parallel33_t : public bin_op<decltype(parallel33), 3, domain_e::nonhomogenous, log_val_traits<3>::undefined> {};

    constexpr auto op_id (serial23_t)   { return op_id_t {15}; }
    constexpr auto op_id (parallel23_t) { return op_id_t {16}; }
    constexpr auto op_id (parallel33_t) { return op_id_t {17}; }
    constexpr auto op_is_commutative (serial23_t)   { return false; }
    constexpr auto op_is_commutative (parallel23_t) { return false; }
    constexpr auto op_is_commutative (parallel33_t) { return false; }
}

auto mss_reliability_test()
{
    using log_t      = typename log_val_traits<3>::type;
    using prob_table = typename mdd_manager<double, void, 3>::prob_table;
    using vec_t      = std::array<int, 4>;

    auto m  = mdd_manager<double, void, 3>(4, {2, 3, 2, 3});
    auto& x = m;
    auto sf = m.apply( m.apply(x(0), serial23_t(), x(1))
                     , parallel33_t()
                     , m.apply(x(2), parallel23_t(), x(3)) );
    auto dpbds = m.dpbds_integrated_1({1, 0}, 1, sf);
    auto const ps = prob_table{ {0.1, 0.9, 0.0}
                              , {0.2, 0.6, 0.2}
                              , {0.3, 0.7, 0.0}
                              , {0.1, 0.6, 0.3} };
    m.calculate_probabilities(ps, sf);
    auto const A1   = m.get_availability(1);
    auto const A2   = m.get_availability(2);
    auto const SIs1 = m.structural_importances(dpbds);
    auto const BIs1 = m.birnbaum_importances(ps, dpbds);

    printl(concat("A1 = " , A1));
    printl(concat("A2 = " , A2));
    printl(concat("SI " , concat_range(SIs1, " ")));
    printl(concat("BI " , concat_range(BIs1, " ")));

    auto const mcvs = m.mcvs<vec_t>(dpbds, 1);
    for (auto const& cut : mcvs)
    {
        printl(concat("(", concat_range(cut, ", "), ")"));
    }
}

auto mss_playground()
{
    auto m  = mdd_manager<double, void, 3>(4);
    auto& x = m;
    // TODO set domains, set ps, default a void akou default


    auto f = m.apply( m.apply(x(0), MIN<3>(), x(1))
                    , MAX<3>()
                    , m.apply(x(2), MAX<3>(), x(3)) );
    // auto idpbd = m.dpbd_integrated_3({0, 1}, 1, f, 1);
    // auto mnf   = m.to_mnf(idpbd);
    // m.to_dot_graph(std::cout, mnf);

    // m.to_dot_graph(std::cout, f);
    using log_t = typename log_val_traits<3>::type;
    using vec4 = std::array<log_t, 4>;
    using vec4_v = std::vector<vec4>;
    auto vs = vec4_v {};
    m.template satisfy_all<vec4>(1, f, std::back_inserter(vs));

    for (auto const v : vs)
    {
        printl(concat("(", v[0], ", ", v[1], ", ", v[2], ", ", v[3], ")"));
    }
    printl(concat("Total: ", vs.size()));
    printl(concat("Total: ", m.satisfy_count(1, f)));
}

auto pla_test()
{
    auto constexpr plaDir = "/mnt/c/Users/mrena/Desktop/pla_files/IWLS93/pla/";
    // auto files = {"15-adder_col.pla"};
    auto files = {"xor5.pla"};

    auto load_pla = [plaDir](auto&& fileName)
    {
        auto const filePath = mix::utils::concat(plaDir , fileName);
        auto file           = pla_file::load_file(filePath);
        auto manager        = bdd_manager<void, void>(file.variable_count());
        auto const ds       = manager.from_pla(file, fold_e::tree);
        auto sum            = 0u;
        for (auto& d : ds)
        {
            sum += manager.vertex_count(d);
        }
        std::cout << fileName << " [" << sum << "] " << std::endl;
        manager.to_dot_graph(std::cout);
    };

    for (auto fileName : files)
    {
        auto et = avg_run_time(1, std::bind(load_pla, fileName));
        printl(concat(fileName , " -> " , et , "ms [" , "-" , "]"));
    }
}

auto basic_test()
{
    using manager_t = mdd_manager<double, void, 2>;
    auto manager    = manager_t(100);

    auto zero = manager.just_val(0);
    auto one  = manager.just_val(1);
    auto x1   = manager.just_var(1);
    auto x2   = manager.just_var(2);
    auto x3   = manager.just_var(3);
    auto prod = manager.apply(x1, AND(), x2);

    manager.to_dot_graph(std::cout);

}

auto main() -> int
{
    auto watch = stopwatch();

    // TODO V texte diplomovky určite spomenúť, že každý vrchol je unikátna funckia, preto je možné využiť matematické vlastnosti ako komutativita pri
    // implementácii cache mechanizmu a tabuľky unikátnych funkcií

    // basic_test();
    // pla_test();
    bss_reliability_test();
    // mss_reliability_test();
    // mss_playground();

    auto const timeTaken = watch.elapsed_time().count();
    printl("Done.");
    printl("Time taken: " + std::to_string(timeTaken) + " ms");

    return 0;
}