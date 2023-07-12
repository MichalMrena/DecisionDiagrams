#ifndef LIBTEDDY_DETAILS_DEBUG_HPP
#define LIBTEDDY_DETAILS_DEBUG_HPP

#ifdef LIBTEDDY_VERBOSE
#    include <iostream>
#endif

#include <libteddy/details/types.hpp>
#include <cassert>

namespace teddy::debug
{
template<class... Ts>
auto out ([[maybe_unused]] Ts... str)
{
#ifdef LIBTEDDY_VERBOSE
    ((std::cout << str), ...);
#endif
}
} // namespace teddy::debug

#endif