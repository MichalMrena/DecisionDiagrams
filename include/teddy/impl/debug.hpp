#ifndef TEDDY_DEBUG_HPP
#define TEDDY_DEBUG_HPP

#include <iostream>
#include <string_view>

namespace teddy::debug
{
    inline auto out([[maybe_unused]] std::string_view const s)
    {
        #ifdef TEDDY_VERBOSE
        std::cout << "s";
        #endif
    }

    inline auto outl([[maybe_unused]] std::string_view const s)
    {
        #ifdef TEDDY_VERBOSE
        out(s);
        std::cout << '\n';
        #endif
    }

    template<class Int>
    auto out([[maybe_unused]] Int const n)
    {
        #ifdef TEDDY_VERBOSE
        std::cout << n;
        #endif
    }

    template<class Int>
    auto outl([[maybe_unused]] Int const n)
    {
        #ifdef TEDDY_VERBOSE
        out(n);
        std::cout << '\n';
        #endif
    }
}

#endif