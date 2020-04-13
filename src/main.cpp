#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <type_traits>

#include "utils/stopwatch.hpp"
#include "utils/io.hpp"
#include "utils/string_utils.hpp"
#include "utils/carthesian_product.hpp"

#include "bdd_test/diagram_tests.hpp"
#include "bdd/bdd_creator.hpp"
#include "bdd/pla_file.hpp"
#include "bdd/pla_function.hpp"
#include "dd/mdd_creator.hpp"
#include "dd/mdd_manipulator.hpp"

#include "dd/mdd.hpp"

#include "reliability.hpp"

using namespace mix::dd;
using namespace mix::utils;

auto pla_dir_path () -> std::string
{
    return "/mnt/c/Users/mrena/Desktop/pla_files/IWLS93/pla/";
}

auto test_string_utils ()
{
    // const auto file {pla_file::load_from_file(pla_dir_path() + "01-adder.pla")};
    // pla_file::save_to_file("testout.txt", file);

    using namespace std::string_literals;

    // const auto str {"  foo    boo  moo"s};
    // printl(shrink_spaces(str));

    const auto s {" foo boo   moo"s};
    const auto ws {to_words(s)};
    for (auto& w : ws)
    {
        printl(w);
    }
}

auto test_adders ()
{
    const auto fileNames = { "01-adder.pla"
                           , "02-adder_col.pla"
                           , "03-adder_col.pla"
                           , "04-adder_col.pla"
                           , "05-adder_col.pla"
                           , "06-adder_col.pla"
                           , "07-adder_col.pla"
                           , "08-adder_col.pla"
                           , "09-adder_col.pla" };
                        //    , "10-adder_col.pla"
                        //    , "11-adder_col.pla"
                        //    , "12-adder_col.pla"
                        //    , "13-adder_col.pla"
                        //    , "14-adder_col.pla"
                        //    , "15-adder_col.pla"
                        //    , "16-adder_col.pla" };
    
    for (auto fileName : fileNames)
    {
        const auto plaFile {pla_file::load_from_file(pla_dir_path() + fileName)};
    
        printl("Testing " + std::string {fileName});
        if (! test_pla_creator(plaFile))
        {
            break;
        }
        printl("");
    }
}

auto test_plas ()
{
    const auto fileNames = { "apex5.pla" };
                        //    , "pdc.pla"
                        //    , "spla.pla" };

    for (auto&& filename : fileNames)
    {
        const auto plaFile {pla_file::load_from_file(pla_dir_path() + filename)};
        test_pla_creator(plaFile);
    }
}

namespace mdd_inputs_aux
{
    template<size_t I, class Domain>
    auto&& dummy(Domain&& domain)
    {
        return domain;
    }

    template<class Domain, size_t... Is>
    auto domain_product(Domain&& domain, std::index_sequence<Is...>)
    {
        return product {dummy<Is>(std::forward<Domain>(domain))...};;
    }

    template<size_t N, size_t P>
    struct all_inputs
    {
        using domain_t = std::array<typename log_val_traits<P>::value_t, P>;
        domain_t domain;

        all_inputs()
        {
            // dalo by sa spraviť compile time ale to by bol v tejto chvíli tryhard...
            for (auto i = 0u; i < P; ++i)
            {
                domain[i] = i;
            }
        }

        auto operator() () const
        { 
            return domain_product(domain, std::make_index_sequence<N>{}); 
        }
    };

    template<typename Tuple>
    constexpr auto tuple_to_array(Tuple&& tuple)
    {
        auto constexpr get_array = [](auto&& ... x){ return std::array {std::forward<decltype(x)>(x)...}; };
        return std::apply(get_array, std::forward<Tuple>(tuple));
    }
}

inline auto min = [](auto v1, auto v2)
{
    using log_t = log_val_traits<3>::value_t;

    if (0 == v1 || 0 == v2) return static_cast<log_t>(0);
    if (3 == v1 || 3 == v2) return static_cast<log_t>(3);
    
    return std::min(v1, v2);
};

inline auto max = [](auto v1, auto v2)
{
    using log_t = log_val_traits<3>::value_t;

    if (2 == v1 || 2 == v2) return static_cast<log_t>(2);
    if (3 == v1 || 3 == v2) return static_cast<log_t>(3);
    
    return std::max(v1, v2);
};

auto test_mdd ()
{
    using namespace mdd_inputs_aux;
    using log_t = log_val_traits<3>::value_t;
    using val_v = std::vector<log_t>;

    auto  creator     = mdd_creator<empty_t, empty_t, 3> {};
    auto  manipulator = mdd_manipulator<empty_t, empty_t, 3> {};
 
    auto const x0       = creator.just_var(0);
    auto const x1       = creator.just_var(1);
    auto const x2       = creator.just_var(2);
    auto const x3       = creator.just_var(3);
    auto const parallel = manipulator.apply(x1, max, x2);
    auto const min1     = manipulator.apply(x0, min, parallel);
    auto const result   = manipulator.apply(min1, min, x3);

    auto correct_value = [](auto const& varVals)
    {
        return std::min({ varVals.at(0), std::max(varVals.at(1), varVals.at(2)), varVals.at(3) });
    };

    auto const allInputs = all_inputs<4, 3> {};
    for (auto&& varValsTuple : allInputs())
    {
        // for (auto val : tuple_to_array(varValsTuple))
        // {
        //     std::cout << (int)val << ' ';
        // }
        // std::cout << '\n';
        auto const inputArr   = tuple_to_array(varValsTuple);
        auto const diagramVal = result.get_value(inputArr);
        auto const correctVal = correct_value(inputArr);

        if (diagramVal != correctVal)
        {
            printl("!!! Error different value.");
        }
    }

    printl("Diagram is correct.");
    // printl(result.to_dot_graph());
}

auto test_satisfy ()
{
    bdd_creator<double, empty_t> creator;
    const auto plaFile  = pla_file::load_from_file(pla_dir_path() + "07-adder_col.pla");
          auto diagrams = creator.create_from_pla(plaFile, merge_mode_e::iterative);

    for (size_t i (0); i < diagrams.size(); ++i)
    {
        test_satisfy_all(diagrams.at(i));
    }
}

auto verify_adder ()
{
    auto const f1_1 = x(1) * x(3) + x(2) * x(3) * x(4) + x(1) * x(2) * x(4);
    auto const f2_1 = !x(1) * x(3) * !x(4) + x(1) * !x(3) * !x(4) + !x(1) * !x(2) * x(3) + x(1) * !x(2) * !x(3) + x(1) * x(2) * x(3) * x(4) + !x(1) * x(2) * !x(3) * x(4);
    auto const f3_1 = x(2) * !x(4) + !x(2) * x(4);

    auto const f1_2 = !(   !(x(1) * x(3)) * !(x(2) * x(3) * x(4)) * !(x(1) * x(2) * x(4)) );
    auto const f2_2 = !(   !( !x(1) * x(3) * !x(4) ) * !( x(1) * !x(3) * !x(4) ) * !( !x(1) * !x(2) * x(3) ) * !( x(1) * !x(2) * !x(3) ) *  !( x(1) * x(2) * x(3) * x(4) ) * !( !x(1) * x(2) * !x(3) * x(4) )   );
    auto const f3_2 = !(   !(x(2) * !x(4)) * !(!x(2) * x(4)) );

    printl( f1_1 == f1_2 ? "good" : "not good" );
    printl( f2_1 == f2_2 ? "good" : "not good" );
    printl( f3_1 == f3_2 ? "good" : "not good" );

    // printl( f1_2.to_dot_graph() );
    // printl( f2_2.to_dot_graph() );
    // printl( f3_2.to_dot_graph() );

    for (auto prve = 0u; prve < 4; ++prve)
    {
        for (auto druhe = 0u; druhe < 4; ++druhe)
        {
            auto const prveBits  = std::bitset<2> {prve};
            auto const druheBits = std::bitset<2> {druhe};
            auto       inputBits = std::bitset<5> {0};

            inputBits[1] = prveBits[1];            
            inputBits[2] = prveBits[0];            
            inputBits[3] = druheBits[1];            
            inputBits[4] = druheBits[0];            

            auto const spravnyvysledok  = prve + druhe;
            auto const funkciaVysledok1 = 4u * f1_1.get_value(inputBits) + 2u * f2_1.get_value(inputBits) + f3_1.get_value(inputBits);
            auto const funkciaVysledok2 = 4u * f1_2.get_value(inputBits) + 2u * f2_2.get_value(inputBits) + f3_2.get_value(inputBits);

            printl( concat(prve, " + ", druhe, " = ", funkciaVysledok1, " -> ", spravnyvysledok == funkciaVysledok1 ? "good" : "not good") );            
            printl( concat(prve, " + ", druhe, " = ", funkciaVysledok2, " -> ", spravnyvysledok == funkciaVysledok2 ? "good" : "not good") );            
        }   
    }
}

auto main() -> int
{
    auto watch = stopwatch {};

    // mix::solve_examples_week_3();
    // mix::solve_examples_week_5();
    // test_plas();
    // verify_adder();
    test_mdd();

    // auto const diagram1 = !x(1) * x(2) * !x(3) + x(1) * !x(2) * !x(3) + x(1) * !x(2) * x(3) + x(1) * x(2) * !x(3);
    // auto       diagram2 = x(1) * !x(2) * !x(3) + !x(1) * !x(2) * x(3) + !x(1) * x(2) * x(3) + x(1) * !x(2) * x(3);
    // diagram2.set_labels({"_", "x2", "x3", "x1"});

    // printl(diagram1.to_dot_graph());
    // printl(diagram2.to_dot_graph());

    auto const timeTaken = watch.elapsed_time().count();
    printl("Done.");
    printl("Time taken: " + std::to_string(timeTaken) + " ms");

    return 0;
}