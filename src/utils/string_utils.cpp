#include "string_utils.hpp"

#include <sstream>

namespace mix::utils
{
    auto to_words(const std::string& str) -> std::vector<std::string>
    {
        std::istringstream istr {str};
        std::vector<std::string> words;
        std::string word;
        
        while (istr >> word)
        {
            words.push_back(word);
        }

        return words;
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