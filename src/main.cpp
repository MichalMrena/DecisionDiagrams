#include <iostream>
#include "graph.hpp"
#include "truth_table.hpp"
#include "utils/math_utils.hpp"

using namespace mix::dd;
using namespace mix::utils;

auto main() -> int
{
    auto table {truth_table::load_from_file("sample_table.txt")};

    table.to_string(std::cout);

    return 0;
}