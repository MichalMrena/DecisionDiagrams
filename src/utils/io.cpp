#include "io.hpp"

#include <iostream>

namespace mix::utils
{
    // auto print (const std::string& s) -> void
    // {
    //     std::cout << s;
    // }

    // auto printl (const std::string& s) -> void
    // {
    //     std::cout << s << EOL;
    // }

    // auto print (const char* s) -> void
    // {
    //     std::cout << s;
    // }

    // auto printl (const char* s) -> void
    // {
    //     std::cout << s << EOL;
    // }

    auto print (std::string_view s) -> void
    {
        std::cout << s;
    }

    auto printl (std::string_view s) -> void
    {
        std::cout << s << EOL;
    }
}