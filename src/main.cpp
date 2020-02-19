#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>

#include "bdd/bdd_tools.hpp"
#include "bdd_test/diagram_tests.hpp"
#include "utils/stopwatch.hpp"

using namespace mix::dd;
using namespace mix::utils;

using VertexDataType = empty_t;
using ArcDataType    = empty_t;
using bdd_t          = bdd<VertexDataType, ArcDataType>;
using merger_t       = bdd_merger<VertexDataType, ArcDataType>;
using diagram_tools  = bdd_tools<VertexDataType, ArcDataType>;

auto pla_path () -> std::string
{
    return "/mnt/c/Users/mrena/Desktop/pla_files/LGSynth91/pla/";
}

auto pla_adder_path () -> std::string
{
    return "/mnt/c/Users/mrena/Desktop/pla_files/Adders/";
}

auto compare_merges ()
{
    using bdd_pla_t = bdds_from_pla<empty_t, empty_t>;

    const auto fileNames = {"apex5", "pdc", "spla", "seq", "ex4", "cps", "misex3", "apex2",
                            "cordic", "alu4", "ex1010", "apex1",};
    const auto iterations {1};
    const auto colWidth   {14};

    std::map<std::string, stopwatch::milliseconds> sequentialTimes;
    std::map<std::string, stopwatch::milliseconds> iterativeTimes;
    std::map<std::string, stopwatch::milliseconds> parallelTimes;
    std::map<std::string, stopwatch::milliseconds> smartTimes;
    std::map<std::string, size_t> iterativeMaxSizes;
    std::map<std::string, size_t> smartMaxSizes;
    bdd_pla_t plaCreator;

    for (const auto& fileName : fileNames)
    {
        const auto plaFile
        {
            pla_file::load_from_file(pla_path() + fileName + ".pla")
        };

        // {
        //     const auto ds  {plaCreator.create_s(plaFile)};
        //     const auto di  {plaCreator.create_i(plaFile)};
        //     const auto dip {plaCreator.create_ip(plaFile)};

        //     for (size_t i {0}; i < ds.size(); i++)
        //     {
        //         if (ds.at(i) != di.at(i) || di.at(i) != dip.at(i))
        //         {
        //             std::cout << "!!! error not equal" << '\n';
        //             return;
        //         }
        //     }
        // }

        // const auto timeSequential {stopwatch::avg_run_time(iterations, [&plaFile, &plaCreator]()
        // {
        //     plaCreator.create_s(plaFile);
        // })};

        // const auto timeIterative {stopwatch::avg_run_time(iterations, [&plaFile, &plaCreator]()
        // {
        //     plaCreator.create_i(plaFile);
        // })};

        // const auto timeParallel {stopwatch::avg_run_time(iterations, [&plaFile, &plaCreator]()
        // {
        //     plaCreator.create_ip(plaFile);
        // })};

        const auto timeSmart {stopwatch::avg_run_time(iterations, [&plaFile, &plaCreator]()
        {
            plaCreator.create_smart(plaFile);
        })};

        const auto diagramsIterative {plaCreator.create_i(plaFile)};
        const auto diagramsSmart     {plaCreator.create_smart(plaFile)};

        size_t maxSizeIterative {0};
        for (const auto& d : diagramsIterative)
        {
            maxSizeIterative = std::max(maxSizeIterative, d.vertex_count());
        }

        size_t maxSizeSmart {0};
        for (const auto& d : diagramsSmart)
        {
            maxSizeSmart = std::max(maxSizeSmart, d.vertex_count());
        }

        // sequentialTimes.emplace(fileName, timeSequential);
        // iterativeTimes.emplace(fileName, timeIterative);
        // parallelTimes.emplace(fileName, timeParallel);
        smartTimes.emplace(fileName, timeSmart);
        iterativeMaxSizes.emplace(fileName, maxSizeIterative);
        smartMaxSizes.emplace(fileName, maxSizeSmart);
    }

    std::cout << std::left;
    std::cout << std::setw(colWidth) << "pla file" 
            //   << std::setw(colWidth) << "sequential" 
              << std::setw(colWidth) << "iterative sz" 
            //   << std::setw(colWidth) << "parallel" 
              << std::setw(colWidth) << "smart" 
              << std::setw(colWidth) << "smart sz" 
              << '\n';

    for (const auto& fileName : fileNames)
    {
        std::cout << std::setw(colWidth) << fileName
                //   << std::setw(colWidth) << sequentialTimes.at(fileName).count()
                //   << std::setw(colWidth) << iterativeTimes.at(fileName).count()
                //   << std::setw(colWidth) << parallelTimes.at(fileName).count()
                  << std::setw(colWidth) << iterativeMaxSizes.at(fileName)
                  << std::setw(colWidth) << smartTimes.at(fileName).count()
                  << std::setw(colWidth) << smartMaxSizes.at(fileName)
                  << '\n';
    }
}

auto pla_max_size (const std::string& name)
{
    bdds_from_pla<VertexDataType, ArcDataType> creator;

    auto plaFile
    {
        pla_file::load_from_file(pla_path() + name + ".pla")
    };

    const auto betterFile
    {
        improve_ordering(plaFile)
    };

    const auto diagrams
    {
        creator.create_i(betterFile)
    };

    const auto maxSize {std::max_element(diagrams.begin(), diagrams.end(), 
        [](const auto& lhs, const auto& rhs) 
    { 
        return lhs.vertex_count() < rhs.vertex_count();
    })};

    pla_file::save_to_file(name + "_changed.pla", plaFile);
    printl((*maxSize).vertex_count());
}

auto test_big_pla ()
{
    bdds_from_pla<VertexDataType, ArcDataType> creator;

    auto plaFile
    {
        pla_file::load_from_file(pla_adder_path() + "16-adder_col.pla")
    };

    // const auto improvedFile
    // {
    //     improve_ordering(plaFile)
    // };

    // pla_file::save_to_file("10-adder_col_reordered.pla", improvedFile);

    const auto iterativeDiagrams
    {
        creator.create_i(plaFile)
    };

    // const auto sequentialDiagrams
    // {
    //     creator.create_p(plaFile)
    // };

    // for (size_t i {0}; i < iterativeDiagrams.size(); ++i)
    // {
    //     if (iterativeDiagrams.at(i) != sequentialDiagrams.at(i))
    //     {
    //         printl("Not good.");
    //     }
    //     else
    //     {
    //         printl("Ok.");
    //     }
    // }

    for (const auto& diagram : iterativeDiagrams)
    {
        printl(diagram.vertex_count());
    }

    // printl(iterativeDiagrams.back().to_dot_graph());

    // printl(bdd_merger<VertexDataType, ArcDataType>::avg.average());
}

auto unordered_merge_test ()
{
    auto d1 {x(1) + x(2) + x(3)};
    auto d2 {x(4) * x(2) * x(6)};

    merger_t merger;

    auto newDiagram {merger.merge_unordered(d1, d2, AND {})};

    printl(newDiagram.to_dot_graph());
}

auto iter_vs_seq_test ()
{
    const auto files = {""};
}

auto better_merger_test ()
{
    // const auto d1 {!(x(1) * x(3))};
    const auto d1 {((x(1) + x(2)) * x(3)) + x(4)};
    // const auto d2 { (x(2) * x(3))};
    const auto d2 {(x(1) * (!x(3))) + x(4)};
    merger_t merger;

    const auto dd {merger.merge_reduced(d1, d2, OR{})};

    printl(dd.to_dot_graph());
}

auto main() -> int
{
    stopwatch watch;

    std::ofstream fout {"output.txt"};
    std::ostream& ost  {std::cout};
    // std::ostream& ost  {fout};

    // unordered_merge_test();
    test_big_pla();
    // better_merger_test();        

    const auto timeTaken {watch.elapsed_time().count()};
    printl("Done.");
    printl("Time taken: " + std::to_string(timeTaken) + " ms");

    return 0;
}