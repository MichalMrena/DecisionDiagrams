#ifndef _MIX_UTILS_STRING_UTILS_
#define _MIX_UTILS_STRING_UTILS_

#include <string>
#include <vector>

namespace mix::utils
{
    constexpr auto EOL {"\n"};

    auto to_words (const std::string& str) -> std::vector<std::string>;
    
    auto to_bit_string (unsigned long long number) -> std::string;
}

#endif