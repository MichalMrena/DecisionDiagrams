#ifndef LIBTEDDY_IMPL_CONTAINSERS_HPP
#define LIBTEDDY_IMPL_CONTAINSERS_HPP

#include <teddy/impl/config.hpp>
#include <teddy/impl/types.hpp>
#include <teddy/impl/tools.hpp>

namespace teddy::details
{

template<class T>
class array {
  static_assert(tools::is_scalar<T>, "Array supports only scalar types.");

public:
  explicit array(int32 size);
  array(const array &other);
  array(array &&other) noexcept;
  ~array();

  auto operator=(const array &other) -> array&;
  auto operator=(array &&other) noexcept -> array&;

  // TODO(michal): possibly inline definiton
  auto operator[](int32 i) -> T &;

  // TODO(michal): possibly inline definiton
  auto operator[](int32 i) const -> const T &;

  // TODO(michal): possibly inline definiton
  [[nodiscard]] auto size() const noexcept -> int32;

private:
  int32 length_;
  T *data_;
};


template<class T>
class array_list {

};

} // namespace teddy::details

// Include definitions in header-only mode
#ifndef TEDDY_NO_HEADER_ONLY
#include <teddy/impl/containers.cpp>
#endif

#endif
