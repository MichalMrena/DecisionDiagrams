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

namespace teddy::utils
{
    /**
     *  @brief Removes spaces in place both side of the @p s .
     */
    auto trim (std::string& s) -> void
    {
        s.erase(std::begin(s), std::find_if(std::begin(s), std::end(s), [](auto const c)
        {
            return !std::isspace(c);
        }));

        s.erase(std::find_if(std::rbegin(s), std::rend(s), [](auto const c)
        {
            return !std::isspace(c);
        }).base(), std::end(s));
    }

    /**
     *  @brief Replaces groups of spaces (' ') with a single one.
     *  @return new string with no consecutive spaces.
     */
    auto shrink_spaces (std::string& s) -> void
    {
        auto const newEnd = std::unique(std::begin(s), std::end(s), [](auto const l, auto const r)
        {
            return (l == r) && (l == ' ');
        });
        s.erase(newEnd, std::end(s));
    }

    /**
     *  @brief Splits the string into two parts.
     *  The first one contains the first word (head)
     *  and the second one constains the rest of the string (tail).
     *  @return pair of string_view that are the head and the tail of @p sv .
     */
    auto to_head_tail (std::string_view sv) -> std::pair<std::string_view, std::string_view>
    {
        auto const firstSpaceIt = std::find(std::begin(sv), std::end(sv), ' ');

        if (std::end(sv) == firstSpaceIt)
        {
            return std::make_pair("", "");
        }

        auto const firstSpacePos = static_cast<std::size_t>(firstSpaceIt - std::begin(sv));
        return std::make_pair( sv.substr(0, firstSpacePos)
                             , sv.substr(firstSpacePos + 1) );
    }

    /**
     *  @brief Splits the string into words using space as a delimiter.
     *  @return vector of new string that are words from @p s .
     */
    auto to_words (std::string_view sv, std::string_view delimiters = " ") -> std::vector<std::string>
    {
        auto const end = std::cend(sv);
        auto first     = std::cbegin(sv);
        auto words     = std::vector<std::string>();

        while (first != end)
        {
            auto const last = std::find_first_of(first, end, std::cbegin(delimiters), std::cend(delimiters));

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

    /**
     *  @brief Check if @p sv begins with @p prefix . Ignores spaces.
     *  @return true if @p sv starts with @p prefix false otherwise.
     */
    auto starts_with (std::string_view sv, std::string_view prefix) -> bool
    {
        if (sv.size() < prefix.size())
        {
            return false;
        }

        auto const end = std::end(sv);
        auto svIt      = std::begin(sv);

        while (std::isspace(*svIt) && svIt < end)
        {
            ++svIt;
        }

        return std::equal(std::begin(prefix), std::end(prefix), std::begin(sv));
    }

    /**
     *  @brief Prints each element in [ @p first , @p last ) into @c std::ostream .
     *  @param glue string inserted in between elements.
     *  @return new string representing the range.
     */
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

    /**
     *  @brief Prints each element in @p range into @c std::ostream .
     *  @param glue string inserted in between elements.
     *  @return new string representing the range.
     */
    template<class Range>
    auto concat_range ( Range const& strs
                      , std::string_view glue ) -> std::string
    {
        return concat_range(std::begin(strs), std::end(strs), glue);
    }

    namespace detail
    {
        // https://stackoverflow.com/a/18899027/8616625

        template<class N>
        struct string_traits
        {
            static constexpr auto size (N n) { return std::to_string(n).size(); }
            static auto to_str (N n) { using std::to_string; return to_string(n); }
        };

        template<size_t N>
        struct string_traits<const char[N]>
        {
            static constexpr auto size   (char const (&) [N])  { return N - 1; }
            static constexpr auto to_str (char const (&s) [N]) { return s; }
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

    /**
     *  @brief Concatenates strings of different types.
     *  Numbers are converted to string using @c std::to_string .
     */
    template<class... Strings>
    auto concat (Strings const&... str) -> std::string
    {
        auto result = std::string {};
        result.reserve((detail::string_size(str) + ...));
        return (result += ... += detail::to_str(str));
    }
}

#endif