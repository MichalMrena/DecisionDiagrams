#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <type_traits>

#include "utils/stopwatch.hpp"
#include "utils/io.hpp"
#include "utils/string_utils.hpp"

#include "bdd_test/diagram_tests.hpp"
#include "bdd/bdd_creator.hpp"
#include "bdd/pla_file.hpp"
#include "bdd/pla_function.hpp"
#include "dd/mdd_creator.hpp"

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

    for (const auto& filename : fileNames)
    {
        const auto plaFile {pla_file::load_from_file(pla_dir_path() + filename)};
        test_pla_creator(plaFile);
    }
}

auto test_mdd ()
{
    mdd_creator<empty_t, empty_t, 3> creator;

    const auto firstMdd {creator.just_var(0)};
    printl(firstMdd.to_dot_graph());
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

auto main() -> int
{
    stopwatch watch;

    mix::solve_examples_week_5();

    // auto dd = (x(1) * x(2)) + (x(1) * x(3)) + (x(2) * x(3));
    // printl(dd.truth_density());
    // printl(dd.to_dot_graph());

    auto const timeTaken = watch.elapsed_time().count();
    printl("Done.");
    printl("Time taken: " + std::to_string(timeTaken) + " ms");

    return 0;
}