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
auto out ([[maybe_unused]] Ts... s)
{
#ifdef LIBTEDDY_VERBOSE
    ((std::cout << s), ...);
#endif
}

inline auto assert_true ([[maybe_unused]] bool const b)
{
    assert(b);
}

inline auto assert_in_range (
    [[maybe_unused]] int64 const i, [[maybe_unused]] int64 const size
)
{
    assert((i >= 0 && i < size) && "Invalid index!");
}
} // namespace teddy::debug

#endif