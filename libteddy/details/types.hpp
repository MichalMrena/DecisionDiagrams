#ifndef LIBTEDDY_DETAILS_TYPES_HPP
#define LIBTEDDY_DETAILS_TYPES_HPP

#include <limits>

namespace teddy
{
    using index_t = unsigned int;
    using level_t = unsigned int;
    using uint_t  = unsigned int;

    inline constexpr auto UintMax       = (std::numeric_limits<uint_t>::max)();
    inline constexpr auto Undefined     = UintMax;
    inline constexpr auto Nondetermined = UintMax - 1;

    auto constexpr special_val_to_index (uint_t const val) -> index_t
    {
        return static_cast<index_t>(UintMax - val);
    }

    auto constexpr is_special (uint_t const val) -> bool
    {
        return val == Undefined;
    }
}

#endif