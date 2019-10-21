#include <iostream>
#include "graph.hpp"
#include "truth_table.hpp"
#include "bin_dd_creator.hpp"
#include "utils/math_utils.hpp"

using namespace mix::dd;
using namespace mix::utils;

auto main() -> int
{
    auto table {truth_table::load_from_file("sample_table.txt")};
    
    bin_dd_creator<decltype(table)> creator {std::move(table)};

    auto diagram {creator.create_diagram()};

    std::cout << diagram.to_dot_graph() << '\n';
    
    std::cout << static_cast<unsigned>(diagram.get_value(0)) << '\n';
    std::cout << static_cast<unsigned>(diagram.get_value(1)) << '\n';
    std::cout << static_cast<unsigned>(diagram.get_value(2)) << '\n';
    std::cout << static_cast<unsigned>(diagram.get_value(3)) << '\n';
    std::cout << static_cast<unsigned>(diagram.get_value(4)) << '\n';
    std::cout << static_cast<unsigned>(diagram.get_value(5)) << '\n';
    std::cout << static_cast<unsigned>(diagram.get_value(6)) << '\n';
    std::cout << static_cast<unsigned>(diagram.get_value(7)) << '\n';

    return 0;
}