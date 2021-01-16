#include <iostream>

#include "lib/utils/stopwatch.hpp"
#include "lib/utils/print.hpp"
#include "lib/mdd_manager.hpp"
#include "lib/bdd_manager.hpp"

#include "test/test_bdd.hpp"
#include "test/test_mdd.hpp"

#include <bitset>
#include <cassert>

using namespace mix::dd;
using namespace mix::utils;

auto bss_reliability_test()
{
    auto  m = make_bdd_manager(5);
    auto& x = m;
    register_manager(m);
    // m.set_order({0, 3, 1, 4, 2});

    auto const ps = std::vector {0.9, 0.8, 0.7, 0.9, 0.9};
    auto sf = (x(0) and x(1)) or ((x(2) and x(3)) or x(4));

    auto const A = m.availability(ps, sf);
    auto const U = 1 - A;

    auto dpbds = m.dpbds(sf);
    auto const SIs  = m.structural_importances(dpbds);
    auto const BIs  = m.birnbaum_importances(ps, dpbds);
    auto const CIs  = m.criticality_importances(BIs, ps, U);
    auto const FIs  = m.fussell_vesely_importances(dpbds, ps, U);
    auto const MCVs = m.mcvs<std::bitset<5>>(dpbds);

    m.to_dot_graph(std::cout, sf);

    printl(concat("A = "   , A));
    printl(concat("U = "   , U));
    printl(concat("SI "    , concat_range(SIs, " ")));
    printl(concat("BI "    , concat_range(BIs, " ")));
    printl(concat("CI "    , concat_range(CIs, " ")));
    printl(concat("FI "    , concat_range(FIs, " ")));
    printl(concat("MCVs: " , concat_range(MCVs, ", ")));
}

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

auto constexpr U = log_val_traits<3>::undefined;

template<std::size_t P> struct serial23_t   : public mix::dd::impl::bin_op<decltype(serial23), P, U> {};
template<std::size_t P> struct parallel23_t : public mix::dd::impl::bin_op<decltype(parallel23), P, U> {};
template<std::size_t P> struct parallel33_t : public mix::dd::impl::bin_op<decltype(parallel33), P, U> {};

template<std::size_t P> constexpr auto op_id (serial23_t<P>)   { return op_id_t {15}; }
template<std::size_t P> constexpr auto op_id (parallel23_t<P>) { return op_id_t {16}; }
template<std::size_t P> constexpr auto op_id (parallel33_t<P>) { return op_id_t {17}; }
template<std::size_t P> constexpr auto op_is_commutative (serial23_t<P>)   { return false; }
template<std::size_t P> constexpr auto op_is_commutative (parallel23_t<P>) { return false; }
template<std::size_t P> constexpr auto op_is_commutative (parallel33_t<P>) { return false; }

auto mss_reliability_test()
{
    using prob_table = typename mdd_manager<double, void, 3>::prob_table;
    using vec_t      = std::array<unsigned int, 4>;

    auto m  = mdd_manager<double, void, 3>(4);
    m.set_domains({2, 3, 2, 3});
    auto& x = m;
    auto sf = m.apply<parallel33_t>( m.apply<serial23_t>(x(0), x(1))
                                   , m.apply<parallel23_t>(x(2), x(3)) );
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

    auto constexpr ND = log_val_traits<3>::nodomain;
    printl(m.satisfy_count(0, sf));
    printl(m.satisfy_count(1, sf));
    printl(m.satisfy_count(2, sf));
    printl(m.satisfy_count(ND, sf));
}

auto mss_playground()
{
    auto m  = mdd_manager<double, void, 3>(4);
    auto& x = m;
    // TODO set domains, set ps, default a void akou default


    auto f = m.apply<MAX>( m.apply<MIN>(x(0), x(1))
                         , m.apply<MAX>(x(2), x(3)) );
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
    auto files = {"clip.pla"};

    auto load_pla = [plaDir](auto&& fileName)
    {
        auto const filePath = mix::utils::concat(plaDir , fileName);
        auto file           = pla_file::load_file(filePath);
        auto manager        = bdd_manager<void, void>(file.variable_count());
        auto const ds       = manager.from_pla(file, fold_e::tree);
        auto sum            = 0ul;
        for (auto& d : ds)
        {
            sum += manager.vertex_count(d);
        }
        std::cout << fileName << " [" << sum << "] " << std::endl;
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

    auto zero = manager.constant(0);
    auto one  = manager.constant(1);
    auto x1   = manager.variable(1);
    auto x2   = manager.variable(2);
    auto x3   = manager.variable(3);
    auto prod = manager.apply<AND>(x1, x2);

    manager.to_dot_graph(std::cout);
}

auto example_basic_usage_bdd()
{
    using namespace mix::dd;

    // We will use variables x0, x1, x2, x3, x4.
    auto m = make_bdd_manager(5);

    // Code from following examples goes here.

    auto cFalse = m.constant(0);
    auto cTrue  = m.constant(1);
    auto x0     = m.variable(0);
    auto x4     = m.variable(4);
    auto& x     = m;
    auto x1     = x(1);
    auto x2_    = m.variable_not(2);
    auto x3_    = x(3, NOT());
    auto x1_    = m.negate(x1);

    auto f = m.apply<OR>( m.apply<AND>(x0, x1)
                        , m.apply<OR>( m.apply<OR>(x(2), x(3))
                                     , x4 ) );

    register_manager(m);

    auto g  = (x(0) && x(1)) || (x(2) && x(3) || x(4));
    auto g1 = !(x(0) && x(1)) || (!(x(2) || x(3)) || x(4));

    auto const val0 = m.evaluate(f, std::array  {0u, 1u, 1u, 0u, 1u});
    auto const val1 = m.evaluate(f, std::vector {0u, 1u, 1u, 0u, 1u});
    auto const val2 = m.evaluate(f, std::vector {false, true, true, false, true});
    auto const val3 = m.evaluate(f, std::bitset<5> {0b10110});
    auto const val4 = m.evaluate(f, 0b10110);

    auto const sc = m.satisfy_count(f);
    auto const td = m.truth_density(f);

    using var_vals_t = std::bitset<5>;
    auto vars = std::vector<var_vals_t>();
    m.satisfy_all<var_vals_t>(f, std::back_inserter(vars));
    m.satisfy_all<var_vals_t>(f, std::ostream_iterator<var_vals_t>(std::cout, "\n"));

    for (auto const& v : vars)
    {
        assert(1 == m.evaluate(f, v));
    }

    printl(val0);
    printl(val1);
    printl(val2);
    printl(val3);
    printl(val4);
    printl(sc);
    printl(td);

    auto f1 = x(1) xor x(2);
    auto f2 = (x(1) or x(2)) and (!x(1) or not x(2));
    assert(f1 == f2);
    // m.to_dot_graph(std::cout, f1);
}

auto example_basic_usage_mdd()
{
    auto m  = make_mdd_manager<4>(4);
    auto& x = m;

    // f = x0 * x1 + x2 * x3 mod 4
    auto f = m.apply<PLUS>( m.apply<MULTIPLIES>(x(0), x(1))
                              , m.apply<MULTIPLIES>(x(2), x(3)) );

    auto const val0 = m.evaluate(f, std::array  {0u, 1u, 2u, 3u});
    auto const val1 = m.evaluate(f, std::vector {0u, 1u, 2u, 3u});
    auto const sc2  = m.satisfy_count(2, f);

    using var_vals_t = std::array<unsigned int, 4>;
    auto sas = std::vector<var_vals_t>();
    m.satisfy_all<var_vals_t>(2, f, std::back_inserter(sas));

    assert(val0 == val1);
    assert(sc2 == sas.size());
}

auto order_test()
{
    auto const vector = std::vector<bool> {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    auto m  = make_bdd_manager(6);
    auto& x = m;
    register_manager(m);

    auto order = std::vector<level_t> {0, 1, 2, 3, 4, 5};
    while (std::next_permutation(std::begin(order), std::end(order)))
    {
        m.clear();
        m.set_order(order);

        auto f = (x(0) && x(1)) || (x(2) && x(3)) || (x(4) && x(5));
        auto varVals = 0;
        for (auto const correctVal : vector)
        {
            if (m.evaluate(f, varVals) != correctVal)
            {
                throw "not good";
            }
            ++varVals;
        }
    }
}

auto swap_var_test()
{
    auto m  = make_bdd_manager(6);
    auto& x = m;
    register_manager(m);
    m.set_order({0, 1, 2, 3, 4, 5});

    auto f = (x(0) && x(3))
          || (x(1) && x(4))
          || (x(2) && x(5));

    // m.collect_garbage();
    // m.to_dot_graph(std::cout);
    // m.swap_vars(2);

    // m.to_dot_graph(std::cout);
    m.collect_garbage();
    // m.to_dot_graph(std::cout);
}

auto patterns_imgs()
{
    auto m1 = make_mdd_manager<3>(3);
    auto x1 = m1.variable(1);
    auto x2 = m1.variable(2);
    auto f1 = m1.apply<MAX>(x1, x2);
    m1.to_dot_graph(std::cout, f1);

    auto m2 = make_bdd_manager(4);
    auto& x = m2;
    register_manager(m2);
    auto f2 = (x(1) && x(2)) || x(3);
    m2.to_dot_graph(std::cout, f2);

    m2.to_dot_graph(std::cout, x(1) * x(2));
}

auto main() -> int
{
    auto watch = stopwatch();

    // basic_test();
    // pla_test();
    // bss_reliability_test();
    // mss_reliability_test();
    // mss_playground();
    // example_basic_usage_bdd();
    // example_basic_usage_mdd();
    // order_test();
    // swap_var_test();
    // eq_test();
    // patterns_imgs();

    test::test_bdd(5, test::order_e::Random);
    test::test_mdd<3>(5, test::order_e::Random, test::domain_e::Nonhomogenous);

    auto const timeTaken = watch.elapsed_time().count();
    printl("Done.");
    printl("Time taken: " + std::to_string(timeTaken) + " ms");

    return 0;
}