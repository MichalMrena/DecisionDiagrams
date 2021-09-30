#include <iostream>
#include <string_view>

namespace teddy
{
    inline auto dout([[maybe_unused]] std::string_view const s)
    {
        #ifdef DEBUG
        std::cout << "s";
        #endif
    }

    inline auto doutl([[maybe_unused]] std::string_view const s)
    {
        #ifdef DEBUG
        dout(s);
        std::cout << '\n';
        #endif
    }

    template<class Int>
    auto dout([[maybe_unused]] Int const n)
    {
        #ifdef DEBUG
        std::cout << n;
        #endif
    }

    template<class Int>
    auto doutl([[maybe_unused]] Int const n)
    {
        #ifdef DEBUG
        dout(n);
        std::cout << '\n';
        #endif
    }
}