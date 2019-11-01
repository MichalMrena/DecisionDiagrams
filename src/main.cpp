#include <iostream>
#include "graph.hpp"
#include "bool_function.hpp"
#include "bdd_creator.hpp"
#include "utils/math_utils.hpp"
#include "test/diagram_tests.hpp"

using namespace mix::dd;
using namespace mix::utils;

auto main() -> int
{
    auto table {bool_function::load_from_file("sample_table_2.txt")};
    
    bdd_creator creator {};

    auto diagram {creator.create_diagram(table)};

    std::cout << diagram.to_dot_graph() << '\n';

    // generate_bool_function<6>(std::cout, [](const std::bitset<6>& x) {
    //     // x1.x2 + x3.x4 + x5.x6        
    //     // x1.x4 + x2.x5 + x3.x6        
    //     return (x[5] and x[2]) or (x[4] and x[1]) or (x[3] and x[0]);
    // });

    return 0;
}