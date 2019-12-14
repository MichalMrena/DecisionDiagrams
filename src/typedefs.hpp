#ifndef _MIX_DD_TYPEDEFS_
#define _MIX_DD_TYPEDEFS_

#include <cstdint>
#include <limits>

namespace mix::dd
{
    using log_val_t         = int8_t;
    using input_t           = uint64_t;
    using id_t              = int32_t;
    using def_vertex_data_t = int8_t;
    using def_arc_data_t    = int8_t;

    /* undefined logical value */
    constexpr log_val_t X {std::numeric_limits<log_val_t>::max()};

    inline auto log_val_to_char (log_val_t val) -> char
    {
        switch (val)
        {
        case 0:  return '0';
        case 1:  return '1';
        case X:  return 'X';
        default: return '-';
        }
    }
}

#endif