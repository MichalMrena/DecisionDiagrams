#ifndef MIX_DD_UTILS_MORE_ASSERT_HPP
#define MIX_DD_UTILS_MORE_ASSERT_HPP

#include <stdexcept>

namespace teddy::utils
{
    inline auto runtime_assert(bool const b, char const* msg)
    {
        // TODO global toggle macro #define UNSAFE
        if (!b)
        {
            throw std::runtime_error(msg);
        }
    }
}

#endif