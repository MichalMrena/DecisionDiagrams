#include "reliability.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <string_view>
#include <bitset>
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <sstream>
#include <functional>
#include <utility>
#include "bdd/bdd_manipulator.hpp"
#include "bdd/bdd_creator.hpp"

namespace mix
{
    using namespace mix::dd;

    using bdd_t      = bdd<double, empty_t>;
    using vertex_t   = typename bdd_t::vertex_t;
    using labels_v   = std::vector<std::string_view>;
    using probs_v    = std::vector<double>;
    using probs_v_v  = std::vector<probs_v>;
    using state_t    = std::bitset<10>;
    using states_v   = std::vector<state_t>;
    using states_v_v = std::vector<states_v>;
    using bdds_v     = std::vector<bdd_t>;
    using bool_v     = std::vector<bool_t>;

    namespace
    {
        auto availiability ( bdd_t&         function
                           , const probs_v& probabilities ) -> double
        {
            for (auto& vertex : function)
            {
                vertex.data = 0.0;
            }
            function.get_root()->data = 1.0;

            for (auto& vertex : function)
            {
                if (! vertex.is_leaf())
                {
                    vertex.son(0)->data += vertex.data * (1 - probabilities[vertex.index]);
                    vertex.son(1)->data += vertex.data * (probabilities[vertex.index]);
                }
            }

            return function.true_leaf()->data;
        }

        auto calculate_derivatives ( const bdd_t& structureFunction ) -> bdds_v
        {
            bdds_v ds;

            bdd_manipulator<double, empty_t> manipulator;
            for (size_t i (0); i < structureFunction.variable_count(); i++)
            {
                ds.emplace_back( manipulator.restrict_var(structureFunction.clone(), i, 0)
                               ^ manipulator.restrict_var(structureFunction.clone(), i, 1) );
            }

            return ds;
        }

        auto satisfying_set ( const bdd_t& function ) -> states_v
        {
            auto states {states_v {}};
            function.satisfy_all<state_t>(std::back_inserter(states));
            return states;
        }

        auto critical_states ( const bdds_v& derivatives ) -> states_v_v
        {
            auto states = states_v_v {};
            states.reserve(derivatives.size());

            for (auto i = 0ul; i < derivatives.size(); ++i)
            {
                std::unordered_set<state_t> uniqueStates;
                for (auto& state : satisfying_set(derivatives.at(i)))
                {
                    state &= state_t {~(1u << i)};
                    uniqueStates.insert(state);
                }
                states.emplace_back(uniqueStates.begin(), uniqueStates.end());
            }

            return states;
        }

        auto crit_states_probabilities ( bdds_v&        derivatives
                                       , const probs_v& probabilities )
        {
            probs_v critStatesProbs(derivatives.size());
            std::transform( derivatives.begin(), derivatives.end(), critStatesProbs.begin()
                          , [&probabilities](bdd_t& d) {return availiability(d, probabilities);} );

            return critStatesProbs;
        }

        auto dpbd_value ( const bdd_t&   derivative 
                        , const state_t& xs
                        , const index_t  i
                        , const bool_t   from ) -> bool_t
        {
            return xs[i] != from ? X : derivative.get_value(xs);
        }

        auto minimal_paths_or_cuts ( bdds_v&       derivatives
                                   , const index_t varCount 
                                   , const bool_t  from ) -> states_v
        {
            states_v   minPaths;
            var_vals_t state    (0);
            // TOSOLVE O(2^n) not good, maybe just check critical states?
            const auto endState (1u << varCount);

            while (state != endState) 
            {
                bool_v dpbdVals;
                for (index_t i (0); i < varCount; ++i)
                {
                    dpbdVals.push_back(dpbd_value(derivatives.at(i), state, i, from));
                }

                const auto minVal (*std::min_element(dpbdVals.begin(), dpbdVals.end()));

                if (1 == minVal)
                {
                    minPaths.push_back(state);
                }

                ++state;
            }

            return minPaths;
        }

        auto minimal_paths ( bdds_v&       derivatives
                           , const index_t varCount ) -> states_v
        {
            return minimal_paths_or_cuts(derivatives, varCount, 1);
        }

        auto minimal_cuts ( bdds_v&       derivatives
                          , const index_t varCount ) -> states_v
        {
            return minimal_paths_or_cuts(derivatives, varCount, 0);
        }

        auto print_states ( const states_v& states
                          , const index_t   varCount
                          , const index_t   i ) -> std::string
        {
            std::vector<std::string> statesStr(states.size());
            std::transform( states.begin(), states.end(), statesStr.begin()
                          , [varCount](const state_t& bs) 
                            {auto s {bs.to_string()}; return std::string {s.rbegin(), s.rend()}.substr(0, varCount);} );
            std::sort(statesStr.begin(), statesStr.end());
            std::transform( statesStr.begin(), statesStr.end(), statesStr.begin()
                          , [i, varCount](std::string s) {if (i < varCount) s[i] = '-'; return s;} );
            return std::accumulate( statesStr.begin(), statesStr.end(), std::string ("")
                                  , [](auto acc, auto s) {return acc + "    " + s + "\n";} );
        }

        auto birnbaum_importance ( const probs_v& probabilities
                                 , bdd_t&         derivative ) -> double
        {
            return availiability(derivative, probabilities);
        }

        auto structural_importances ( bdds_v& derivatives ) -> probs_v
        {
            auto importances = probs_v {};
            importances.reserve(derivatives.size());

            auto structural_importance = [](bdd_t& derivative)
            {
                auto const workingStatesNum = static_cast<double>(derivative.truth_density() / 2);
                auto const totalStatesNum   = utils::two_pow(derivative.variable_count() - 1);

                return workingStatesNum / totalStatesNum;
            };

            std::transform( std::begin(derivatives), std::end(derivatives), std::back_inserter(importances)
                          , structural_importance );

            return importances;
        }

        auto birnbaum_importances ( bdds_v&        derivatives
                                  , const probs_v& probabilities ) -> probs_v
        {
            using namespace std::placeholders;

            auto indices = probs_v {};
            indices.reserve(derivatives.size());

            std::transform( std::begin(derivatives), std::end(derivatives), std::back_inserter(indices)
                          , std::bind(birnbaum_importance, probabilities, _1) );

            return indices;
        }

        auto criticality_importnces ( bdd_t&         structureFunction
                                    , bdds_v&        derivatives
                                    , const probs_v& probabilities ) -> probs_v
        {
            const auto U {1 - availiability(structureFunction, probabilities)};
            auto indices {probs_v {}};

            for (auto i {0u}; i < derivatives.size(); ++i)
            {
                const auto bi {birnbaum_importance(probabilities, derivatives.at(i))};
                const auto q  {1 - probabilities.at(i)};
                
                indices.push_back(bi * (q / U));
            }
            
            return indices;
        }

        auto solve_example_week_3 ( bdd_t            structureFunction
                                  , labels_v         labels
                                  , probs_v          probabilities
                                  , std::string_view exampleName )
        {
            const auto varCount        {structureFunction.variable_count()};
                  auto derivatives     {calculate_derivatives(structureFunction)};
            const auto criticalStates  {critical_states(derivatives)};
            const auto critStatesProbs {crit_states_probabilities(derivatives, probabilities)};
            const auto minimalCuts     {minimal_cuts(derivatives, varCount)};
            const auto minimalPaths    {minimal_paths(derivatives, varCount)};

            std::cout << exampleName                                     << '\n'
                      << "Availiability = "
                      << availiability(structureFunction, probabilities) << '\n'
                      << "Critical states:"                              << '\n';
            for (size_t i {0}; i < varCount; ++i)
            {
                std::cout << "  x" << i << " " << labels.at(i)           
                          << " ; p = " << critStatesProbs.at(i)          << '\n'
                          << print_states(criticalStates.at(i), varCount, i);
            }
            std::cout << "Minimal cuts:"                                 <<'\n'
                      << print_states(minimalCuts, varCount, varCount + 1)
                      << "Minimal paths:"                                <<'\n'
                      << print_states(minimalPaths, varCount, varCount + 1);

            std::cout << "----------" << '\n' << '\n';
        }

        auto solve_example_week_5 ( bdd_t            structureFunction
                                  , labels_v         labels
                                  , probs_v          probabilities
                                  , std::string_view exampleName )
        {
            auto       derivatives   = calculate_derivatives(structureFunction);
            auto const varCount      = structureFunction.variable_count();
            auto const structureIs   = structural_importances(derivatives) ;
            auto const birnbaumIs    = birnbaum_importances(derivatives, probabilities);
            auto const criticalityIs = criticality_importnces(structureFunction, derivatives, probabilities);
        
            auto constexpr numColW  = 6;
            auto constexpr headColW = 18;

            std::cout << std::fixed << std::showpoint << std::setprecision(2);

            std::cout << exampleName << utils::EOL;
            std::cout << std::setw(headColW) << std::left << ' ' 
                      << std::setw(numColW)  << std::left << "IS"
                      << std::setw(numColW)  << std::left << "IB"
                      << std::setw(numColW)  << std::left << "IC "
                      << utils::EOL;
            for (auto i = 0ul; i < varCount ; ++i)
            {
                std::cout << std::setw(headColW) << std::left << labels.at(i)        
                          << std::setw(numColW)  << std::left << structureIs.at(i)    
                          << std::setw(numColW)  << std::left << birnbaumIs.at(i)     
                          << std::setw(numColW)  << std::left << criticalityIs.at(i) 
                          << utils::EOL;
            }
            std::cout << utils::EOL;
        }
    }

    auto solve_examples_week_3 () -> void
    {
        solve_example_week_3( x(0) * (x(1) + x(2) + x(3)) * x(4)
                            , {"Hospital_registry", "Department_1", "Department_2", "Department_3", "Pharmacy"}
                            , {0.8, 0.9, 0.9, 0.8, 0.7}
                            , "# Example 3" );

        solve_example_week_3( x(0) * (x(1) + x(2) + x(3)) * x(4)
                            , {"Hospital_registry", "Department_1", "Department_2", "Department_3", "Pharmacy"}
                            , {0.7, 0.8, 0.8, 0.8, 0.6}
                            , "# Example 4" );

        solve_example_week_3( x(0) * (x(1) + x(2) + x(3)) * (x(4) + x(5))
                            , {"Hospital_registry", "Department_1", "Department_2", "Department_3", "Pharmacy", "Pharmacy_1"}
                            , {0.8, 0.9, 0.9, 0.8, 0.7, 0.7}
                            , "# Example 5.0" );

        solve_example_week_3( x(0) * ( x(5) + ( (x(1) + x(2) + x(3)) * x(4) ) )
                            , {"Hospital_registry", "Department_1", "Department_2", "Department_3", "Pharmacy", "Pharmacy_1"}
                            , {0.8, 0.9, 0.9, 0.8, 0.7, 0.7}
                            , "# Example 5.1" );
    }

    auto solve_examples_week_5 () -> void
    {
        solve_example_week_5( x(0) * ( x(1) + x(2) )
                            , {"x1", "x2", "x3"}
                            , {0.8, 0.7, 0.5}
                            , "# Test" );

        solve_example_week_5( (x(0) * x(1)) + (x(0) * x(2)) + (x(1) * x(2))
                            , {"Hospital_registry", "Department_1", "Department_2", "Department_3", "Pharmacy"}
                            , {0.8, 0.7, 0.9}
                            , "# Example 2" );

        solve_example_week_5( x(0) * (x(1) + x(2) + x(3)) * x(4)
                            , {"Hospital_registry", "Department_1", "Department_2", "Department_3", "Pharmacy"}
                            , {0.8, 0.9, 0.9, 0.8, 0.7}
                            , "# Example 3" );

        solve_example_week_5( x(0) * (x(1) + x(2) + x(3)) * x(4)
                            , {"Hospital_registry", "Department_1", "Department_2", "Department_3", "Pharmacy"}
                            , {0.7, 0.8, 0.8, 0.8, 0.6}
                            , "# Example 4" );

        solve_example_week_5( x(0) * (x(1) + x(2) + x(3)) * (x(4) + x(5))
                            , {"Hospital_registry", "Department_1", "Department_2", "Department_3", "Pharmacy", "Pharmacy_1"}
                            , {0.8, 0.9, 0.9, 0.8, 0.7, 0.7}
                            , "# Example 5.0" );

        solve_example_week_5( x(0) * ( x(5) + ( (x(1) + x(2) + x(3)) * x(4) ) )
                            , {"Hospital_registry", "Department_1", "Department_2", "Department_3", "Pharmacy", "Pharmacy_1"}
                            , {0.8, 0.9, 0.9, 0.8, 0.7, 0.7}
                            , "# Example 5.1" );
    }
}