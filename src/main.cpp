#include <iostream>
#include <iomanip>

#include "lib/utils/stopwatch.hpp"
#include "lib/utils/print.hpp"
#include "lib/utils/object_pool.hpp"
#include "lib/mdd_manager.hpp"
#include "lib/bdd_manager.hpp"

#include "test/test_mdd.hpp"
#include "test/test_reliability.hpp"

#include <bitset>
#include <cassert>

using namespace mix::dd;
using namespace mix::utils;
using namespace mix::dd::test;

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
    m.template satisfy_all_g<vec4>(1u, f, std::back_inserter(vs));

    for (auto const v : vs)
    {
        printl(concat("(", v[0], ", ", v[1], ", ", v[2], ", ", v[3], ")"));
    }
    printl(concat("Total: ", vs.size()));
    printl(concat("Total: ", m.satisfy_count(1, f)));
}

auto pla_sanity_check()
{
    auto constexpr plaDir = "/mnt/c/Users/mrena/Desktop/pla_files/IWLS93/pla/";
    auto const files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    // auto const files = {"14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    // auto const files = {"14-adder_col.pla"};

    for (auto const fileName : files)
    {
        auto const filePath = mix::utils::concat(plaDir , fileName);
        auto const file     = pla_file::load_file(filePath);
        auto manager        = bdd_manager<void, void>(file.variable_count());
        auto const ds       = manager.from_pla(file, fold_e::tree);
        auto sum            = 0ul;
        for (auto& d : ds)
        {
            sum += manager.vertex_count(d);
        }
        std::cout << fileName << " [" << sum << "] " << std::endl;
    }
}

auto pla_test_speed()
{
    auto constexpr plaDir = "/mnt/c/Users/mrena/Desktop/pla_files/IWLS93/pla/";
    auto const files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    // auto const files = {"14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla", "apex1.pla", "apex3_alt.pla", "apex5.pla", "seq.pla", "spla.pla"};
    // auto const files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla", "11-adder_col.pla", "10-adder_col.pla"};
    // auto const files = {"15-adder_col.pla"};

    auto const build_diagrams = [](auto const& plaFileRef)
    {
        auto manager  = bdd_manager<void, void>(plaFileRef.get().variable_count());
        auto const ds = manager.from_pla(plaFileRef.get(), fold_e::left);
    };

    for (auto fileName : files)
    {
        auto const filePath = mix::utils::concat(plaDir , fileName);
        auto const plaFile  = pla_file::load_file(filePath);
        auto const elapsed  = avg_run_time(1, std::bind_front(build_diagrams, std::cref(plaFile)));
        printl(concat(fileName , " -> " , elapsed , "ms"));
    }
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
    auto const satisfyingSet = m.satisfy_all<var_vals_t>(f);
    m.satisfy_all_g<var_vals_t>(f, std::ostream_iterator<var_vals_t>(std::cout, "\n"));

    for (auto const& v : satisfyingSet)
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
    assert(f1.equals(f2));
    m.to_dot_graph(std::cout, f1);
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
    auto sas = m.satisfy_all<var_vals_t>(2, f);

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
    auto  m = make_mdd_manager<3>(3);
    auto& x = m;
    m.set_domains({3, 2, 3});
    auto  f = m.apply<MIN>(x(0), m.apply<MAX>(x(1), x(2)));
    m.collect_garbage();
    m.swap_vars(1);
    m.collect_garbage();
    m.to_dot_graph(std::cout);
}

auto symmetric_example()
{
    auto constexpr VarCount = 20;
    auto order = mix::utils::fill_vector(VarCount, mix::utils::identityv);
    auto rng   = std::mt19937(144);
    for (auto i = 0u; i < 50; ++i)
    {
        std::shuffle(std::begin(order), std::end(order), rng);
        auto m = make_mdd_manager<4>(VarCount);
        m.set_order(order);
        // m.set_domains(mix::utils::fill_vector(VarCount, [](auto const){ return (unsigned)(std::rand() & 1 ? 2 : 3); }));

        auto xs = m.variables(mix::utils::fill_vector(VarCount, mix::utils::identityv));
        auto f  = m.tree_fold<PLUS>(std::begin(xs), std::end(xs));
        std::cout << m.vertex_count(f) << " :: " << mix::utils::concat_range(order, " ") << '\n';
    }
}

auto main () -> int
{
    // pla_sanity_check();
    // pla_test_speed();

//    test_mdd_random<3>(15, order_e::Random, domain_e::Nonhomogenous);
//    test_mdd_vector(10);
//    test_bss();
//    test_mss(5);

    swap_var_test();
    // symmetric_example();

    std::cout << "Done." << '\n';
    return 0;
}
