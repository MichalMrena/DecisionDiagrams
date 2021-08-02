#ifndef MIX_DD_TYPEDEFS_HPP
#define MIX_DD_TYPEDEFS_HPP

#include <limits>

namespace teddy
{
    using index_t = unsigned int;
    using level_t = unsigned int;
    using uint_t  = unsigned int;

    inline constexpr auto UintMax       = std::numeric_limits<uint_t>::max();
    inline constexpr auto Undefined     = UintMax;
    inline constexpr auto Nondetermined = UintMax - 1;
}

#endif