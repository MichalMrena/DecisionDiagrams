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

    return 0;
}