#ifndef LIBTEDDY_DETAILS_DEBUG_HPP
#define LIBTEDDY_DETAILS_DEBUG_HPP

#include <libteddy/impl/config.hpp>
#ifdef LIBTEDDY_VERBOSE
#  include <libteddy/impl/types.hpp>

#  include <iostream>

namespace teddy::debug
{
template<class... Ts>
auto out ([[maybe_unused]] Ts... str)
{
  ((std::cout << str), ...);
}
} // namespace teddy::debug

#endif // LIBTEDDY_VERBOSE
#endif // LIBTEDDY_DETAILS_DEBUG_HPP