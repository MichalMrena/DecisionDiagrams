#ifndef MIX_UTILS_IO_
#define MIX_UTILS_IO_

#include <iostream>
#include <string_view>
#include <type_traits>

namespace teddy::utils
{
    inline auto constexpr EOL = "\n";

    inline auto print (std::string_view s)
    {
        std::cout << s;
    }

    inline auto printl (std::string_view s)
    {
        std::cout << s << EOL;
    }

    template<class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
    auto print (T const& t) -> void
    {
        print(std::to_string(t));
    }

    template<class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
    auto printl (T const& t) -> void
    {
        printl(std::to_string(t));
    }
}

#endif