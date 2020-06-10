#include "string_utils.hpp"

#include <sstream>
#include <bitset>
#include <algorithm> 
#include <cctype>
#include <locale>

namespace mix::utils
{
    auto to_words (std::string s) -> std::vector<std::string>
    {
        std::vector<std::string> words;
        const auto delims = {' '};
        const auto end {std::cend(s)};
        auto first     {std::cbegin(s)};

        while (first != end)
        {
            const auto last 
            {
                std::find_first_of(first, end, std::cbegin(delims), std::cend(delims))
            };

            if (first != last)
            {
                words.emplace_back(first, last);
            }

            if (last == end)
            {
                break;
            }

            first = std::next(last);
        }

        return words;
    }

    auto to_head_tail (std::string s) -> std::pair<std::string, std::string>
    {
        const auto firstSpace {s.find(' ')};

        if (std::string::npos == firstSpace)
        {
            return std::make_pair(s, "");
        }

        return std::make_pair( s.substr(0, firstSpace)
                             , s.substr(firstSpace + 1) );
    }

    auto shrink_spaces (std::string s) -> std::string
    {
        auto newEnd {std::unique(s.begin(), s.end(), [](const char lhs, const char rhs) 
        {
            return (lhs == rhs) && (lhs == ' ');
        })};

        s.erase(newEnd, s.end());
        return s;
    }

    auto trim (std::string s) -> std::string
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) 
        {
            return ! std::isspace(ch);
        }));

        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) 
        {
            return ! std::isspace(ch);
        }).base(), s.end());

        return s;
    }

    auto reverse (std::string s) -> std::string
    {
        std::reverse(s.begin(), s.end());
        return s;
    }

    auto starts_with ( std::string_view s
                     , std::string_view pattern ) -> bool
    {
        if (s.size() < pattern.size())
        {
            return false;
        }

        auto stringIt  {s.begin()};
        auto stringEnd {s.end()};

        while (std::isspace(*stringIt))
        {
            ++stringIt;
        }

        auto patternIt  {pattern.begin()};
        auto patternEnd {pattern.end()};

        while (stringIt != stringEnd && patternIt != patternEnd)
        {
            if (*stringIt != *patternIt)
            {
                return false;
            }

            ++stringIt;
            ++patternIt;
        }

        return true;
    }
}