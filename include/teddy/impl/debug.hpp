#ifndef TEDDY_DEBUG_HPP
#define TEDDY_DEBUG_HPP

#include <iostream>
#include <string_view>

namespace teddy::debug
{
    template<class... Ts>
    auto out([[maybe_unused]] Ts... s)
    {
        #ifdef TEDDY_VERBOSE
        ((std::cout << s), ...);
        #endif
    }
}

#endif