#ifndef LIBTEDDY_DETAILS_DEBUG_HPP
#define LIBTEDDY_DETAILS_DEBUG_HPP

#include <iostream>
#include <string_view>

namespace teddy::debug
{
template<class... Ts>
auto out([[maybe_unused]] Ts... s)
{
#ifdef LIBTEDDY_VERBOSE
    ((std::cout << s), ...);
#endif
}
} // namespace teddy::debug

#endif