#ifndef LIBTEDDY_TSL_TYPES_HPP
#define LIBTEDDY_TSL_TYPES_HPP

#include <cstdint>
#include <random>

namespace teddy::tsl {
using int32                      = std::int32_t;
using int64                      = std::int64_t;
using uint32                     = std::uint32_t;
using uint64                     = std::uint64_t;
using rng_t                      = std::ranlux48;

inline int32 constexpr Undefined = ~(1U << (8 * sizeof(int32) - 1U));

[[nodiscard]]
inline auto constexpr as_uindex(int32 const index) {
  return static_cast<std::size_t>(index);
}

[[nodiscard]]
inline auto constexpr as_uindex(int64 const index) {
  return static_cast<std::size_t>(index);
}

[[nodiscard]]
inline auto constexpr as_usize(int32 const size) {
  return static_cast<std::size_t>(size);
}

[[nodiscard]]
inline auto constexpr as_usize(int64 const size) {
  return static_cast<std::size_t>(size);
}
} // namespace teddy::tsl

#endif