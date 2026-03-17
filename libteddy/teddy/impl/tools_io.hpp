#ifndef LIBTEDDY_IMPL_TOOLS_IO_HPP
#define LIBTEDDY_IMPL_TOOLS_IO_HPP

#include <charconv>
#include <optional>
#include <string_view>

/**
 * \file tools_ip.hpp
 * \brief Contains (simplified) implementations of (STL) algorithms
 *
 * Similar to tools but brings the <charconv> dependency.
 */

namespace teddy::tools {
/**
  *  \brief Tries to parse \p input to \p Num
  *  \param input input string
  *  \return optinal result
  */
template<class Num>
auto parse (std::string_view const input) -> std::optional<Num> {
  Num ret;
  char const *const first             = input.data();
  char const *const last              = input.data() + input.size();
  std::from_chars_result const result = std::from_chars(first, last, ret);
  return std::errc {} == result.ec && result.ptr == last
          ? std::optional<Num>(ret)
          : std::nullopt;
}
}  // namespace teddy::tools

#endif
