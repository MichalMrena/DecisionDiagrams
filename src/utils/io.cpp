#include "io.hpp"

#include <iostream>

namespace mix::utils
{
    auto print (const std::string& s) -> void
    {
        std::cout << s;
    }

    auto printl (const std::string& s) -> void
    {
        std::cout << s << EOL;
    }
}