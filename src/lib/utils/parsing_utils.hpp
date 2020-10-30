#ifndef MIX_UTILS_PARSING_UTILS_HPP
#define MIX_UTILS_PARSING_UTILS_HPP

#include <string_view>
#include <stdexcept>
#include <charconv>

namespace mix::utils
{
    template<class Num>
    struct parse_result
    {
        Num  value;
        bool isValid;
        operator Num  () const { return value; }
        operator bool () const { return isValid; }
    };

    template<class Num>
    auto parse (std::string_view in) -> parse_result<Num>
    {
        auto ret    = Num {};
        auto result = std::from_chars(in.data(), in.data() + in.size(), ret);

        return {ret, std::errc {} == result.ec && result.ptr == in.data() + in.size()};
    }

    template<class Num>
    auto parse_except (std::string_view in) -> Num
    {
        auto const res = parse<Num>(in);

        if (!res)
        {
            throw std::invalid_argument {"Failed to parse number."};
        }

        return res;
    }
}

#endif