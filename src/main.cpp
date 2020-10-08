#include "lib/utils/stopwatch.hpp"
#include "lib/utils/more_random.hpp"
#include "lib//bdd_tools.hpp"

// #include <iostream>
// #include <algorithm>
// #include <vector>
// #include <iterator>
#include "lib/diagrams/mdd_reliability.hpp"

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
    auto constexpr plaDir = "/mnt/c/Users/mrena/Desktop/pla_files/IWLS93/pla/";
    // auto files = {"16-adder_col.pla", "seq.pla", "apex2.pla", "apex1.pla"};
    // auto files = {"10-adder_col.pla", "11-adder_col.pla", "12-adder_col.pla", "13-adder_col.pla", "14-adder_col.pla"};
    // auto files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla"};
    // auto files = {"16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla"};
    // auto files = {"10-adder_col.pla", "seq.pla", "apex2.pla", "apex1.pla"};
    // auto files = {"10-adder_col.pla"};
    // auto files   = {"apex1.pla", "apex3.pla", "apex5.pla", "seq.pla", "spla.pla"};
    auto files = { "16-adder_col.pla", "15-adder_col.pla", "14-adder_col.pla", "13-adder_col.pla", "12-adder_col.pla"
                 , "apex1.pla", "apex5.pla", "seq.pla", "spla.pla" };

    auto load_pla = [](auto&& filePath)
    {
        auto tools   = bdd_tools();
        auto creator = tools.creator();
        auto file    = pla_file::load_file(filePath);
        // (void)creator.from_pla(file, merge_mode_e::iterative);
        // (void)creator.from_pla(file, merge_mode_e::sequential);
        auto const ds = creator.from_pla(file, merge_mode_e::iterative);
        auto sum = 0u;
        for (auto d : ds)
        {
            sum += d.vertex_count();
        }
        std::cout << filePath << " [" << sum << "] " << std::endl;
    };

    for (auto fileName : files)
    {
        std::bind(load_pla, mix::utils::concat(plaDir , fileName)) ();
        // auto et = avg_run_time(1, std::bind(load_pla, mix::utils::concat(plaDir , fileName)));
        // printl(concat(fileName , " -> " , et , "ms [" , "-" , "]"));
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
    auto tools    = bdd_tools();
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
    // auto       mutableF  = (x(0) && x(1)) || ((x(2) && x(3)) || x(4));
    // auto const constantF = (x(0) && x(1)) || ((x(2) && x(3)) || x(4));

    // auto const countMutable  = mutableF.satisfy_count();
    // auto const countConstant = constantF.satisfy_count();
    // printl(concat("mutable: ", countMutable));
    // printl(concat("const:   ", countConstant));

    auto x = mdd_creator<double, void, 2>();
    auto f = (x(0) * x(1) + x(2));
    f.to_dot_graph(std::cout);
}

auto mvl_test()
{
    auto constexpr P = 4;
    auto x = mdd_creator<void, void, P>();
    auto m = mdd_manipulator<void, void, P>();

    auto x1 = x(P, 3);
    auto f  = m.apply(x1, EQUAL_TO<P, domain_e::homogenous>(), x.just_val(1));

    f.to_dot_graph(std::cout);
}

auto mvl_non_homogenous()
{
    using namespace mix::utils;
    using log_t      = typename log_val_traits<3>::type;    
    using prob_table = typename mdd_reliability<double, void, 3>::prob_table;

    auto const serial_c = [](auto const x1val, auto const x2val) -> log_t
    {
        auto constexpr N     = log_val_traits<3>::nondetermined;
        auto constexpr ND    = log_val_traits<3>::nodomain;
        auto constexpr table = std::array<std::array<log_t, 3>, 2>
        {{
            {0, 0, 0},
            {0, 1, 2},
        }};
        return N == x1val  or N == x2val  ? N  :
               ND == x1val or ND == x2val ? ND : table.at(x1val).at(x2val);
    };

    auto const parallel_d = [](auto const x3val, auto const x5val) -> log_t
    {
        auto constexpr N     = log_val_traits<3>::nondetermined;
        auto constexpr ND    = log_val_traits<3>::nodomain;
        auto constexpr table = std::array<std::array<log_t, 3>, 2>
        {{
            {0, 1, 2},
            {1, 1, 2},
        }};
        return N == x3val  or N == x5val  ? N  :
               ND == x3val or ND == x5val ? ND : table.at(x3val).at(x5val);
    };

    auto const parallel_f = [](auto const cval, auto const dval)
    {
        auto constexpr N     = log_val_traits<3>::nondetermined;
        auto constexpr ND    = log_val_traits<3>::nodomain;
        auto constexpr table = std::array<std::array<log_t, 3>, 3>
        {{
            {0, 1, 1},
            {1, 2, 2},
            {1, 2, 2},
        }};
        return N == cval  or N == dval  ? N  :
               ND == cval or ND == dval ? ND : table.at(cval).at(dval);
    };

    auto x  = mdd_creator<double, void, 3>();
    auto m  = mdd_manipulator<double, void, 3>();
    auto r  = mdd_reliability<double, void, 3>();
    auto x1 = x(1, 2);
    auto x2 = x(2);
    auto x3 = x(3, 2);
    auto x5 = x(5);
    auto c  = m.apply(x1, serial_c, x2);
    auto d  = m.apply(x3, parallel_d, x5);
    auto f  = m.apply(c, parallel_f, d);

    auto const ps = prob_table{ {0, 0, 0}
                              , {0.1, 0.9, 0.0}
                              , {0.2, 0.6, 0.2}
                              , {0.3, 0.7, 0.0}
                              , {0, 0, 0}
                              , {0.1, 0.6, 0.3} };

    auto const A1 = r.availability(f, 1, ps);
    auto const A2 = r.availability(f, 2, ps);

    auto dpbds = r.dpbds_integrated_1(f, {1, 0}, 1);
    auto const bis1 = r.birnbaum_importances(dpbds, ps);

    for (auto const i : {1, 2, 3, 5})
    {
        printl(concat("x" , i , " = " , bis1[i]));
    }

    for (auto const i : {1, 2, 3, 5})
    {
        auto const& d = dpbds[i];
        auto const p0  = d.get_leaf(0)->data;
        auto const p1  = d.get_leaf(1)->data;
        printl(concat("d" , i , ": p0=" , p0 , " p1=" , p1));
    }
    printl("----------");

    dpbds[1].to_dot_graph(std::cout);

    // for (auto& d : dpbds)
    // {
    //     d.to_dot_graph(std::cout);
    //     printl("----------");
    // }

    // printl(concat("A1 = " , A1));
    // printl(concat("A2 = " , A2));
}

auto main() -> int
{
    auto watch = stopwatch();

    // graph();
    // alloc();
    // vec_create();
    // pla_alloc_speed_default();
    // pla_alloc_speed_pooled();
    // sanity_check();
    // map_test();
    // reliability_test();
    satisfy_test();
    // mvl_test();
    // mvl_non_homogenous();

    auto const timeTaken = watch.elapsed_time().count();
    printl("Done.");
    printl("Time taken: " + std::to_string(timeTaken) + " ms");

    return 0;
}