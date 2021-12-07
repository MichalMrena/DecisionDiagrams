auto PrintLoadFactor = true;

#include "teddy/teddy_reliability.hpp"
#include <charconv>
#include <cassert>
#include <iostream>
#include <numeric>
#include <optional>
#include <random>
#include <ranges>
#include <type_traits>
#include <vector>

namespace rs = std::ranges;

using uint_t = unsigned int;

enum class system_type_e
{
    Serial,
    Parallel,
    SerialParallel,
    Unknown
};

enum class structure_func_e
{
    One,
    Multiple,
    Unknown
};

namespace teddy
{
    template<uint_t P>
    auto create_serial (mss_manager<P>& manager)
    {
        auto const n   = manager.get_var_count();
        auto variables = manager.variables(rs::iota_view(0u, n));
        return manager.template tree_fold<MIN>(variables);
    }

    template<uint_t P>
    auto create_parallel (mss_manager<P>& manager)
    {
        auto const n   = manager.get_var_count();
        auto variables = manager.variables(rs::iota_view(0u, n));
        return manager.template tree_fold<MAX>(variables);
    }

    template<uint_t P>
    auto create_serialparallel ( mss_manager<P>&  manager
                               , std::mt19937_64& rngtype
                               , std::mt19937_64& rngbranch )
    {
        auto go = [&, i = 0u](auto& self, auto const n) mutable
        {
            if (n == 1)
            {
                return manager.variable(i++);
            }
            else
            {
                auto denomdist = std::uniform_int_distribution<uint_t>(2, 10);
                auto typedist  = std::uniform_real_distribution(0.0, 1.1);
                // TODO cache robí problémy pri asymetrických veľkostiach
                auto const denom   = denomdist(rngbranch);
                // auto const denom   = 2u;
                auto const lhssize = std::max(1u, n / denom);
                auto const rhssize = n - lhssize;
                auto const lhs = self(self, lhssize);
                auto const rhs = self(self, rhssize);
                auto const p   = typedist(rngtype);
                return p < 0.5 ? manager.template apply<MIN>(lhs, rhs)
                               : manager.template apply<MAX>(lhs, rhs);
            }
        };
        return go(go, static_cast<uint_t>(manager.get_var_count()));
    }

    template<uint_t P>
    auto transform_sf ( mss_manager<P>& manager
                      , auto const&     sf )
    {
        using diagram_t = typename mss_manager<P>::diagram_t;
        auto sfs = std::vector<diagram_t>();
        for (auto k = 1u; k < P; ++k)
        {
            sfs.emplace_back(manager.booleanize(sf, [k](auto const v)
            {
                return v >= k;
            }));
            // PrintLoadFactor = true;
            // manager.nodes_.clear_caches();
            // sfs.emplace_back(manager.template apply<GREATER_EQUAL>(sf, manager.constant(k)));
        }
        return sfs;
    }

    template<uint_t P>
    auto create_structure_function ( mss_manager<P>&        manager
                                   , std::mt19937_64&       rngtype
                                   , std::mt19937_64&       rngbranch
                                   , system_type_e const    systemtype
                                   , structure_func_e const sftype )
    {
        using diagram_t = typename mss_manager<P>::diagram_t;

        auto const sf = [&, systemtype]()
        {
            switch (systemtype)
            {
                case system_type_e::Serial:
                    return create_serial(manager);

                case system_type_e::Parallel:
                    return create_parallel(manager);

                case system_type_e::SerialParallel:
                    return create_serialparallel(manager, rngtype, rngbranch);

                case system_type_e::Unknown:
                    return diagram_t();
            }
            return diagram_t();
        }();

        switch (sftype)
        {
            case structure_func_e::One:
                return std::vector<diagram_t> {sf};

            case structure_func_e::Multiple:
                return transform_sf(manager, sf);

            case structure_func_e::Unknown:
                return std::vector<diagram_t> {};
        }

        return std::vector<diagram_t> {};
    }

    template<uint_t P>
    auto generate_probabilities (uint_t const n, std::mt19937_64& rngps)
    {
        using probs = typename mss_manager<P>::probabilities_t;
        auto psdist = std::uniform_real_distribution(.0, 1.0);
        auto ps     = probs();
        for (auto i = 0u; i < n; ++i)
        {
            auto ips = std::array<double, P> {};
            for (auto j = 0u; j < P; ++j)
            {
                ips[j] = psdist(rngps);
            }
            auto const norm = std::reduce(begin(ips), end(ips));
            for (auto j = 0u; j < P; ++j)
            {
                ips[j] = ips[j] / norm;
            }
            ps.emplace_back(ips);
        }
        return ps;
    }

    template<uint_t P>
    auto calculate_availabilities ( mss_manager<P>&        manager
                                  , structure_func_e const sftype
                                  , auto const&            ps
                                  , auto&                  sfs )
    {
        auto As = std::array<double, P - 1> {};
        if (sftype == structure_func_e::One)
        {
            manager.calculate_probabilities(ps, sfs.front());
            for (auto j = 1u; j < P; ++j)
            {
                As[j - 1] = manager.get_availability(j);
            }
        }
        else if (sftype == structure_func_e::Multiple)
        {
            for (auto j = 1u; j < P; ++j)
            {
                As[j - 1] = manager.availability(1, ps, sfs[j - 1]);
            }
        }
        return As;
    }

    template<uint_t P>
    auto do_experiment ( uint_t const           seed
                       , uint_t const           n
                       , system_type_e const    systemtype
                       , structure_func_e const sftype )
    {
        auto seeder    = std::mt19937_64(seed);
        auto rngtype   = std::mt19937_64(seeder());
        auto rngbranch = std::mt19937_64(seeder());
        auto rngps     = std::mt19937_64(seeder());
        auto manager   = mss_manager<P>(n, 10'000);
        auto sfs       = create_structure_function( manager, rngtype, rngbranch
                                                  , systemtype, sftype );
        manager.gc();
        std::cout << "Node count: " << manager.node_count() << '\n';

        auto const ps = generate_probabilities<P>(n, rngps);
        auto const As = calculate_availabilities(manager, sftype, ps, sfs);

        for (auto j = 1u; j < P; ++j)
        {
            std::cout << "A" << j << " = " << As[j - 1] << '\n';
        }
    }
}

auto print_help () -> void;
auto print_params
    (uint_t, uint_t, system_type_e, structure_func_e, uint_t) -> void;
auto string_to_uint (std::string_view) -> std::optional<uint_t>;
auto sf_type_to_string (structure_func_e) -> std::string_view;
auto string_to_sf_type (std::string_view) -> structure_func_e;
auto system_type_to_string (system_type_e) -> std::string_view;
auto string_to_system_type (std::string_view) -> system_type_e;

/**
 *  main
 */
auto main (int argc, char** argv) -> int
{
    // TODO počítanie derivácií s použitím projekcií pre koncové vrcholy
    // a pre projekcie na get_sony

    if (argc < 6)
    {
        print_help();
        return -1;
    }

    auto const seed       = string_to_uint(argv[1]);
    auto const P          = string_to_uint(argv[2]);
    auto const systemtype = string_to_system_type(argv[3]);
    auto const sftype     = string_to_sf_type(argv[4]);
    auto const n          = string_to_uint(argv[5]);

    if ( not P or not n or systemtype == system_type_e::Unknown
                        or sftype == structure_func_e::Unknown )
    {
        print_help();
        return -1;
    }

    print_params(*seed, *P, systemtype, sftype, *n);

    switch (*P)
    {
        case 2:
            teddy::do_experiment<2>(*seed, *n, systemtype, sftype);
            break;

        case 3:
            teddy::do_experiment<3>(*seed, *n, systemtype, sftype);
            break;

        case 4:
            teddy::do_experiment<4>(*seed, *n, systemtype, sftype);
            break;

        case 5:
            teddy::do_experiment<5>(*seed, *n, systemtype, sftype);
            break;

        default:
            print_help();
            break;
    }

    std::cout << "done" << '\n';
}

auto print_help () -> void
{
    std::cout << "Usage:"                                              << '\n'
              << "./experiment"                                        << '\n'
              << " <seed ∈ N>"                                         << '\n'
              << " <P ∈ {2, 3, 4, 5}>"                                 << '\n'
              << " <system_type ∈ {serial, parallel, serialparallel}>" << '\n'
              << " <structure_function ∈ {one, multiple}>"             << '\n'
              << " <n ∈ N>"                                            << '\n'
              << '\n';
}

auto print_params ( uint_t const           seed
                  , uint_t const           P
                  , system_type_e const    systemtype
                  , structure_func_e const sftype
                  , uint_t const           n ) -> void
{
    std::cout << "seed=" << seed
              << ";P=" << P
              << ";system_type=" << system_type_to_string(systemtype)
              << ";structure_function=" << sf_type_to_string(sftype)
              << ";n=" << n
              << '\n';
}

auto string_to_system_type (std::string_view const str) -> system_type_e
{
    if (str == "serial")
    {
        return system_type_e::Serial;
    }
    else if (str == "parallel")
    {
        return system_type_e::Parallel;
    }
    else if (str == "serialparallel")
    {
        return system_type_e::SerialParallel;
    }
    else
    {
        return system_type_e::Unknown;
    }
}

auto system_type_to_string (system_type_e const t) -> std::string_view
{
    switch (t)
    {
        case system_type_e::Serial:
            return "serial";

        case system_type_e::Parallel:
            return "parallel";

        case system_type_e::SerialParallel:
            return "serialparallel";

        case system_type_e::Unknown:
            return "unknown";
    }
    return "notgood";
}

auto string_to_sf_type (std::string_view const str) -> structure_func_e
{
    if (str == "one")
    {
        return structure_func_e::One;
    }
    else if (str == "multiple")
    {
        return structure_func_e::Multiple;
    }
    else
    {
        return structure_func_e::Unknown;
    }
}

auto sf_type_to_string (structure_func_e const t) -> std::string_view
{
    switch (t)
    {
        case structure_func_e::One:
            return "one";

        case structure_func_e::Multiple:
            return "multiple";

        case structure_func_e::Unknown:
            return "unknown";
    }
    return "notgood";
}

auto string_to_uint (std::string_view const str) -> std::optional<uint_t>
{
    auto num = uint_t {};
    auto const status = std::from_chars(std::begin(str), std::end(str), num);
    return status.ec == std::errc {} ? std::optional(num) : std::nullopt;
}
