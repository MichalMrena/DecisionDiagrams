#ifndef _MIX_DD_DIAGRAM_TESTS_
#define _MIX_DD_DIAGRAM_TESTS_

#include <iostream>
#include <bitset>
#include <stdexcept>
#include "../dd/typedefs.hpp"
#include "../bdd/bdd.hpp"
#include "../bdd/bool_function.hpp"
#include "../utils/math_utils.hpp"
#include "../utils/bits.hpp"
#include "../utils/random_uniform.hpp"
#include "../utils/stopwatch.hpp"
#include "../utils/io.hpp"

namespace mix::dd
{
    class pla_file;
    class pla_function;

    template< class BoolFunction
            , class GetFunctionValue = get_f_val<BoolFunction> >
    auto full_test_diagram 
        ( const BoolFunction&          function
        , const bdd<empty_t, empty_t>& diagram) -> bool
    {
        GetFunctionValue get_f_val;

        if (diagram.variable_count() > 31)
        {
            throw std::invalid_argument {"Too many variables for full test."};
        }

        const auto maxVarVals {static_cast<int64_t>(utils::two_pow(diagram.variable_count()))};
        bool testPassed {true};

        #pragma omp parallel for
        for (int64_t varVals = 0; varVals < maxVarVals; ++varVals)
        {
            const auto expectedVal {get_f_val(function, varVals)};
            const auto diagramVal  {diagram.get_value(static_cast<var_vals_t>(varVals))};

            testPassed &= expectedVal == diagramVal;
            if (expectedVal != diagramVal)
            {
                utils::printl("Output missmatch for input:");
                utils::printl(utils::to_string(varVals, diagram.variable_count()));
            }
        }

        if (testPassed)
        {
            utils::printl("Diagram is correct.");
        }

        return true;
    }

    auto test_pla_creator  ( const pla_file& file ) -> bool;
    auto test_constructors ( const pla_file& file ) -> bool;
    auto random_pla_test   ( const pla_function&          function
                           , const bdd<empty_t, empty_t>& diagram
                           , const uint32_t               runSeconds = 5) -> bool;
    auto test_satisfy_all  ( bdd<double, empty_t>& diagram ) -> bool;
        
}

#endif