#ifndef _MIX_UTILS_PARSING_UTILS_
#define _MIX_UTILS_PARSING_UTILS_

#include <string_view>
#include <stdexcept>
#include <charconv>

namespace mix::utils
{
    template<class N>
    struct parse_result
    {
        N    value;
        bool isValid;
        operator N    () const { return value; }
        operator bool () const { return isValid; }
    };

    template<class N>
    auto parse (std::string_view in) -> parse_result<N>
    {
        auto ret    = N {};
        auto result = std::from_chars(in.data(), in.data() + in.size(), ret);

        return {ret, std::errc {} == result.ec && result.ptr == in.data() + in.size()};
    }

    template<class N>
    auto parse_except (std::string_view in) -> N
    {
        auto const res = parse<N>(in);
        
        if (!res.isValid)
        {
            throw std::invalid_argument {"Failed to parse number."};
        }

        return res;
    }
}

#endif