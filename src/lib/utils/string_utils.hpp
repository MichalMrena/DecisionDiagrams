#ifndef MIX_UTILS_STRING_UTILS_HPP
#define MIX_UTILS_STRING_UTILS_HPP

#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <sstream>
#include <iterator>
#include <cstring>
#include <algorithm> 
#include <locale>

namespace mix::utils
{
    /**
        Splits the string into words using space as a delimiter.
        @return vector of new string that are words from @p s .
     */
    auto to_words (std::string s) -> std::vector<std::string>;

    /**
        Splits the string into two parts. The first one contains the first word (head)
        and the second one constains the rest of the string (tail).
        @return pair of new strings that are the head and the tail of @p s .
     */
    auto to_head_tail (std::string s) -> std::pair<std::string, std::string>;

    /**
        Replaces consecutive spaces with a single one.
        By space we mean ' '.
        @return new string with no consecutive spaces.
     */
    auto shrink_spaces (std::string s) -> std::string;

    /**
        Removes spaces from the begining and from the end of the string.
        @return new string with no leading and trailing spaces.
     */
    auto trim (std::string s) -> std::string;

    /**
        @return new string that is reversed version of @p s .
     */
    auto reverse (std::string s) -> std::string;


    /**
        Ignores leading spaces. C++20 introduces member function for std::string
        and std::string_view that does almost the same.
        @return true if @p s starts with @p pattern 
                false otherwise
     */
    auto starts_with ( std::string_view s
                     , std::string_view pattern ) -> bool;

    /**
        Concatenates a string representation of each element of the range
        using provided @p glue .
        Elements of the range must be printable to std::ostream.
        @return new string representing the range.
     */
    template<class InputIt>
    auto concat_range ( InputIt first
                      , InputIt last
                      , std::string_view glue ) -> std::string;

    /**
        Concatenates a string representation of each element of the range
        using provided @p glue .
        Elements of the range must be printable to std::ostream.
        @return new string representing the range.
     */
    template<class Range>
    auto concat_range ( Range const& strs
                      , std::string_view glue ) -> std::string;

// definitions:

    /**
        https://stackoverflow.com/a/18899027/8616625
    */
    namespace aux_impl 
    {
        template<class N>
        struct string_traits
        {
            static constexpr auto size (N n) { return std::to_string(n).size(); }
            static auto to_str (N n) { using std::to_string; return to_string(n); }
        };

        template<size_t N>
        struct string_traits<const char[N]> 
        {
            static constexpr auto size (const char (&) [N]) { return N - 1; }
            static auto to_str (const char (&s) [N]) { return s; }
        };

        template<size_t N>
        struct string_traits<char[N]> 
        {
            static auto size   (char (&s) [N]) { return N ? std::strlen(s) : 0; }
            static auto to_str (char (&s) [N]) { return s; }
        };

        template<>
        struct string_traits<char const*> 
        {
            static auto size   (char const* s) { return s ? std::strlen(s) : 0; }
            static auto to_str (char const* s) { return s; }
        };

        template<>
        struct string_traits<char*> 
        {
            static auto size   (char* s) { return s ? std::strlen(s) : 0; }
            static auto to_str (char* s) { return s; }
        };

        template<>
        struct string_traits<std::string> 
        {
            static auto size   (std::string const& s) { return s.size(); }
            static auto to_str (std::string const& s) { return s; }
        };

        template<>
        struct string_traits<std::string_view> 
        {
            static auto size   (std::string_view s) { return s.size(); }
            static auto to_str (std::string_view s) { return s; }
        };

        template<class String> 
        auto string_size (String&& s)
        {
            using noref_t  = std::remove_reference_t<String>;
            using string_t = std::conditional_t< std::is_array_v<noref_t>, noref_t, std::remove_cv_t<noref_t> >;
            return string_traits<string_t>::size(s);
        }

        template<class String>
        auto to_str (String&& s)
        {
            using noref_t  = std::remove_reference_t<String>;
            using string_t = std::conditional_t< std::is_array_v<noref_t>, noref_t, std::remove_cv_t<noref_t> >;
            return string_traits<string_t>::to_str(s);
        }
    }

    template<class... Strings>
    auto concat (Strings const&... str) -> std::string
    {
        auto result = std::string {};
        result.reserve((aux_impl::string_size(str) + ...));
        return (result += ... += aux_impl::to_str(str));
    }

    inline auto to_words (std::string s) -> std::vector<std::string>
    {
        auto const delims = {' '};
        auto const end    = std::cend(s);
        auto first        = std::cbegin(s);
        auto words        = std::vector<std::string> {};

        while (first != end)
        {
            auto const last = std::find_first_of(first, end, std::cbegin(delims), std::cend(delims));

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

    inline auto to_head_tail (std::string s) -> std::pair<std::string, std::string>
    {
        auto const firstSpaceIt = std::find(std::begin(s), std::end(s), ' ');

        if (std::end(s) == firstSpaceIt)
        {
            return std::make_pair(s, "");
        }

        auto const firstSpacePos = static_cast<std::size_t>(firstSpaceIt - std::begin(s));
        return std::make_pair( s.substr(0, firstSpacePos)
                             , s.substr(firstSpacePos + 1) );
    }

    inline auto shrink_spaces (std::string s) -> std::string
    {
        auto newEnd = std::unique(std::begin(s), std::end(s), 
            [](const char lhs, const char rhs) 
        {
            return (lhs == rhs) && (lhs == ' ');
        });

        s.erase(newEnd, std::end(s));
        return s;
    }

    inline auto trim (std::string s) -> std::string
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

    inline auto reverse (std::string s) -> std::string
    {
        std::reverse(std::begin(s), std::end(s));
        return s;
    }

    inline auto starts_with 
        (std::string_view s, std::string_view pattern) -> bool
    {
        if (s.size() < pattern.size())
        {
            return false;
        }

        auto stringIt  = std::begin(s);
        auto stringEnd = std::end(s);

        while (std::isspace(*stringIt))
        {
            ++stringIt;
        }

        auto patternIt  = std::begin(pattern);
        auto patternEnd = std::end(pattern);

        while (stringIt != stringEnd && patternIt != patternEnd)
        {
            if (*stringIt++ != *patternIt++)
            {
                return false;
            }
        }

        return true;
    }

    template<class InputIt>
    auto concat_range ( InputIt first
                      , InputIt last
                      , std::string_view glue ) -> std::string
    {
        if (first == last)
        {
            return std::string {};
        }

        auto ost = std::ostringstream {};

        ost << *first;
        ++first;

        while (first != last)
        {
            ost << glue << *first;
            ++first;
        }

        return ost.str();
    }

    template<class Range>
    auto concat_range ( Range const& strs
                      , std::string_view glue ) -> std::string
    {
        return concat_range(std::begin(strs), std::end(strs), glue);
    }
}

#endif