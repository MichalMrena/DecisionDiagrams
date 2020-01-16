#ifndef _MIX_UTILS_RANDOM_UNIFORM_
#define _MIX_UTILS_RANDOM_UNIFORM_

#include <type_traits>
#include <cstdint>
#include <limits>
#include "random_base.hpp"

namespace mix::utils
{
    template<class T> struct is_valid_int_type    : public std::false_type {};
    template<> struct is_valid_int_type<int8_t>   : public std::true_type  {};
    template<> struct is_valid_int_type<uint8_t>  : public std::true_type  {};
    template<> struct is_valid_int_type<int16_t>  : public std::true_type  {};
    template<> struct is_valid_int_type<uint16_t> : public std::true_type  {};
    template<> struct is_valid_int_type<int32_t>  : public std::true_type  {};
    template<> struct is_valid_int_type<uint32_t> : public std::true_type  {};
    template<> struct is_valid_int_type<int64_t>  : public std::true_type  {};
    template<> struct is_valid_int_type<uint64_t> : public std::true_type  {};

    template< class IntType
            , class Enable = typename std::enable_if<is_valid_int_type<IntType>::value, IntType>::type>
    class random_uniform_int : private random_base
    {
    private:
        std::uniform_int_distribution<IntType> dist;

    public:
        random_uniform_int( const IntType min  = std::numeric_limits<IntType>::min()
                          , const IntType max  = std::numeric_limits<IntType>::max()
                          , unsigned long seed = std::random_device {} ());

        auto next_int () -> IntType;
    };

    template<class IntType, class Enable>
    random_uniform_int<IntType, Enable>::random_uniform_int ( const IntType min
                                                            , const IntType max
                                                            , unsigned long seed) :
        random_base {seed}
      , dist {min, max}
    {
    }
    
    template<class IntType, class Enable>
    auto random_uniform_int<IntType, Enable>::next_int
        () -> IntType
    {
        return this->dist(random_base::generator);
    }
}

#endif