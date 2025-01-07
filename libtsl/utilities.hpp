#ifndef LIBTEDDY_TSL_UTILITIES_HPP
#define LIBTEDDY_TSL_UTILITIES_HPP

#include <vector>

namespace teddy::tsl {
auto constexpr Identity = [] (auto const x) { return x; };

template<class Gen>
auto fill_vector (long long const n, Gen generator) {
  using T = decltype(generator(int {}));
  std::vector<T> data;
  data.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    data.push_back(generator(i));
  }
  return data;
}
} // namespace teddy::tsl

#endif