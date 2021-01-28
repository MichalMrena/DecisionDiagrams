#ifndef MIX_DD_TEST_TEST_RELIABILITY_HPP
#define MIX_DD_TEST_TEST_RELIABILITY_HPP

#include "../lib/bdd_manager.hpp"
#include <cmath>
#include <string>

namespace mix::dd::test
{
    using bdd_man_t = bdd_manager<double, void>;
    using bdd_t     = mdd<double, void, 2>;
    using double_v  = std::vector<double>;

    template<std::size_t N>
    struct bss_characteristic
    {
        using bitset_v = std::vector<std::bitset<N>>;

        double   A;
        double   U;
        double_v SIs;
        double_v BIs;
        double_v CIs;
        double_v FIs;
        bitset_v MCVs;
    };

    auto constexpr equal_enough(double const l, double const r)
    {
        return std::abs(l - r) < 0.00001;
    }

    template<class Range1, class Range2>
    auto const equal_set(Range1&& r1, Range2&& r2)
    {
        return std::size(r1) == std::size(r2)
           and std::is_permutation(std::begin(r1), std::end(r1), std::begin(r2));
    }

    template<std::size_t N>
    auto compare_bss( bss_characteristic<N> const& got
                    , bss_characteristic<N> const& expected )
    {
        if (not equal_enough(got.A, expected.A))
        {
            return utils::concat("Availability missmatch. Got ", got.A, " expected ", expected.A);
        }

        if (not equal_enough(got.U, expected.U))
        {
            return utils::concat("Unavailability missmatch. Got ", got.U, " expected ", expected.U);
        }

        if (not std::equal(std::begin(got.SIs), std::end(got.SIs), std::begin(expected.SIs), equal_enough))
        {
            return utils::concat( "SI missmatch. Got [", utils::concat_range(got.SIs, ", ")
                                , "] expected [", utils::concat_range(expected.SIs, ", "), "]." );
        }

        if (not std::equal(std::begin(got.BIs), std::end(got.BIs), std::begin(expected.BIs), equal_enough))
        {
            return utils::concat( "BI missmatch. Got [", utils::concat_range(got.BIs, ", ")
                                , "] expected [", utils::concat_range(expected.BIs, ", "), "]." );
        }

        if (not std::equal(std::begin(got.CIs), std::end(got.CIs), std::begin(expected.CIs), equal_enough))
        {
            return utils::concat( "CI missmatch. Got [", utils::concat_range(got.CIs, ", ")
                                , "] expected [", utils::concat_range(expected.CIs, ", "), "]." );
        }

        if (not std::equal(std::begin(got.FIs), std::end(got.FIs), std::begin(expected.FIs), equal_enough))
        {
            return utils::concat( "FI missmatch. Got [", utils::concat_range(got.FIs, ", ")
                                , "] expected [", utils::concat_range(expected.FIs, ", "), "]." );
        }

        if (not equal_set(got.MCVs, expected.MCVs))
        {
            return std::string("MCVs missmatch.");
        }

        return std::string("OK");
    }

    template<std::size_t N>
    auto analyze_bss(bdd_man_t& m, bdd_t sf, double_v const& ps)
    {
        auto dpbds = m.dpbds(sf);
        auto bss   = bss_characteristic<N> {};
        bss.A      = m.availability(ps, sf);
        bss.U      = m.unavailability(ps, sf);
        bss.SIs    = m.structural_importances(dpbds);
        bss.BIs    = m.birnbaum_importances(ps, dpbds);
        bss.CIs    = m.criticality_importances(bss.BIs, ps, bss.U);
        bss.FIs    = m.fussell_vesely_importances(dpbds, ps, bss.U);
        bss.MCVs   = m.mcvs<std::bitset<5>>(dpbds);
        return bss;
    }

    auto test_bss()
    {
        auto m  = make_bdd_manager(5);
        auto& x = m;
        register_manager(m);

        auto const ps1 = std::vector {0.9, 0.8, 0.7, 0.9, 0.9};
        auto const sf1 = (x(0) and x(1)) or ((x(2) and x(3)) or x(4));
        auto const bs1 = bss_characteristic<5>
        {
            .A    = 0.989640,
            .U    = 0.010360,
            .SIs  = {0.187500, 0.187500, 0.187500, 0.187500, 0.562500},
            .BIs  = {0.029600, 0.033300, 0.025200, 0.019600, 0.103600},
            .CIs  = {0.285714, 0.642857, 0.729730, 0.189189, 1.000000},
            .FIs  = {0.357143, 0.714286, 0.810811, 0.270270, 1.000000},
            .MCVs = {std::bitset<5>(0b01010), std::bitset<5>(0b00110), std::bitset<5>(0b01001), std::bitset<5>(0b00101)}
        };
        auto const res1 = analyze_bss<5>(m, sf1, ps1);
        std::cout << "Test 1: " << compare_bss<5>(res1, bs1) << '\n';

        auto const ps2 = std::vector {0.9, 0.8, 0.7, 0.9, 0.9};
        auto const sf2 = x(0) and ((x(1) and x(3)) or (x(2) and x(4)));
        auto const bs2 = bss_characteristic<5>
        {
            .A    = 0.80676,
            .U    = 0.19324,
            .SIs  = {0.437500, 0.187500, 0.187500, 0.187500, 0.187500},
            .BIs  = {0.896400, 0.299700, 0.226800, 0.266400, 0.176400},
            .CIs  = {0.463879, 0.310184, 0.352101, 0.137860, 0.091285},
            .FIs  = {0.517491, 0.382943, 0.434693, 0.191472, 0.144898},
            .MCVs = {std::bitset<5>(0b11110), std::bitset<5>(0b11001), std::bitset<5>(0b01101), std::bitset<5>(0b10011), std::bitset<5>(0b00111)}
        };
        auto const res2 = analyze_bss<5>(m, sf2, ps2);
        std::cout << "Test 2: " << compare_bss<5>(res2, bs2) << '\n';
    }
}

#endif