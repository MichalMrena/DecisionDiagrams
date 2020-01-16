#ifndef _MIX_UTILS_BITS_
#define _MIX_UTILS_BITS_

#include <cstdint>
#include <string>

namespace mix::utils
{
    auto reverse_bits (const uint64_t n) -> uint64_t;
    auto to_string    ( const uint64_t n
                      , const size_t take) -> std::string;
}

#endif