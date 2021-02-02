#ifndef MIX_DD_TEST_TEST_RELIABILITY_HPP
#define MIX_DD_TEST_TEST_RELIABILITY_HPP

#include "test_mdd.hpp"
#include "../lib/bdd_manager.hpp"
#include <cmath>
#include <string>

namespace mix::dd::test
{
    using bdd_man_t  = bdd_manager<double, void>;
    using bdd_t      = mdd<double, void, 2>;
    using double_v   = std::vector<double>;
    using double_vv  = std::vector<double_v>;
    template<std::size_t P>
    using mdd_man_t  = mdd_manager<double, void, P>;
    template<std::size_t N>
    using double_a   = std::array<double, N>;
    template<std::size_t P>
    using prob_table = typename mdd_man_t<P>::prob_table;

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

    template<std::size_t N, std::size_t P>
    struct mss_characteristic
    {
        using bitset_v = std::vector<std::bitset<N>>;

        double_a<P> Ps;
        double_a<P> As;
        double_a<P> Us;
    };

    auto constexpr equal_enough(double const l, double const r)
    {
        return std::abs(l - r) < 0.00001;
    }

    template<class Range1, class Range2>
    auto equal_set(Range1&& r1, Range2&& r2)
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
        bss.MCVs   = m.mcvs<std::bitset<N>>(dpbds);
        return bss;
    }

    template<std::size_t N, std::size_t P>
    auto compare_mss( mss_characteristic<N, P> const& got
                    , mss_characteristic<N, P> const& expected )
    {
        if (not std::equal(std::begin(got.Ps), std::end(got.Ps), std::begin(expected.Ps), equal_enough))
        {
            return utils::concat( "Ps missmatch. Got [", utils::concat_range(got.Ps, ", ")
                                , "] expected [", utils::concat_range(expected.Ps, ", "), "]." );
        }

        if (not std::equal(std::begin(got.As), std::end(got.As), std::begin(expected.As), equal_enough))
        {
            return utils::concat( "As missmatch. Got [", utils::concat_range(got.As, ", ")
                                , "] expected [", utils::concat_range(expected.As, ", "), "]." );
        }

        if (not std::equal(std::begin(got.Us), std::end(got.Us), std::begin(expected.Us), equal_enough))
        {
            return utils::concat( "Us missmatch. Got [", utils::concat_range(got.Us, ", ")
                                , "] expected [", utils::concat_range(expected.Us, ", "), "]." );
        }

        return std::string("OK");
    }

    template<std::size_t N, std::size_t P>
    auto analyze_mss(mdd_man_t<P>& m, mdd_t<P> sf, prob_table<P> const& ps)
    {
        m.calculate_probabilities(ps, sf);
        return mss_characteristic<N, P>
        {
            .Ps = utils::fill_array<P>([&](auto const i)
            {
                return m.get_probability(i);
            }),
            .As = utils::fill_array<P>([&](auto const i)
            {
                return m.get_availability(i);
            }),
            .Us = utils::fill_array<P>([&](auto const i)
            {
                return m.get_unavailability(i);
            })
        };
    }

    auto test_bss(seed_t const seed = 0u)
    {
        auto const initSeed = 0u == seed ? std::random_device () () : seed;
        auto seeder         = int_rng<seed_t>(0u, UIntMax, initSeed);
        std::cout << "Test BSS. Init seed was " << initSeed << '.' << '\n';

        {
            auto  m       = make_bdd_manager(5);
            auto& x       = m;
            register_manager(m);
            auto rngOrder = std::mt19937(initSeed);
            m.set_order(get_order(order_e::Random, rngOrder, 5));
            auto const ps = std::vector {0.9, 0.8, 0.7, 0.9, 0.9};
            auto const sf = (x(0) and x(1)) or ((x(2) and x(3)) or x(4));
            auto const bs = bss_characteristic<5>
            {
                .A    = 0.989640,
                .U    = 0.010360,
                .SIs  = {0.187500, 0.187500, 0.187500, 0.187500, 0.562500},
                .BIs  = {0.029600, 0.033300, 0.025200, 0.019600, 0.103600},
                .CIs  = {0.285714, 0.642857, 0.729730, 0.189189, 1.000000},
                .FIs  = {0.357143, 0.714286, 0.810811, 0.270270, 1.000000},
                .MCVs = {std::bitset<5>(0b01010), std::bitset<5>(0b00110), std::bitset<5>(0b01001), std::bitset<5>(0b00101)}
            };
            auto const res = analyze_bss<5>(m, sf, ps);
            std::cout << "    Test 1: " << compare_bss<5>(res, bs) << '\n';
        }

        {
            auto  m       = make_bdd_manager(5);
            auto& x       = m;
            register_manager(m);
            auto rngOrder = std::mt19937(initSeed);
            m.set_order(get_order(order_e::Random, rngOrder, 5));
            auto const ps = std::vector {0.9, 0.8, 0.7, 0.9, 0.9};
            auto const sf = x(0) and ((x(1) and x(3)) or (x(2) and x(4)));
            auto const bs = bss_characteristic<5>
            {
                .A    = 0.80676,
                .U    = 0.19324,
                .SIs  = {0.437500, 0.187500, 0.187500, 0.187500, 0.187500},
                .BIs  = {0.896400, 0.299700, 0.226800, 0.266400, 0.176400},
                .CIs  = {0.463879, 0.310184, 0.352101, 0.137860, 0.091285},
                .FIs  = {0.517491, 0.382943, 0.434693, 0.191472, 0.144898},
                .MCVs = {std::bitset<5>(0b11110), std::bitset<5>(0b11001), std::bitset<5>(0b01101), std::bitset<5>(0b10011), std::bitset<5>(0b00111)}
            };
            auto const res = analyze_bss<5>(m, sf, ps);
            std::cout << "    Test 2: " << compare_bss<5>(res, bs) << '\n';
        }

        {
            auto  m       = make_bdd_manager(6);
            auto& x       = m;
            register_manager(m);
            auto rngOrder = std::mt19937(initSeed);
            m.set_order(get_order(order_e::Random, rngOrder, 6));
            auto const ps = std::vector {0.9, 0.8, 0.9, 0.7, 0.6, 0.9};
            auto const sf = x(0) and ((x(1) and x(2)) or x(3) or x(4)) and x(5);
            auto const bs = bss_characteristic<6>
            {
                .A    = 0.782784,
                .U    = 0.217216,
                .SIs  = {0.406250000, 0.031250000, 0.031250000, 0.093750000, 0.093750000, 0.406250000},
                .BIs  = {0.869760000, 0.087480000, 0.077760000, 0.090720000, 0.068040000, 0.869760000},
                .CIs  = {0.400412493, 0.080546553, 0.035798468, 0.125294638, 0.125294638, 0.400412493},
                .FIs  = {0.460371243, 0.110489098, 0.055244549, 0.154684738, 0.154684738, 0.460371243},
                .MCVs = {std::bitset<6>(0b111110), std::bitset<6>(0b100101), std::bitset<6>(0b100011), std::bitset<6>(0b011111)}
            };
            auto const res = analyze_bss<6>(m, sf, ps);
            std::cout << "    Test 2: " << compare_bss<6>(res, bs) << '\n';
        }
    }

    auto test_mss(std::size_t const)
    {
        {
            auto constexpr P = 3;
            auto constexpr N = 4;
            auto p = prob_table<P> { {0.1, 0.9, 0.0}
                                   , {0.2, 0.6, 0.2}
                                   , {0.3, 0.7, 0.0}
                                   , {0.1, 0.6, 0.3} };
            auto m       = make_mdd_manager<P>(N);
            m.set_domains({2, 3, 2, 3});
            auto const d = m.from_vector(std::vector<unsigned> {0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2});
            auto const g = analyze_mss<N, P>(m, d, p);
            auto const e = mss_characteristic<N, P>
            {
                .Ps = {0.0084, 0.2932, 0.6984},
                .As = {1.0000, 0.9916, 0.6984},
                .Us = {0.0000, 0.0084, 0.3016}
            };
            std::cout << compare_mss(g, e) << '\n';
        }
    }
}

#endif