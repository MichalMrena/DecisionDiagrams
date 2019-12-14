#include <iostream>

#include "truth_table.hpp"
#include "half_symbolic_f.hpp"

#include "bdd_creator.hpp"
#include "bdd_merger.hpp"
#include "bdd_pla.hpp"

#include "utils/math_utils.hpp"
#include "utils/string_utils.hpp"
#include "test/diagram_tests.hpp"

using namespace mix::dd;
using namespace mix::utils;

auto main() -> int
{
    // auto table {truth_table::load_from_file("sample_table_2.txt")};
    
    // auto inputF {f<4>([](xs<4> x) {
    //     return (x[1] and x[2]) or x[4];
    // })};

    // bdd_creator creator;
    // bdd diagram1 {creator.create_diagram(table)};
    // bdd diagram2 {creator.create_diagram(inputF)};

    // std::cout << ( nand(x(1), x(3)) or (x(1) and x(2)) ).to_dot_graph() << '\n';

    bdds_from_pla<char, char> pla;
    // std::vector<bdd<char, char>> ds {pla.create("C:\\Users\\mrena\\Desktop\\pla_files\\LGSynth91\\pla\\apex1.pla")};

    auto diagram 
    {
        pla.create_diagram(
            std::vector<log_val_t> {0, X, 0, X, 1, X, 1}
          , 1
        )
    };

    std::cout << diagram.to_dot_graph();

    return 0;
}