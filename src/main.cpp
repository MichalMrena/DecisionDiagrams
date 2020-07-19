#include "lib/utils/stopwatch.hpp"
#include "lib/utils/more_random.hpp"
#include "lib//bdd_tools.hpp"

// #include <iostream>
// #include <algorithm>
// #include <vector>
// #include <iterator>

using namespace mix::dd;
using namespace mix::utils;

auto map_test()
{
    auto constexpr tcc = 20;
    auto rng   = mix::utils::random_uniform_int<int> {10, 50, 274};
    auto map   = mix::ds::simple_map<int, std::string> {};
    auto ks    = std::vector<int> {};
    auto count = 0u;

    for (auto i = 0u; i < tcc; ++i)
    {
        auto const key = rng.next_int();
        // map.emplace(key, std::to_string(key) + "foo");
        // map.try_emplace(key, std::to_string(key) + "foo");
        auto const [it, isIn] = map.insert(std::make_pair(key, std::to_string(key)));
        count += isIn;
        ks.emplace_back(key);
    }

    if (map.size() != count)
    {
        printl("!!! Error size missmatch.");
        return;
    }

    for (auto&& k : ks)
    {
        auto const it = map.find(k);
        if (map.end() == it)
        {
            printl("!!! Error key not found.");
            return;
        }
    }

    map.erase(21);

    for (auto&& [key, val] : map)
    {
        printl(concat(key, " ", val));
    }
}

auto graph()
{
    // std::cout << sizeof(vertex<void,   void,   2>); // 32 << '\n';
    // std::cout << sizeof(vertex<double, void,   2>); // 40 << '\n';
    // std::cout << sizeof(vertex<void,   double, 2>); // 48 << '\n';
    // std::cout << sizeof(vertex<double, double, 2>); // 56 << '\n';

    auto bdd  = x(0) * x(1) + x(2);

    auto val1 = bdd.get_value(std::array  {1, 1, 0});
    auto val2 = bdd.get_value(std::vector {true, true, false});
    auto val3 = bdd.get_value(0b110);

    std::cout << (int)val1 << " " << (int)val2 << " " << (int)val3 << std::endl;

    // using var_vals_t = std::bitset<3>;
    // auto satisfySet  = std::vector<var_vals_t> {};
    // auto outputIt    = std::back_inserter(satisfySet);
    
    // bdd.satisfy_all<var_vals_t>(outputIt);

    // auto tools       = bdd_tools {};
    // auto creator     = tools.creator();
    // auto manipulator = tools.manipulator();
    
    // auto x1   = creator.just_var(1);
    // auto x2c  = manipulator.negate(creator.just_var(2));
    // auto conj = manipulator.apply(x1, OR{}, x2c);
    // manipulator.apply(x1, OR {}, manipulator.restrict_var(conj, 2, 0))
    //            .to_dot_graph(std::cout);


    // using vertex_t = vertex<int, int, 2>;

    // auto v1 = vertex_t {0, 0};
    // auto v2 = v1;
    // auto v3 = std::move(v2);
    // auto v4 = vertex_t {std::move(v3)};
    // auto v5 = vertex_t {v4};
}

auto alloc()
{
    auto pool = recycling_pool<int> {};
    auto a    = pool_allocator {pool};
    // auto at   = std::allocator_traits<decltype(a)> {};

    auto const p = std::allocator_traits<decltype(a)>::allocate(a, 1);
    std::allocator_traits<decltype(a)>::construct(a, p, 7);

    std::cout << *p << std::endl;

    std::allocator_traits<decltype(a)>::destroy(a, p);
    std::allocator_traits<decltype(a)>::deallocate(a, p, 1);
}

auto vec_create()
{
    auto tools    = bdd_tools {};
    auto creator  = tools.creator();
    auto const xs1 = std::vector {0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    auto const xs2 = (x(0) * x(1)) + ((x(2) * x(3)) + x(4));
    auto const xs3 = truth_vector::from_text_file("input_func.txt");
    auto const xs4 = truth_vector::from_string("01010111");
    auto const vec = truth_vector::from_lambda([](auto&& x) 
    { 
        return (x(1) && x(2)) || x(3);
    });
 
    auto const d = creator.from_vector(xs3);

    creator.from_vector(xs3).to_dot_graph(std::cout);
    creator.from_vector(xs1).to_dot_graph(std::cout);
    creator.from_vector(vec).to_dot_graph(std::cout);
    creator.from_vector(xs4).to_dot_graph(std::cout);
    xs2.to_dot_graph(std::cout);
}

auto pla_alloc_speed_pooled()
{
    // auto files = {"16-adder_col.pla", "seq.pla", "apex2.pla", "apex1.pla"};
    // auto files = {"10-adder_col.pla", "11-adder_col.pla", "12-adder_col.pla", "13-adder_col.pla", "14-adder_col.pla"};
    // auto files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla"};
    // auto files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla"};
    // auto files = {"10-adder_col.pla", "seq.pla", "apex2.pla", "apex1.pla"};
    // auto files = {"10-adder_col.pla"};
    // auto files   = {"apex1.pla", "apex3.pla", "apex5.pla", "seq.pla", "spla.pla"};
    auto files = { "16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla"
                 , "apex1.pla", "apex3.pla", "apex5.pla", "seq.pla", "spla.pla" };

    auto load_pla = [](auto&& fileName)
    {
        auto tools   = bdd_tools {};
        auto creator = tools.creator();
        auto file    = pla_file::load_file(fileName);
        (void)creator.from_pla(file, merge_mode_e::iterative);
        // (void)creator.from_pla(file, merge_mode_e::sequential);
    };

    for (auto fileName : files)
    {
        auto et = avg_run_time(1, std::bind(load_pla, fileName));
        printl(concat(fileName , " -> " , et , "ms [" , "-" , "]"));
    }
}

auto pla_alloc_speed_default()
{
    // auto files = {"16-adder_col.pla", "seq.pla", "apex2.pla", "apex1.pla"};
    // auto files = {"10-adder_col.pla", "11-adder_col.pla", "12-adder_col.pla", "13-adder_col.pla", "14-adder_col.pla"};
    // auto files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla"};
    // auto files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla"};
    // auto files = {"10-adder_col.pla", "seq.pla", "apex2.pla", "apex1.pla"};
    auto files = {"10-adder_col.pla"};
    // auto files   = {"apex1.pla", "apex3.pla", "apex5.pla", "seq.pla", "spla.pla"};
    // auto files = { "16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla"
    //              , "apex1.pla", "apex3.pla", "apex5.pla", "seq.pla", "spla.pla" };

    auto load_pla = [](auto&& fileName)
    {
        auto creator = bdd_creator<double, void> {};
        auto file    = pla_file::load_file(fileName);
        return creator.from_pla(file, merge_mode_e::sequential);
    };

    for (auto fileName : files)
    {
        auto et   = avg_run_time(1, std::bind(load_pla, fileName));
        auto ds   = load_pla(fileName);
        auto sum  = 0ul;
        for (auto&& d : ds) { sum += d.vertex_count(); }
        printl(concat(fileName , " -> " , et , "ms [" , sum , "]"));
    }
}

auto reliability_test()
{
    auto tools    = bdd_tools {};
    auto relTools = tools.reliability();
    auto creator  = tools.creator();
    
    auto sfVector = truth_vector::from_lambda([](auto&& x)
    {
        return (x(0) && x(1)) || ((x(2) && x(3)) || x(4));
    });
    auto structureFunction = creator.from_vector(sfVector);
    auto const ps = std::vector {0.9, 0.8, 0.7, 0.9, 0.9};
    
    auto const A    = relTools.availability(structureFunction, ps);
    auto const U    = relTools.unavailability(structureFunction, ps);
    auto dpbds      = relTools.dpbds(std::move(structureFunction));
    auto const SIs  = relTools.structural_importances(dpbds);
    auto const BIs  = relTools.birnbaum_importances(dpbds, ps);
    auto const CIs  = relTools.criticality_importances(dpbds, ps, U);
    auto const FIs  = relTools.fussell_vesely_importances(dpbds, ps, U);
    auto const MCVs = relTools.mcvs<std::bitset<5>>(std::move(dpbds));

    printl(concat("A = "   , A));
    printl(concat("U = "   , U));
    printl(concat("SI "    , concat_range(SIs, " ")));
    printl(concat("BI "    , concat_range(BIs, " ")));
    printl(concat("CI "    , concat_range(CIs, " ")));
    printl(concat("FI "    , concat_range(FIs, " ")));
    printl(concat("MCVs: " , concat_range(MCVs, ", ")));
}

auto sanity_check()
{
    auto files = { "16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla"
                 , "apex1.pla", "apex3.pla", "apex5.pla", "seq.pla", "spla.pla" };
    // auto files = {"15-adder_col.pla"};

    auto tools   = bdd_tools {};
    auto creator = tools.creator();

    for (auto const& file : files)
    {
        auto pla = pla_file::load_file(file);
        auto ds  = creator.from_pla(pla);
        auto cs  = 0u;
        for (auto const& d : ds) { cs += d.vertex_count(); }
        printl(concat(file , " [" , cs , "]"));
    }
}

auto satisfy_test()
{
    auto       mutableF  = (x(0) && x(1)) || ((x(2) && x(3)) || x(4));
    auto const constantF = (x(0) && x(1)) || ((x(2) && x(3)) || x(4));
    
    auto const countMutable  = mutableF.satisfy_count();
    auto const countConstant = constantF.satisfy_count();
    printl(concat("mutable: ", countMutable));
    printl(concat("const:   ", countConstant));
}

auto mvl_test()
{
    using log_t = typename log_val_traits<4>::type;
    auto constexpr P = log_t {4};
    auto plusMod4 = [](log_t const lhs, log_t const rhs)
    {
        auto constexpr N = log_val_traits<P>::nondetermined;
        if (N == lhs || N == rhs) return N;
        return static_cast<log_t>((lhs + rhs) % P);
    };

    auto x = mdd_creator<void, void, P> {};
    auto m = mdd_manipulator<void, void, P> {};
    
    // f(x) = x0 + x1 + x2 + x3
    auto f = m.apply( m.apply(x(0), plusMod4, x(1))
                    , plusMod4
                    , m.apply(x(2), plusMod4, x(3)) );

    auto xs = range(0, 4);
    for (auto&& [v1, v2, v3, v4] : product(xs, xs, xs, xs))
    {
        auto const correctResult = (v1 + v2 + v3 + v4) % P;
        auto const diagramResult = f.get_value(std::array {v1, v2, v3, v4});
        if (correctResult != diagramResult)
        {
            printl(concat("!!! not good : ", v1, " ", v2, " ", v3, " ", v4));
        }
    }
    f.to_dot_graph(std::cout);
}

auto main() -> int
{
    auto watch = stopwatch {};

    // graph();
    // alloc();
    // vec_create();
    // pla_alloc_speed_default();
    // pla_alloc_speed_pooled();
    // sanity_check();
    // map_test();
    // reliability_test();
    // satisfy_test();
    mvl_test();

    auto const timeTaken = watch.elapsed_time().count();
    printl("Done.");
    printl("Time taken: " + std::to_string(timeTaken) + " ms");

    return 0;
}