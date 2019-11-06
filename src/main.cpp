#include <iostream>

#include "truth_table.hpp"
#include "half_symbolic_f.hpp"

#include "bdd_creator.hpp"
#include "bdd_merger.hpp"

#include "utils/math_utils.hpp"
#include "utils/string_utils.hpp"
#include "test/diagram_tests.hpp"

using namespace mix::dd;
using namespace mix::utils;

auto main() -> int
{
    auto table {truth_table::load_from_file("sample_table_2.txt")};
    
    auto inputF {create_hs<4>([](xs<4> x) {
        return (x[1] and x[2]) or x[4];
    })};

    // bdd_creator creator;
    // bdd diagram1 {creator.create_diagram(table)};
    // bdd diagram2 {creator.create_diagram(inputF)};

    // compare_results(table, diagram1);
    // compare_results(inputF, diagram2);

    // std::cout << diagram1.to_dot_graph() << '\n';
    // std::cout << diagram2.to_dot_graph() << '\n';

    // bdd diagram3 {x(1) and x(2)};

    bdd_merger merger;
    // bdd merged {diagram1 && diagram2};

    using t_bdd = bdd<int, int>;

    bdd x1 {t_bdd::VARIABLE(1)};
    bdd x2 {t_bdd::VARIABLE(2)};

    bdd x1orx2 {x1 || x2};

    std::cout << x1orx2.to_dot_graph() << '\n';

    return 0;
}