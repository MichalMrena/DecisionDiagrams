#ifndef _MIX_UTILS_STRING_UTILS_
#define _MIX_UTILS_STRING_UTILS_

#include <string>
#include <vector>
#include <utility>
#include <sstream>
#include <iterator>
#include <string_view>
#include <cstring>

namespace mix::utils
{
	// TODO po správnosti by tu asi mali byť 2 overloady, jeden pre const string ref a druhý pre rval ref
    auto to_words      (std::string s) -> std::vector<std::string>;
    auto to_head_tail  (std::string s) -> std::pair<std::string, std::string>;
    auto shrink_spaces (std::string s) -> std::string;
    auto trim          (std::string s) -> std::string;
    auto reverse       (std::string s) -> std::string;

    auto starts_with   ( std::string_view s
                       , std::string_view pattern ) -> bool;

    template<class InputIt>
    auto concat_range ( InputIt it
                      , InputIt end
                      , std::string_view glue ) -> std::string
    {
        if (it == end)
        {
            return std::string {};
        }

        auto ost = std::ostringstream {};

        ost << *it;
        ++it;

        while (it != end)
        {
            ost << glue << *it;
            ++it;
        }

        return ost.str();
    }

    template<class Container>
    auto concat_range ( Container const& strs
                      , std::string_view glue ) -> std::string
    {
        return concat_range(std::begin(strs), std::end(strs), glue);
    }

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
        struct string_traits<const char*> 
        {
            static auto size   (const char* s) { return s ? std::strlen(s) : 0; }
            static auto to_str (const char* s) { return s; }
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
        result.reserve((0 + ... + aux_impl::string_size(str)));
        return (result += ... += aux_impl::to_str(str));
    }
}

#endif