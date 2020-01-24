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
#include "bdd/bool_f_input.hpp"

using namespace mix::dd;
using namespace mix::utils;

using VertexDataType = empty;
using ArcDataType    = empty;

auto print_sizes (std::vector< bdd<empty, empty> >& diagrams) -> void
{
    for (auto& d : diagrams)
    {
        printl(std::to_string(d.vertex_count()));
    }
}

auto find_bug (const pla_file& file) -> void
{
    using bdd_t = bdd<empty, empty>;
    bdd_creator<empty, empty> creator;
    bdd_merger<empty, empty> merger;

    auto& lines         {file.get_lines()};
    size_t diagramCount {lines.size()};

    print_diagram(creator.create_product( lines.at(61).varVals.begin()
                                        , lines.at(61).varVals.end()
                                        , lines.at(61).fVals.at(0) ));
}

auto create_from_truth_table ()
{
    bdd_creator<VertexDataType, ArcDataType> creator;

    const auto table
    {
        truth_table::load_from_file("sample_table_2.txt")
    };

    auto diagramFromTable 
    {
        creator.create_from(table)
    };
}

auto create_from_lambda ()
{
    bdd_creator<VertexDataType, ArcDataType> creator;

    lambda_bool_f lambda
    {
        4, [] (const var_vals x) -> bool_t 
        {
            return ( x(0) && x(3) ) || ( x(1) && x(2) );
        }
    };

    auto bddFromLambda 
    {
        creator.create_from(lambda)
    };
}

auto create_from_pla_file ()
{
    bdd_creator<VertexDataType, ArcDataType> creator;

    const auto plaFile
    {
        pla_file::load_from_file("apex1.pla")
    };

    auto plaFunction
    {
        pla_function::create_from_file(plaFile)
    };

    auto diagramFromPla 
    {
        creator.create_from(plaFunction.at(0))
    };
}

auto build_from_pla_file ()
{
    bdds_from_pla<VertexDataType, ArcDataType> plaCreator;

    const auto plaFile
    {
        pla_file::load_from_file("apex4.pla")
    };

    auto bddsFromPla
    {
        plaCreator.create(plaFile)
    };

    bdd<VertexDataType, ArcDataType> firstDiagram
    {
        std::move(bddsFromPla.at(0))
    };
}

auto just_build ()
{
    bdd_creator<VertexDataType, ArcDataType> creator;
    // creator.create_product()
}

/*
    heuristiky na poradnie premenny
    generalisation of shannons expansion
    multiple val logic case
    decision diagrams case
*/
auto main() -> int
{
    stopwatch watch;

    create_from_truth_table();
    create_from_lambda();
    create_from_pla_file();

    build_from_pla_file();

    // ku každému diagramu by sa dal pridať ešte jeden, ktorý by mal 1 pre každý definovaný vstup,
    // vedel by potom vrátiť 0 1 alebo X
    // bdd_creator<empty, empty> creator;
    bdds_from_pla<empty, empty> plaCreator;

    pla_file plaFile
    {
        // pla_file::load_from_file("C:\\Users\\mrena\\Desktop\\pla_files\\LGSynth91\\pla\\apex1.pla")
        pla_file::load_from_file("/mnt/c/Users/mrena/Desktop/pla_files/LGSynth91/pla/apex4.pla")
    };

    // test_pla_creator(plaFile);
    // test_constructors(plaFile);    

    // auto bddFromTable  {creator.create_from(table)};
    // auto bddFromLambda {creator.create_from(lambda)};
    // auto bddFromDirect {( x(0) and x(3) ) or ( x(1) and x(2) )};
    auto bddsFromPla {plaCreator.create(plaFile)};

    // full_test_diagram(table, bddFromTable);
    // full_test_diagram(lambda, bddFromLambda);

    for (size_t i {0}; i < bddsFromPla.size(); i++)
    {
        printl(std::to_string(bddsFromPla.at(i).vertex_count()));
    }
    

    // printl(bdd<empty, empty>::just_var(1).negate().negate().to_dot_graph());
    // print_diagram(bddsFromPla.at(0));

    // find_bug(plaFile);

    printl("Done.");
    printl("Time taken: " + std::to_string(watch.elapsed_time().count()) + " ms");

    return 0;
}