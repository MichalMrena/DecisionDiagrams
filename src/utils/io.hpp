#ifndef _MIX_UTILS_IO_
#define _MIX_UTILS_IO_

#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

namespace mix::utils
{
    constexpr auto EOL {"\n"};

    auto print  (std::string_view s) -> void;
    auto printl (std::string_view s) -> void;

    template<class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
    auto print (const T& t) -> void
    {
        print(std::to_string(t));
    }

    template<class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
    auto printl (const T& t) -> void
    {
        printl(std::to_string(t));
    }
}

#endif