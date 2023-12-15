#ifndef LIBTEDDY_TSL_TYPES_HPP
#define LIBTEDDY_TSL_TYPES_HPP

#include <cstdint>

namespace teddy::tsl
{
using int32                      = std::int32_t;
using int64                      = std::int64_t;
using uint32                     = std::uint32_t;
using uint64                     = std::uint64_t;

inline constexpr int32 Undefined = ~(1 << (8 * sizeof(int32) - 1));

// TODO add rng type

[[nodiscard]] inline auto constexpr as_uindex(int32 const index)
{
    return static_cast<std::size_t>(index);
}

[[nodiscard]] inline auto constexpr as_uindex(int64 const index)
{
    return static_cast<std::size_t>(index);
}

[[nodiscard]] inline auto constexpr as_usize(int32 const size)
{
    return static_cast<std::size_t>(size);
}

[[nodiscard]] inline auto constexpr as_usize(int64 const size)
{
    return static_cast<std::size_t>(size);
}
} // namespace teddy::tsl

#endif