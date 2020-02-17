#ifndef _MIX_UTILS_IO_
#define _MIX_UTILS_IO_

#include <iostream>
#include <string>

namespace mix::utils
{
    constexpr auto EOL {"\n"};

    auto print  (const std::string& s) -> void;
    auto printl (const std::string& s) -> void;
    auto print  (const char* s)        -> void;
    auto printl (const char* s)        -> void;

    template<class T>
    auto print (const T& t) -> void
    {
        using std::to_string;
        print(to_string(t));
    }

    template<class T>
    auto printl (const T& t) -> void
    {
        using std::to_string;
        printl(to_string(t));
    }
}

#endif