#include <iostream>

#include "bdd/truth_table.hpp"
#include "bdd/lambda_bool_f.hpp"
#include "bdd/bdd_creator.hpp"
#include "bdd/bdd_merger.hpp"
#include "bdd/bdd_pla.hpp"
#include "bdd/pla_function.hpp"

#include "utils/math_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/file_reader.hpp"
#include "utils/bits.hpp"
#include "utils/random_uniform.hpp"
#include "utils/stopwatch.hpp"

#include "data_structures/list_map.hpp"
#include "data_structures/bit_vector.hpp"

#include "bdd_test/diagram_tests.hpp"
#include "bdd_test/pla_tests.hpp"
#include "bdd_test/other_tests.hpp"

using namespace mix::dd;
using namespace mix::utils;

/*
    heuristiky na poradnie premenny
    generalisation of shannons expansion
    multiple val logic case
    decision diagrams case
*/
auto main() -> int
{
    stopwatch watch;

    bdd_creator<empty, empty> creator;
    bdds_from_pla<empty, empty> placreator;

    truth_table table
    {
        truth_table::load_from_file("sample_table_2.txt")
    };

    lambda_bool_f lambda
    {
        6
      , [](const var_vals x) -> bool_t {
            return ( x(0) and x(1) and x(2) ) or ( x(3) and x(1) );
        }
    };

    pla_file plaFile
    {
        // pla_function::load_from_file("C:\\Users\\mrena\\Desktop\\pla_files\\LGSynth91\\pla\\con1.pla")
        pla_file::load_from_file("/mnt/c/Users/mrena/Desktop/pla_files/LGSynth91/pla/apex2.pla")
    };

    auto pla {placreator.create(plaFile)};

    // test_pla_creator(plaFile);
    // test_constructors(plaFile);    

    // auto bddFromTable  {creator.create_from(table)};
    // auto bddFromLambda {creator.create_from(lambda)};

    // full_test_diagram(table, bddFromTable);
    // full_test_diagram(lambda, bddFromLambda);

    // print_diagram(bddFromTable);
    // print_diagram(bddFromLambda);    

    printl("Done.");
    printl("Time taken: " + std::to_string(watch.elapsed_time().count()) + " ms");

    return 0;
}