#ifndef _MIX_UTILS_STRING_UTILS_
#define _MIX_UTILS_STRING_UTILS_

#include <string>
#include <vector>
#include <utility>

namespace mix::utils
{
    auto to_words      (std::string s) -> std::vector<std::string>;
    auto to_head_tail  (std::string s) -> std::pair<std::string, std::string>;
    auto shrink_spaces (std::string s) -> std::string;
    auto trim          (std::string s) -> std::string;
    auto reverse       (std::string s) -> std::string;
}

#endif