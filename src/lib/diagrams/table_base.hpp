#ifndef MIX_DD_TABLE_BASE_HPP
#define MIX_DD_TABLE_BASE_HPP

#include <array>
#include <utility>

namespace teddy
{
    class table_base
    {
    protected:
        using capacity_it = std::array<std::size_t, 24>::const_iterator;

    protected:
        static inline auto constexpr Capacities = std::array<std::size_t, 24>
        {
            307u,         617u,         1'237u,         2'477u,         4'957u,
            9'923u,       19'853u,      39'709u,        79'423u,        158'849u,
            317'701u,     635'413u,     1'270'849u,     2'541'701u,     5'083'423u,
            10'166'857u,  20'333'759u,  40'667'527u,    81'335'063u,    162'670'129u,
            325'340'273u, 650'680'571u, 1'301'361'143u, 2'602'722'289u
        };

    protected:
        table_base () = default;
        table_base (table_base&& other) noexcept :
            size_     (std::exchange(other.size_, 0)),
            capacity_ (std::exchange(other.capacity_, std::begin(Capacities)))
        {
        }

    protected:
        std::size_t size_     {0};
        capacity_it capacity_ {std::cbegin(Capacities)};
    };
}

#endif