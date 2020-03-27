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

    template<class Container>
    auto concat_range ( const Container& strs
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
        struct string_size_impl
        {
            static constexpr size_t size (N n) { return std::to_string(n).size(); }
        };

        template<size_t N>
        struct string_size_impl<const char[N]> 
        {
            static constexpr size_t size (const char (&) [N]) { return N - 1; }
        };

        template<size_t N>
        struct string_size_impl<char[N]> 
        {
            static size_t size (char (&s) [N]) { return N ? std::strlen(s) : 0; }
        };

        template<>
        struct string_size_impl<const char*> 
        {
            static size_t size (const char* s) { return s ? std::strlen(s) : 0; }
        };

        template<>
        struct string_size_impl<char*> 
        {
            static size_t size (char* s) { return s ? std::strlen(s) : 0; }
        };

        template<>
        struct string_size_impl<std::string> 
        {
            static size_t size (const std::string& s) { return s.size(); }
        };

        template<>
        struct string_size_impl<std::string_view> 
        {
            static size_t size (std::string_view s) { return s.size(); }
        };

        template<class T>
        struct to_str_impl
        {
            static auto to_str (const T& n) -> std::string { using std::to_string; return to_string(n); }
        };

        template<size_t N>
        struct to_str_impl<const char[N]> 
        {
            static auto to_str (const char (&s) [N]) -> const char (&) [N] { return s; }
        };

        template<size_t N>
        struct to_str_impl<char[N]> 
        {
            static auto to_str (char (&s) [N]) -> char (&) [N] { return s; }
        };

        template<>
        struct to_str_impl<const char*> 
        {
            static auto to_str (const char* s) -> const char* { return s; }
        };

        template<>
        struct to_str_impl<char*> 
        {
            static auto to_str (char* s) -> char* { return s; }
        };

        template<>
        struct to_str_impl<std::string> 
        {
            static auto to_str (const std::string& s) -> const std::string& { return s; }
        };

        template<>
        struct to_str_impl<std::string_view> 
        {
            static auto to_str (std::string_view s) -> std::string_view { return s; }
        };

        template<class String> 
        auto string_size (String&& s) -> size_t
        {
            using noref_t  = std::remove_reference_t<String>;
            using string_t = std::conditional_t< std::is_array_v<noref_t>, noref_t, std::remove_cv_t<noref_t> >;

            return string_size_impl<string_t>::size(s);
        }

        template<class String>
        auto to_str (String&& s)
        {
            using noref_t  = std::remove_reference_t<String>;
            using string_t = std::conditional_t< std::is_array_v<noref_t>, noref_t, std::remove_cv_t<noref_t> >;

            return to_str_impl<string_t>::to_str(s);
        }

        template<class...>
        struct concatenate_impl;

        template<class String>
        struct concatenate_impl<String> 
        {
            static auto size (String&& s) -> size_t
            { 
                return string_size(s); 
            }
            
            static auto concatenate (std::string& result, String&& s) -> void
            { 
                result += to_str(s);
            }
        };

        template<class String, class... Rest>
        struct concatenate_impl<String, Rest...> 
        {
            static auto size (String&& s, Rest&&... rest) -> size_t
            {
                return string_size(s) + concatenate_impl<Rest...>::size(std::forward<Rest>(rest)...);
            }

            static auto concatenate (std::string& result, String&& s, Rest&&... rest) -> void
            {
                result += to_str(s);
                concatenate_impl<Rest...>::concatenate(result, std::forward<Rest>(rest)...);
            }
        };
    }

    template<class... Strings>
    auto concat (Strings&&... strings) -> std::string
    {
        std::string result;
        result.reserve(aux_impl::concatenate_impl<Strings...>::size(std::forward<Strings>(strings)...));
        aux_impl::concatenate_impl<Strings...>::concatenate(result, std::forward<Strings>(strings)...);
        return result;
    }
}

#endif