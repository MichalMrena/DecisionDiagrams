#ifndef _MIX_UTILS_PARSING_UTILS_
#define _MIX_UTILS_PARSING_UTILS_

#include <cstdint>
#include <string>
#include <stdexcept>

namespace mix::utils
{
    template<class N>
    static auto just_parse (const std::string& in, size_t* idx) -> N;

    template<> 
    auto just_parse<int32_t> (const std::string& in, size_t* idx) -> int32_t
    {
        return std::stoi(in, idx);
    }

    template<> 
    auto just_parse<uint32_t> (const std::string& in, size_t* idx) -> uint32_t
    {
        const auto ul {std::stoul(in, idx)};

        if (ul > std::numeric_limits<uint32_t>::max())
        {
            *idx = 0;
        }

        return static_cast<uint32_t>(ul);
    }

    template<class N
           , class = typename std::enable_if<std::is_arithmetic<N>::value, N>::type>
    auto parse (const std::string& in, N& out) -> bool
    {
        if (in.empty())
        {
            return false;
        }

        size_t idx;
        
        try 
        {
            out = just_parse<N>(in, &idx);
        }
        catch (...)
        {
            return false;
        }

        return idx == in.size();
    }

    template<class N
           , class = typename std::enable_if<std::is_arithmetic<N>::value, N>::type>
    auto parse_except (const std::string& in) -> N
    {
        N out;
        
        if (! parse<N>(in, out))
        {
            throw std::invalid_argument {"Failed to parse number."};
        }

        return out;
    }
}

#endif