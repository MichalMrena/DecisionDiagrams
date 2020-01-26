#include "bdd/truth_table.hpp"
#include "bdd/lambda_bool_f.hpp"
#include "bdd/bdd_creator.hpp"
#include "bdd/bdd_merger.hpp"
#include "bdd/bdd_pla.hpp"
#include "bdd/pla_function.hpp"
#include "utils/io.hpp"
#include "utils/stopwatch.hpp"
#include "data_structures/bit_vector.hpp"

using namespace mix::dd;
using namespace mix::utils;

using VertexDataType = empty_t;
using ArcDataType    = empty_t;

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
        pla_file::load_from_file("apex4.pla")
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

auto build_from_expression ()
{
    auto bddFromExpression 
    {
        (nand(x(0), x(2)) or nor(x(1), x(3))) and (x(2) xor not x(3))
    };
}

auto merge_example ()
{
    bdd<VertexDataType, ArcDataType> d1 {x(0) and x(1)};
    bdd<VertexDataType, ArcDataType> d2 {x(2) and x(3)};

    bdd_merger<VertexDataType, ArcDataType> merger;

    bdd<VertexDataType, ArcDataType> mergedDiagram {merger.merge(d1, d2, OR {})};

    // Same as above but much cleaner.
    auto mergedDiagramEasier {d1 || d2};
}

auto use_diagram ()
{
    const auto diagram 
    {
        (nand(x(0), x(2)) or nor(x(1), x(3))) and (x(2) xor not x(3))
    };

    const auto vertexCount {diagram.vertex_count()};
    const auto dotString   {diagram.to_dot_graph()};

    // x0 = 0, x1 = 0, x2 = 1, x3 = 1
    const std::vector<bool>     varValsVector {false, false, true, true};
    const std::bitset<4>        varValsBitset {"1100"};
    const var_vals_t            varValsNumber {0b1100};
    const bit_vector<1, bool_t> varValsBitVec {0, 0, 1, 1};

    const auto fValVector {diagram.get_value(varValsVector)};
    const auto fValBitset {diagram.get_value(varValsBitset)};
    const auto fValNumber {diagram.get_value(varValsNumber)};
    const auto fValBitVec {diagram.get_value(varValsBitVec)};

    if (!(fValVector == fValBitset && fValBitset == fValNumber && fValNumber == fValBitVec))
    {
        printl("Not good.");
    }
}

auto main() -> int
{
    stopwatch watch;

    create_from_truth_table();
    create_from_lambda();
    create_from_pla_file();

    build_from_pla_file();
    build_from_expression();

    use_diagram();
    
    printl("Done.");
    printl("Time taken: " + std::to_string(watch.elapsed_time().count()) + " ms");

    return 0;
}