#ifndef LIBTEDDY_TSL_TYPES_HPP
#define LIBTEDDY_TSL_TYPES_HPP

#include <cstdint>

namespace teddy::tsl
{
using int32  = std::int32_t;
using int64  = std::int64_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

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
} // namespace teddy::tsl

#endif