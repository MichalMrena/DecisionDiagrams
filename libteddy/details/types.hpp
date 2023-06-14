#ifndef LIBTEDDY_DETAILS_TYPES_HPP
#define LIBTEDDY_DETAILS_TYPES_HPP

#include <cstdint>
#include <limits>

namespace teddy
{
using int32                         = std::int32_t;
using int64                         = std::int64_t;
using uint32                        = std::uint32_t;
using uint64                        = std::uint64_t;

inline constexpr auto Undefined     = (std::numeric_limits<int32>::max)();
inline constexpr auto Nondetermined = (std::numeric_limits<int32>::max)() - 1;

// TODO move to tools
[[nodiscard]] inline auto constexpr as_uindex(int32 const i)
{
    return static_cast<std::size_t>(i);
}

[[nodiscard]] inline auto constexpr as_uindex(int64 const i)
{
    return static_cast<std::size_t>(i);
}

[[nodiscard]] inline auto constexpr as_usize(int32 const s)
{
    return static_cast<std::size_t>(s);
}

[[nodiscard]] inline auto constexpr as_usize(int64 const s)
{
    return static_cast<std::size_t>(s);
}

[[nodiscard]] inline auto constexpr special_to_index(int32 const val) -> int32
{
    return (std::numeric_limits<int32>::max)() - val;
}

[[nodiscard]] inline auto constexpr is_special(int32 const val) -> bool
{
    return val == Undefined;
}
} // namespace teddy

#endif