#ifndef MIX_DD_UTILS_MORE_ASSERT_HPP
#define MIX_DD_UTILS_MORE_ASSERT_HPP

#include <stdexcept>

#define TEDDY_SAFE

namespace teddy::utils
{
    inline auto runtime_assert
        ([[maybe_unused]] bool const b, [[maybe_unused]] char const* msg)
    {
        #ifdef TEDDY_SAFE
        if (!b)
        {
            throw std::runtime_error(msg);
        }
        #endif
    }
}

#endif