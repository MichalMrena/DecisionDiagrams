#ifndef BDD_TEST_HPP
#define BDD_TEST_HPP

#include <vector>
#include <iostream>
#include <limits>
#include <bitset>
#include <algorithm>
#include <functional>
#include <cassert>

#include "../lib/bdd_manager.hpp"
#include "../lib/utils/more_random.hpp"
#include "../lib/utils/string_utils.hpp"

namespace mix::dd::test
{
    template<class T>
    using int_rng     = utils::random_uniform_int<T>;
    using seed_t      = unsigned int;
    using bool_var_v  = std::vector<bool_var>;
    using bool_var_vv = std::vector<bool_var_v>;

    struct boolean_function
    {
        bool_var_vv products;
    };

    enum class order_e
    {
        Default,
        Random
    };

    auto constexpr UIntMax        = std::numeric_limits<unsigned int>::max();
    auto constexpr VariableCount  = 22;
    auto constexpr ProductCount   = 50;
    auto constexpr MaxProductSize = 5;
    auto constexpr identity       = [](auto const a){ return a; };

    inline auto generate_function( int_rng<std::size_t>&  rngProductSize
                                 , int_rng<unsigned int>& rngIsComplemented
                                 , int_rng<index_t>&      rngVarIndex ) -> boolean_function
    {
        auto products = bool_var_vv(ProductCount);
        for (auto pi = 0u; pi < ProductCount; ++pi)
        {
            auto const productSize = rngProductSize.next_int();
            for (auto vi = 0u; vi <= productSize; ++vi)
            {
                auto const complemented = static_cast<bool>(rngIsComplemented.next_int());
                auto const var = bool_var {rngVarIndex.next_int(), complemented};
                products[pi].push_back(var);
            }
        }

        return boolean_function {std::move(products)};
    }

    inline auto make_diagram(bdd_manager<void, void>& m, boolean_function const& function)
    {
        using bdd = typename bdd_manager<void, void>::bdd_t;

        auto productDiagrams = std::vector<bdd>();
        for (auto const& product : function.products)
        {
            auto const varDiagrams = m.just_vars(product);
            productDiagrams.push_back(m.left_fold(varDiagrams, AND()));
        }

        return m.left_fold(productDiagrams, OR());
    }

    inline auto eval_function(boolean_function const& function, std::bitset<VariableCount> const varVals)
    {
        auto const& pss  = function.products;
        auto productVals = std::vector<bool>();
        std::transform(std::begin(pss), std::end(pss), std::back_inserter(productVals), [&varVals](auto const& ps)
        {
            return std::all_of(std::begin(ps), std::end(ps), [&varVals](auto const bv)
            {
                return bv.complemented ? !varVals[bv.index] : varVals[bv.index];
            });
        });
        return std::any_of(std::begin(productVals), std::end(productVals), identity);
    }

    inline auto get_default_order()
    {
        auto is = std::vector<index_t>(VariableCount);
        std::iota(std::begin(is), std::end(is), 0);
        return is;
    }

    inline auto get_random_order(std::mt19937& rngOrder)
    {
        auto is = get_default_order();
        std::shuffle(std::begin(is), std::end(is), rngOrder);
        return is;
    }

    inline auto get_order(order_e const o, std::mt19937& rngOrder)
    {
        switch (o)
        {
            case order_e::Default: return get_default_order();
            case order_e::Random:  return get_random_order(rngOrder);
            default: throw "not good";
        }
    }

    inline auto test_bdd(std::size_t const n, order_e const order = order_e::Default, seed_t const seed = 0u)
    {
        auto initSeed          = 0ul == seed ? std::random_device () () : seed;
        auto seeder            = int_rng<seed_t>(0u, UIntMax, initSeed);
        auto rngProductSize    = int_rng<std::size_t>(1, MaxProductSize, seeder.next_int());
        auto rngIsComplemented = int_rng<unsigned int>(0u, 1u, seeder.next_int());
        auto rngVarIndex       = int_rng<index_t>(0, VariableCount - 1, seeder.next_int());
        auto rngOrderShuffle   = std::mt19937(seeder.next_int());

        for (auto i = 0u; i < n; ++i)
        {
            auto manager        = bdd_manager<void, void>(VariableCount);
            auto const os       = get_order(order, rngOrderShuffle);
            manager.set_order(os);
            auto const function = generate_function(rngProductSize, rngIsComplemented, rngVarIndex);
            auto const diagram  = make_diagram(manager, function);
            auto result         = true;
            manager.collect_garbage();
            auto const vertexCount = manager.vertex_count(diagram);

            assert(1 == manager.vertex_count(diagram.get_root()->get_index()));
            assert(vertexCount == manager.vertex_count());

            std::cout << "Test #" << i                       << '\n';
            std::cout << "    Seed:         " << initSeed    << '\n';
            std::cout << "    Vertex count: " << vertexCount << '\n';
            std::cout << "    Order:        " << utils::concat_range(os, " > ") << '\n';

            for (auto varVals = 0u; varVals < (1 << VariableCount); ++varVals)
            {
                auto const bits       = std::bitset<VariableCount>(varVals);
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