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

    bdd_creator creator;
    bdd diagram1 {creator.create_diagram(table)};
    bdd diagram2 {creator.create_diagram(inputF)};

    // compare_results(table, diagram1);
    // compare_results(inputF, diagram2);

    // std::cout << diagram1.to_dot_graph() << '\n';
    // std::cout << diagram2.to_dot_graph() << '\n';

    // bdd_merger merger;
    // bdd merged {diagram1 && diagram2};
    // std::cout << merged.to_dot_graph() << '\n';

    // bdd x1orx2 {x(1) or x(2)};
    // std::cout << x1orx2.to_dot_graph() << '\n';

    using vertex = graph<def_vertex_data_t, def_arc_data_t>::vertex;

    vertex* const v0 {new vertex {0, 1}};
    
    vertex* const v1 {new vertex {1, 2}};
    vertex* const v2 {new vertex {2, 2}};

    vertex* const v3 {new vertex {3, 3}};
    vertex* const v4 {new vertex {4, 3}};

    vertex* const v5 {new vertex {5, 4}};
    vertex* const v6 {new vertex {6, 4}};
    vertex* const v7 {new vertex {7, 4}};
    vertex* const v8 {new vertex {8, 4}};
    
    std::map<const vertex*, log_val_t> ltv
    {
        {v5, 0}
      , {v6, 1}
      , {v7, 0}
      , {v8, 1}
    };

    v0->forwardStar[0].target = v1;
    v0->forwardStar[1].target = v2;

    v1->forwardStar[0].target = v5;
    v1->forwardStar[1].target = v3;

    v2->forwardStar[0].target = v3;
    v2->forwardStar[1].target = v4;

    v3->forwardStar[0].target = v7;
    v3->forwardStar[1].target = v6;

    v4->forwardStar[0].target = v7;
    v4->forwardStar[1].target = v8;
    
    bdd notreduced {v0, 3, std::move(ltv)};

    bdd_reducer{}.reduce(notreduced);

    std::cout << notreduced.to_dot_graph() << '\n';

    return 0;
}