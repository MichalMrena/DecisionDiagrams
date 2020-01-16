#ifndef _MIX_UTILS_IO_
#define _MIX_UTILS_IO_

#include <string>

namespace mix::utils
{
    constexpr auto EOL {"\n"};

    auto print  (const std::string& s) -> void;
    auto printl (const std::string& s) -> void;
}

#endif