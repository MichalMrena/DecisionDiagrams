#include "string_utils.hpp"

#include <sstream>
#include <bitset>
#include <algorithm> 
#include <cctype>
#include <locale>

namespace mix::utils
{
    auto to_words (const std::string& s) -> std::vector<std::string>
    {
        std::istringstream istr {s};
        std::vector<std::string> words;
        std::string word;
        
        while (istr >> word)
        {
            words.push_back(word);
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
                             , s.substr(firstSpace + 1));
    }

    auto shrink_spaces (std::string s) -> std::string
    {
        auto newEnd 
        {
            std::unique(s.begin(), s.end(), [](const char lhs, const char rhs) {
                return (lhs == rhs) && (lhs == ' ');
            })
        };

        s.erase(newEnd, s.end());
        return s;
    }

    auto trim (std::string s) -> std::string
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));

        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());

        return s;
    }

    auto to_bit_string (unsigned long long number) -> std::string
    {
        std::bitset<sizeof(unsigned long long)> bits {number};
        std::string bitsStr {bits.to_string()};
        const size_t firstOne {bitsStr.find('1')};

        return firstOne != std::string::npos 
                   ? bitsStr.substr(firstOne)
                   : "0";
    }
}