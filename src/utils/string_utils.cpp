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
}