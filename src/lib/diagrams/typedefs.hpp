#ifndef MIX_DD_TYPEDEFS_HPP
#define MIX_DD_TYPEDEFS_HPP

#include <string>

namespace teddy
{
    /// Types used by MDDs in general.

    using index_t = unsigned int;
    using level_t = unsigned int;
    using int_t   = unsigned int;

    /**
     *  @brief Traits for logical types and constants.
     */
    template<std::size_t P>
    struct log_val_traits
    {
        using type = unsigned int;

        static_assert(P < 250, "P too big.");

        inline static constexpr auto undefined     = type {P};      // * in extended dpbds.
        inline static constexpr auto nondetermined = type {P + 1};  // Internal vertex in apply.
        inline static constexpr auto valuecount    = type {P + 2};

        static auto to_string (type t) -> std::string;
    };

    /**
     *  @brief Used in description of dpbds.
     */
    template<std::size_t P>
    struct val_change
    {
        using log_t = typename log_val_traits<P>::type;
        log_t from;
        log_t to;
    };

    /// Types used only by BDDs.

    using bool_vals_t = std::uint64_t;
    using bool_t      = log_val_traits<2>::type;

    /**
     *  @brief Description of a Boolean variable.
     */
    struct bool_var
    {
        bool    complemented;
        index_t index;
    };

    /// Definitions.

    template<std::size_t P>
    [[nodiscard]] constexpr auto is_undefined
        (typename log_val_traits<P>::type const v)
    {
        return log_val_traits<P>::undefined == v;
    }

    template<std::size_t P>
    [[nodiscard]] constexpr auto is_nondetermined
        (typename log_val_traits<P>::type const v)
    {
        return log_val_traits<P>::nondetermined == v;
    }

    template<std::size_t P>
    [[nodiscard]] auto log_val_traits<P>::to_string
        (type const t) -> std::string
    {
        auto constexpr U = log_val_traits::undefined;
        return t == U ? "*" : std::to_string(t);
    }
}

#endif