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
#include "../bdd.hpp"
#include "../bool_function.hpp"
#include "../utils/math_utils.hpp"
#include "../utils/string_utils.hpp"
#include "../typedefs.hpp"

namespace mix::dd
{
    inline auto bool_to_char (bool bv) -> char
    {
        return bv ? '1' : '0';
    }
    
    template<class VertexData = int8_t, class ArcData = int8_t>
    auto compare_results (const bool_function& function
                        , const bdd<VertexData, ArcData>& diagram) -> bool
    {
        const size_t inputCount {utils::pow(2UL, function.variable_count())};
        for (size_t i {0}; i < inputCount; ++i)
        {
            const log_val_t expectedValue {function[i]};
            const log_val_t diagramValue  {diagram.get_value(i)};

            if (expectedValue != diagramValue)
            {
                std::cout << "Wrong answer for input " << utils::to_bit_string(i)
                          << " expected " << std::to_string(expectedValue)
                          << " got "      << std::to_string(diagramValue)
                          << '\n';

                return false;
            }
        }

        std::cout << "Diagram is correct." << '\n';

        return true;
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

        for (input_t input {0}; input < utils::pow(2, VarCount); input++)
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