#include <iostream>
#include "graph.hpp"
#include "truth_table.hpp"
#include "bdd_creator.hpp"
#include "utils/math_utils.hpp"
#include "test/diagram_tests.hpp"

using namespace mix::dd;
using namespace mix::utils;

auto main() -> int
{
    auto table {truth_table::load_from_file("sample_table_2.txt")};
    
    bdd_creator<decltype(table)> creator {std::move(table)};

    auto diagram {creator.create_diagram()};

    std::cout << diagram.to_dot_graph() << '\n';

    // generate_truth_table<6>(std::cout, [](const std::bitset<6>& x) {
    //     // x1.x2 + x3.x4 + x5.x6        
    //     return (x[5] and x[4]) or (x[3] and x[2]) or (x[1] and x[0]);
    // });

    return 0;
}