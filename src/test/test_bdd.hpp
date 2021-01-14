#ifndef MIX_DD_BDD_TEST_HPP
#define MIX_DD_BDD_TEST_HPP

#include "test_base.hpp"
#include "../lib/bdd_manager.hpp"
#include "../lib/utils/more_functional.hpp"
#include "../lib/utils/string_utils.hpp"

#include <iostream>
#include <bitset>
#include <functional>
#include <cassert>

namespace mix::dd::test
{
    template<class T>
    using int_rng     = utils::random_uniform_int<T>;
    using seed_t      = unsigned int;
    using bool_var_v  = std::vector<bool_var>;
    using bool_var_vv = std::vector<bool_var_v>;

    auto constexpr BddVariableCount  = 22;
    auto constexpr BddProductCount   = 50;
    auto constexpr BddMaxProductSize = 5;

    struct boolean_function
    {
        bool_var_vv products;
    };

    inline auto generate_function( std::size_t const     productCount
                                 , int_rng<std::size_t>& rngProductSize
                                 , utils::random_bool&   rngIsComplemented
                                 , int_rng<index_t>&     rngVarIndex ) -> boolean_function
    {
        auto products = bool_var_vv(productCount);
        for (auto pi = 0u; pi < productCount; ++pi)
        {
            auto const productSize = rngProductSize.next_int();
            for (auto vi = 0u; vi <= productSize; ++vi)
            {
                auto const complemented = rngIsComplemented.next_bool();
                auto const var = bool_var {rngVarIndex.next_int(), complemented};
                products[pi].push_back(var);
            }
        }

        return boolean_function {std::move(products)};
    }

    template<class OrFold, class AndFold>
    inline auto make_diagram ( bdd_manager<void, void>& m
                             , boolean_function const&  function
                             , OrFold                   orFold
                             , AndFold                  andFold )
    {
        using bdd = typename bdd_manager<void, void>::bdd_t;

        auto productDiagrams = std::vector<bdd>();
        for (auto const& product : function.products)
        {
            auto varDiagrams = m.variables(product);
            productDiagrams.push_back(andFold(varDiagrams));
        }

        return orFold(productDiagrams);
    }

    template<std::size_t N>
    inline auto eval_function ( boolean_function const& function
                              , std::bitset<N> const    varVals )
    {
        auto const& pss  = function.products;
        auto productVals = std::vector<bool>();
        // TODO transform reduce
        std::transform(std::begin(pss), std::end(pss), std::back_inserter(productVals), [&varVals](auto const& ps)
        {
            return std::all_of(std::begin(ps), std::end(ps), [&varVals](auto const bv)
            {
                return bv.complemented ? !varVals[bv.index] : varVals[bv.index];
            });
        });
        return std::any_of(std::begin(productVals), std::end(productVals), utils::identity);
    }

    inline auto test_bdd ( std::size_t const n
                         , order_e const     order = order_e::Default
                         , seed_t const      seed  = 0u )
    {
        auto initSeed          = 0ul == seed ? std::random_device () () : seed;
        auto seeder            = int_rng<seed_t>(0u, UIntMax, initSeed);
        auto rngProductSize    = int_rng<std::size_t>(1, BddMaxProductSize, seeder.next_int());
        auto rngIsComplemented = utils::random_bool(seeder.next_int());
        auto rngVarIndex       = int_rng<index_t>(0, BddVariableCount - 1, seeder.next_int());
        auto rngOrderShuffle   = std::mt19937(seeder.next_int());

        std::cout << "Running " << n << " tests."     << '\n';
        std::cout << "    Seed:         " << initSeed << '\n' << '\n';

        for (auto i = 0u; i < n; ++i)
        {
            // Create manager and set order of variables.
            auto manager  = bdd_manager<void, void>(BddVariableCount);
            auto const os = get_order(order, rngOrderShuffle, BddVariableCount);
            manager.set_order(os);

            // Different folding strategies.
            auto const andLeftFold = [&manager](auto&& ds){ return manager.template left_fold<AND>(ds); };
            auto const orLeftFold  = [&manager](auto&& ds){ return manager.template left_fold<OR>(ds); };
            auto const andTreeFold = [&manager](auto&& ds){ return manager.template tree_fold<AND>(ds); };
            auto const orTreeFold  = [&manager](auto&& ds){ return manager.template tree_fold<OR>(ds); };

            // Generate function and create diagrams using different strategies.
            auto const function = generate_function(BddProductCount, rngProductSize, rngIsComplemented, rngVarIndex);
            auto const diagram  = make_diagram(manager, function, orLeftFold, andLeftFold);
            auto const diagram2 = make_diagram(manager, function, orTreeFold, andTreeFold);
            manager.collect_garbage();
            auto const vertexCount = manager.vertex_count(diagram);

            assert(diagram == diagram2);
            assert(1 == manager.vertex_count(diagram.get_root()->get_index()));
            assert(vertexCount == manager.vertex_count());

            std::cout << '#' << i                            << '\n';
            std::cout << "    Vertex count: " << vertexCount << '\n';
            std::cout << "    Order:        " << utils::concat_range(os, " > ") << '\n';

            // TODO move to function test_evaluate
            auto result = true;
            for (auto varVals = 0u; varVals < (1 << BddVariableCount); ++varVals)
            {
                auto const bits       = std::bitset<BddVariableCount>(varVals);
                auto const realVal    = eval_function(function, bits);
                auto const diagramVal = manager.evaluate(diagram, bits);
                if (realVal != diagramVal)
                {
                    std::cout << "    !!! Error output missmatch. " << '\n';
                    result = false;
                    break;
                }
            }

            std::cout << (result ? "    Result:       OK"
                                 : "    Result:       Failed.") << "\n\n";
        }
    }
}

#endif