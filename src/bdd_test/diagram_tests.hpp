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
    template< class BoolFunction
            , class GetFVal  = get_f_val<BoolFunction>
            , class VarCount = var_count<BoolFunction> >
    auto full_test_diagram 
        ( const BoolFunction&      function
        , const bdd<empty_t, empty_t>& diagram) -> bool
    {
        GetFVal get_f_val;
        VarCount var_count;

        if (var_count(function) > 25)
        {
            throw std::invalid_argument {"Too many variables."};
        }

        const auto maxVarVals {utils::two_pow(var_count(function))};
        var_vals_t varVals    {0};

        while (varVals < maxVarVals)
        {
            const auto expectedVal {get_f_val(function, varVals)};
            const auto diagramVal  {diagram.get_value(varVals)};

            if (expectedVal != diagramVal)
            {
                utils::printl("Output missmatch for input:");
                utils::printl(utils::to_string(varVals, var_count(function)));
                return false;
            }

            ++varVals;
        }

        utils::printl("Diagram is correct.");
        return true;
    }

    template< class BoolFunction
            , class GetFVal  = get_f_val<BoolFunction>
            , class VarCount = var_count<BoolFunction> >
    auto random_test_diagram 
        ( const BoolFunction&      function
        , const bdd<empty_t, empty_t>& diagram
        , const uint32_t           runSeconds = 5) -> bool
    {
        using watch_t    = utils::stopwatch;
        using millis_t   = typename utils::stopwatch::milliseconds;
        using rng_vals_t = utils::random_uniform_int<var_vals_t>;

        GetFVal get_f_val;
        VarCount var_count;

        const auto batchSize      {1'000};
        const auto maxVarVals     {utils::two_pow(var_count(function)) - 1};
        const millis_t maxRunTime {runSeconds * 1000};
        
        rng_vals_t rng {0, maxVarVals};
        watch_t watch;

        size_t exp0 {0};
        size_t exp1 {0};

        do
        {
            for (size_t i {0}; i < batchSize; ++i)
            {
                const auto varVals     {rng.next_int()};
                const auto expectedVal {get_f_val(function, varVals)};
                const auto diagramVal  {diagram.get_value(varVals)};

                exp0 += 0 == expectedVal;
                exp1 += 1 == expectedVal;

                if (expectedVal != diagramVal)
                {
                    utils::printl("Output missmatch for input:");
                    utils::printl(utils::to_string(varVals, var_count(function)));
                    return false;
                }
            }
        } while (watch.elapsed_time() < maxRunTime);

        // utils::printl("Zero outputs tested: " + std::to_string(exp0));
        // utils::printl("One outputs tested: " + std::to_string(exp1));

        utils::printl("Diagram is correct.");
        return true;
    }

    inline auto print_diagram (const bdd<empty_t, empty_t>& diagram) -> void
    {
        std::cout << diagram.to_dot_graph() << '\n';
    } 
}

#endif