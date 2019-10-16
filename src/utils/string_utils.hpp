#ifndef MIX_UTILS_STRING_UTILS
#define MIX_UTILS_STRING_UTILS

#include <string>
#include <vector>

namespace mix::utils
{
    auto to_words(const std::string& str) -> std::vector<std::string>;
}

#endif