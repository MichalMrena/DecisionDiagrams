#include "diagram_tests.hpp"

#include <bitset>
#include "../bdd/bdd_creator.hpp"
#include "../bdd/pla_file.hpp"
#include "../bdd/pla_function.hpp"

namespace mix::dd
{
    namespace
    {
        using bits_s   = std::bitset<32>;
        using states_v = std::vector<bits_s>;

        auto naive_satisfy_all (const bdd<double, empty_t>& diagram) -> states_v
        {
            states_v states;

            var_vals_t state    {0};
            const auto endState {1u << diagram.variable_count()};

            while (state != endState) 
            {
                if (1 == diagram.get_value(state))
                {
                    states.push_back(bits_s {state});
                }
                ++state;
            }

            return states;
        }
    }

    auto random_pla_test
        ( const pla_function&          function
        , const bdd<empty_t, empty_t>& diagram
        , const uint32_t               runSeconds ) -> bool
    {
        using utils::printl;
        using bitset_t   = typename pla_function::input_bits_t;
        using rng_vals_t = utils::random_uniform_int<var_vals_t>;
        using watch_t    = utils::stopwatch;
        using millis_t   = typename utils::stopwatch::milliseconds;

        const auto     batchSize  {1'000'000};
        const millis_t maxRunTime {runSeconds * 1000};
        
        rng_vals_t rng;
        watch_t    watch;

        while (watch.elapsed_time() < maxRunTime)
        {
            for (size_t i {0}; i < batchSize; ++i)
            {
                const auto randomInput {(bitset_t {rng.next_int()} << 64) | bitset_t {rng.next_int()}};
                const auto expectedVal {function.get_f_val(randomInput)};
                const auto diagramVal  {diagram.get_value(randomInput)};

                if (expectedVal != diagramVal)
                {
                    printl("Output missmatch for input:");
                    printl(utils::to_string(randomInput, diagram.variable_count()));
                    return false;
                }
            }
        }

        printl("Diagram is correct.");
        return true;
    }

    auto test_pla_creator (const pla_file& file) -> bool
    {
        using bdd_cref_t = const bdd<empty_t, empty_t>&;

        bdd_creator<empty_t, empty_t> creator;

        const auto diagrams {creator.create_from_pla_p(file, merge_mode_e::iterative)};
        auto       function {pla_function::create_from_file(file)};

        const auto test 
        {
            file.variable_count() < 32 
                ? [](pla_function& a1, bdd_cref_t a2) { return full_test_diagram (a1, a2); }
                : [](pla_function& a1, bdd_cref_t a2) { return random_pla_test   (a1, a2); }
        };

        index_t fi {0};
        for (const auto& diagram : diagrams)
        {
            if (! test(function.at(fi++), diagram))
            {
                return false;
            }
        }

        return true;
    }

    auto test_constructors (const pla_file& file) -> bool
    {
        using creator_t = bdd_creator<empty_t, empty_t>;
        using bdd_t     = bdd<empty_t, empty_t>;

        creator_t creator;
        const auto diagrams {creator.create_from_pla(file, merge_mode_e::iterative)};

        // copy constructor
        bdd_t d1 {diagrams.at(0)};
        if (d1 != diagrams.at(0))
        {
            utils::printl("!!! Copy constructed diagram is not equal.");
            return false;
        }

        // copy assign
        bdd_t d2 {creator.just_val(0)};
        d2 = d1;
        if (d1 != d2)
        {
            utils::printl("!!! Copy assigned diagram is not equal.");
            return false;
        }

        // move constructor
        bdd_t d3 {std::move(d2)};
        if (d1 != d3)
        {
            utils::printl("!!! Move constructed diagram is not equal.");
            return false;
        }

        if (d2 == d3)
        {
            utils::printl("!!! Moved from diagram is equal to move constructed diagram.");
            return false;
        }

        // empty copy
        bdd_t e1 {creator.just_val(0)};
        bdd_t e2 {e1};
        if (e1 != e2)
        {
            utils::printl("!!! Empty copies are not equal.");
            return false;
        }

        utils::printl("Constructors are correct.");
        return true;
    }

    auto test_satisfy_all ( bdd<double, empty_t>& diagram ) -> bool
    {
        using utils::printl;
        using utils::concat;

        const auto naiveSet      = naive_satisfy_all(diagram);
        const auto calculatedSet = diagram.satisfy_all<bits_s>();

        if (naiveSet.size() != calculatedSet.size())
        {
            printl(concat("!!! Error different set sizes " , naiveSet.size(), " and  ", calculatedSet.size()));;
            return false;
        }

        if (calculatedSet.size() != diagram.truth_density())
        {
            printl(concat("!!! Truth density doesn't work got " , diagram.truth_density(), " expected  ", calculatedSet.size()));;
            return false;
        }

        printl("satisfy_all seems OK.");
        printl(concat("truth density is ", calculatedSet.size(), "\n"));

        return true;
    }
}