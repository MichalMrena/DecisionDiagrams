#ifndef MIX_DD_MORE_VECTOR_HPP
#define MIX_DD_MORE_VECTOR_HPP

#include <vector>

namespace teddy::utils
{
    /**
     *  @brief Creates vector with given initial capacity.
     */
    template<class T, class Allocator = std::allocator<T>>
    auto vector(typename std::vector<T, Allocator>::size_type initCapacity)
    {
        auto v = std::vector<T, Allocator>();
        v.reserve(initCapacity);
        return v;
    }
}

#endif