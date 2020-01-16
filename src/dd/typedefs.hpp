#ifndef _MIX_DD_TYPEDEFS_
#define _MIX_DD_TYPEDEFS_

#include <cstdint>
#include <limits>
#include <string>

namespace mix::dd
{
    using bool_t     = int8_t;
    using var_vals_t = uint64_t;
    using id_t       = int32_t;
    using index_t    = uint32_t;

    /** 
       Represents undefined bool value. 
     */
    constexpr bool_t X {3};

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