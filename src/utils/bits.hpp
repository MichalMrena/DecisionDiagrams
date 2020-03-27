#ifndef _MIX_UTILS_BITS_
#define _MIX_UTILS_BITS_

#include <cstdint>
#include <string>
#include <bitset>

namespace mix::utils
{
    auto reverse_bits ( const uint64_t n ) -> uint64_t;
    
    auto to_string    ( const uint64_t bits
                      , const size_t   take ) -> std::string;
    
    auto to_string    ( const std::bitset<128> bits
                      , const size_t           take ) -> std::string;
}

#endif