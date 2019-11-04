#ifndef MIX_UTILS_STRING_UTILS
#define MIX_UTILS_STRING_UTILS

#include <string>
#include <vector>
#include <bitset>

namespace mix::utils
{
    auto to_words (const std::string& str) -> std::vector<std::string>;
    
    auto to_bit_string (unsigned long long number) -> std::string;
}

#endif