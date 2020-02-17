#ifndef _MIX_UTILS_STRING_UTILS_
#define _MIX_UTILS_STRING_UTILS_

#include <string>
#include <vector>
#include <utility>
#include <sstream>

namespace mix::utils
{
    auto to_words      (std::string s) -> std::vector<std::string>;
    auto to_head_tail  (std::string s) -> std::pair<std::string, std::string>;
    auto shrink_spaces (std::string s) -> std::string;
    auto trim          (std::string s) -> std::string;
    auto reverse       (std::string s) -> std::string;

    auto starts_with   ( const std::string& s
                       , const std::string& pattern ) -> bool;

    auto concat ( const std::vector<std::string>& strs
                , const std::string glue = " " ) -> std::string;

    template<class InputIt>
    auto concat (InputIt it, InputIt end, const std::string glue = " ") -> std::string
    {
        std::ostringstream ost;

        ost << *it;
        ++it;

        while (it != end)
        {
            ost << glue << *it;
            ++it;
        }

        return ost.str();
    }
}

#endif