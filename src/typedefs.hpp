#ifndef _MIX_DD_TYPEDEFS_
#define _MIX_DD_TYPEDEFS_

#include <cstdint>
#include <limits>
#include <string>

namespace mix::dd
{
    using log_val_t = int8_t;
    using input_t   = uint64_t; // TODO nope
    using id_t      = int32_t;
    using index_t   = uint32_t;

    /* undefined logical value */
    constexpr log_val_t X {std::numeric_limits<log_val_t>::max()};

    inline auto log_val_to_string (log_val_t val) -> std::string
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