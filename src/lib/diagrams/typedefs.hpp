#ifndef MIX_DD_TYPEDEFS_HPP
#define MIX_DD_TYPEDEFS_HPP

#include <cstdint>
#include <limits>
#include <string>

namespace mix::dd
{
    /* Types used by MDDs in general. */

    using id_t    = std::int32_t;
    using index_t = std::uint16_t;
    using level_t = std::uint16_t;

    /**
       Traits for logical types and constants.
     */
    template<std::size_t P>
    struct log_val_traits
    {
        using type = std::uint8_t;

        static_assert(P < std::numeric_limits<type>::max() - 1, "Max for P is 253.");

        inline static constexpr auto undefined     = type {P};      // * in extended dpbds.
        inline static constexpr auto nodomain      = type {P + 1};  // Non homogenous functions.
        inline static constexpr auto nondetermined = type {P + 2};  // Internal vertex in apply.
        inline static constexpr auto valuecount    = type {P + 2};

        static auto to_string (type const t) -> std::string;
    };

    /**
        Auxiliary struct used in description of dpbds.
     */
    template<std::size_t P>
    struct val_change
    {
        using log_t = typename log_val_traits<P>::type;
        log_t from;
        log_t to;
    };

    /* Types used only by BDDs. */

    using bool_vals_t = std::uint64_t;
    using bool_t = typename log_val_traits<2>::type;

    /**
        Description of a Boolean variable.
     */
    struct bool_var
    {
        index_t index;
        bool    complemented;
    };

    /* Definitions. */

    template<std::size_t P>
    constexpr auto is_undefined(typename log_val_traits<P>::type const v)
    {
        return log_val_traits<P>::undefined == v;
    }

    template<std::size_t P>
    constexpr auto is_nondetermined(typename log_val_traits<P>::type const v)
    {
        return log_val_traits<P>::nondetermined == v;
    }

    template<std::size_t P>
    constexpr auto is_nodomain(typename log_val_traits<P>::type const v)
    {
        return log_val_traits<P>::nodomain == v;
    }

    template<std::size_t P>
    auto log_val_traits<P>::to_string
        (type const t) -> std::string
    {
        auto constexpr U  = log_val_traits::undefined;
        auto constexpr ND = log_val_traits::nodomain;

        switch (t)
        {
            case U:  return "*";
            case ND: return "N";
            default: return std::to_string(t);
        }
    }
}

#endif