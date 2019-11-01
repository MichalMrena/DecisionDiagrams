#ifndef MIX_DD_TYPEDEFS
#define MIX_DD_TYPEDEFS

#include <cstdint>
#include <limits>

namespace mix::dd
{
    using log_val_t  = std::int8_t;
    using input_t    = std::uint64_t;
    using id_t       = std::int32_t;

    constexpr log_val_t X {std::numeric_limits<log_val_t>::max()};
}

#endif