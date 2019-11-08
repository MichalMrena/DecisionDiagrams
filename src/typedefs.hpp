#ifndef MIX_DD_TYPEDEFS
#define MIX_DD_TYPEDEFS

#include <cstdint>
#include <limits>

namespace mix::dd
{
    using log_val_t         = int8_t;
    using input_t           = uint64_t;
    using id_t              = int32_t;
    using def_vertex_data_t = int8_t;
    using def_arc_data_t    = int8_t;

    constexpr log_val_t X {std::numeric_limits<log_val_t>::max()};
}

#endif