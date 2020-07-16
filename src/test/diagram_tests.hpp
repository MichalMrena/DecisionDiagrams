#ifndef MIX_DD_DIAGRAM_TESTS_
#define MIX_DD_DIAGRAM_TESTS_

#include "pla_function.hpp"
#include "../bdd/bdd.hpp"
#include "../bdd/bdd_creator.hpp"
#include "../bdd/pla_file.hpp"
#include "../utils/math_utils.hpp"
#include "../utils/random_wrap.hpp"
#include "../utils/stopwatch.hpp"
#include "../utils/print.hpp"

#include <bitset>

namespace mix::dd
{
    auto test_pla (pla_file const& pla)
    {
        auto rng = utils::random_uniform_int<std::uint64_t> {};
        
        auto random_bits = [&]()
        {
            auto b1  = std::bitset<128> {rng.next_int()};
            auto b2  = std::bitset<128> {rng.next_int()};
            return (b1 << 64) | b2;
        };

        auto brute_force_compare = [&](auto const& bdds, auto const& plaFunc, auto const i)
        {
            using namespace utils;
            auto const endInput = two_pow(pla.variable_count());
            auto input = 0u;
            
            while (input != endInput)
            {
                auto const val1 = bdds[i].get_value(input);
                auto const val2 = plaFunc.get_value(input, i);
                if (val1 != val2)
                {
                    printl(concat("!!! Error for input " , input , " in function i = " , i));
                    return;
                }
                ++input;
            }
        };

        auto random_compare = [&](auto const& bdds, auto const& plaFunc, auto const i)
        {
            using namespace utils;
            auto const maxTime = stopwatch::milliseconds {10'000};
            auto watch = stopwatch {};
            while (watch.elapsed_time() < maxTime)
            {
                auto const input = random_bits();
                auto const val1  = bdds[i].get_value(input);
                auto const val2  = plaFunc.get_value(input, i);
                if (val1 != val2)
                {
                    printl(concat("!!! Error for input " , input.to_string() , " in function i = " , i));
                    return;
                }
            }
        };

        using creator_t = bdd_creator<double, void>;
        auto creator = creator_t {};

        auto const ds1 = creator.from_pla(pla);
        auto const ds2 = pla_function::from_file(pla);
        for (auto i = 0u; i < pla.function_count(); ++i)
        {
            pla.variable_count() < 25 ? brute_force_compare(ds1, ds2, i) 
                                      : random_compare(ds1, ds2, i);
        }
    }
}

#endif