#ifndef MIX_DD_DIAGRAM_TESTS
#define MIX_DD_DIAGRAM_TESTS

#include <string>
#include <sstream>
#include <array>
#include <bitset>
#include <iomanip>
#include <iostream>
#include <vector>
#include <cmath>
#include "../bin_decision_diagram.hpp"
#include "../utils/math_utils.hpp"
#include "../typedefs.hpp"

namespace mix::dd
{
    inline auto bool_to_char (bool bv) -> char
    {
        return bv ? '1' : '0';
    }
    
    template<class InputFunction>
    auto compare_results (const InputFunction& function, const bin_decision_diagram& diagram)
    {
        // TODO compare results for each input
    }

    template<size_t VarCount, class InputFunction>
    auto generate_truth_table (std::ostream& ostr, InputFunction f) -> void
    {
        std::array<size_t, VarCount> colWidths;
        for (size_t i {0}; i < VarCount; i++)
        {
            ostr << 'x' << (i + 1) << ' ';
            colWidths[i] = 2 + std::ceil(std::log10(i + 1.001));
        }
        ostr << ' ' << 'f' << '\n';

        for (input_t input {0}; input < utils::pow(2, 6); input++)
        {
            std::bitset<VarCount> inputBits {input};
            for (size_t i {VarCount}; i > 0;)
            {
                --i;
                ostr << std::setw(colWidths[i]) 
                     << std::left 
                     << bool_to_char(inputBits[i]);
            }
            ostr << ' ' << bool_to_char(f(inputBits)) << '\n';
        }
    }
}


#endif