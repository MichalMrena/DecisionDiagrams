#ifndef _MIX_UTILS_COUNTER_
#define _MIX_UTILS_COUNTER_

#include <cstddef>

namespace mix::utils
{
    class counter
    {
    private:
        size_t count {0};

    public:
        auto inc   (const size_t amount = 1) -> void;
        auto get   () const -> size_t;
        auto reset ()       -> void;
    };
}

#endif