#ifndef _MIX_DD_TYPEDEFS_
#define _MIX_DD_TYPEDEFS_

#include <cstdint>
#include <cmath>
#include <limits>
#include <string>

namespace mix::dd
{
    using bool_t     = int8_t;
    using var_vals_t = uint64_t;
    using id_t       = int32_t;
    using index_t    = uint32_t;

    // using log_t = int8_t;

    template<size_t P>
    struct log_val_traits
    {
        using value_t = uint8_t;

        static constexpr value_t X {P};
    };

    /** 
       Represents undefined bool value. 
     */
    constexpr bool_t X {3};

    constexpr auto bool_to_bool_t (const bool b) -> bool_t
    {
        return b ? 1 : 0;
    }

    inline auto to_string (const bool_t val) -> std::string
    {
        switch (val)
        {
            case 0:  return "0";
            case 1:  return "1";
            case X:  return "X";
            default: return "-";
        }
    }
}

#endif