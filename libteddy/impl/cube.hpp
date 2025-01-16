#ifndef TEDDY_IMPL_CUBE_HPP
#define TEDDY_IMPL_CUBE_HPP

#include <libteddy/impl/types.hpp>

#include <cassert>
#include <cstdint>
#include <vector>

namespace teddy {

/**
 *  \brief Cube
 */
class cube {
public:
  static std::uint8_t constexpr DC = 0b11;

public:
  explicit cube(int32 size);

  [[nodiscard]] auto get_size () const -> int32;

  [[nodiscard]] auto get_value (int32 index) const -> int32;

  auto set_value (int32 index, int32 value) -> void;

private:
  struct pieced_byte {
    std::uint8_t b0 : 2;
    std::uint8_t b1 : 2;
    std::uint8_t b2 : 2;
    std::uint8_t b3 : 2;
  };

private:
  int32 size_;
  std::vector<pieced_byte> values_;
};

inline cube::cube(const int32 size) :
  size_(size),
  values_(as_usize(size / 4 + 1), pieced_byte {0, 0, 0, 0}) {
}

inline auto cube::get_size() const -> int32 {
  return size_;
}

inline auto cube::get_value(const int32 index) const -> int32 {
  int32 const byte_index = index / 4;
  assert(byte_index >= 0 && byte_index < ssize(values_));
  switch (index % 4) {
  case 0:
    return values_[as_uindex(byte_index)].b0;
  case 1:
    return values_[as_uindex(byte_index)].b1;
  case 2:
    return values_[as_uindex(byte_index)].b2;
  case 3:
    return values_[as_uindex(byte_index)].b3;
  default:
    return -1;
  }
}

inline auto cube::set_value(const int32 index, const int32 value) -> void {
  const int32 byte_index = index / 4;
  const auto u_value = static_cast<uint32>(value);
  assert((byte_index >= 0 && byte_index < ssize(values_)));
  assert(value == 0 || value == 1 || value == DC);
  switch (index % 4) {
  case 0:
    values_[as_uindex(byte_index)].b0 = u_value & 0b11U;
    break;
  case 1:
    values_[as_uindex(byte_index)].b1 = u_value & 0b11U;
    break;
  case 2:
    values_[as_uindex(byte_index)].b2 = u_value & 0b11U;
    break;
  case 3:
    values_[as_uindex(byte_index)].b3 = u_value & 0b11U;
    break;
  }
}

}  // namespace teddy

#endif