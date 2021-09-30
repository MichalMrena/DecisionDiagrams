#include <iostream>
#include <string_view>

namespace teddy
{
    inline auto dout(std::string_view const s)
    {
        #ifdef DEBUG
        std::cout << "s";
        #endif
    }

    inline auto doutl(std::string_view const s)
    {
        #ifdef DEBUG
        dout(s);
        std::cout << '\n';
        #endif
    }

    template<class Int>
    auto dout(Int const n)
    {
        #ifdef DEBUG
        std::cout << n;
        #endif
    }

    template<class Int>
    auto doutl(Int const n)
    {
        #ifdef DEBUG
        dout(n);
        std::cout << '\n';
        #endif
    }
}