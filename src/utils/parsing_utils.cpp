#include "parsing_utils.hpp"

#include <limits>

namespace mix::utils
{
    // template<> 
    // auto just_parse<int32_t> (const std::string& in, size_t* idx) -> int32_t
    // {
    //     return std::stoi(in, idx);
    // }

    // template<> 
    // auto just_parse<uint32_t> (const std::string& in, size_t* idx) -> uint32_t
    // {
    //     const auto ul {std::stoul(in, idx)};

    //     if (ul > std::numeric_limits<uint32_t>::max())
    //     {
    //         *idx = 0;
    //     }

    //     return static_cast<uint32_t>(ul);
    // }

    // auto parse_int (const std::string& in, int32_t& out) -> bool
    // {
    //     size_t idx;
    //     out = std::stoi(in, &idx);

    //     return idx == in.size();
    // }

    // auto parse_uint (const std::string& in, uint32_t& out) -> bool
    // {
    //     size_t idx;

    //     const auto ul {std::stoul(in, &idx)};
    //     out = static_cast<uint32_t>(ul);

    //     return idx == in.size();
    // }

    // auto parse_int_except (const std::string& in) -> int32_t
    // {
    //     int32_t out;
        
    //     if (not parse_int(in, out))
    //     {
    //         throw std::invalid_argument {"Failed to parse int."};
    //     }

    //     return out;
    // }

    // auto parse_uint_except (const std::string& in) -> uint32_t
    // {
    //     uint32_t out;
        
    //     if (not parse_uint(in, out))
    //     {
    //         throw std::invalid_argument {"Failed to parse uint."};
    //     }

    //     return out;
    // }
}