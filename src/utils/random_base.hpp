#ifndef _MIX_UTILS_RANDOM_BASE_
#define _MIX_UTILS_RANDOM_BASE_

#include <random>

namespace mix::utils
{
    class random_base
    {
    protected:
        std::mt19937 generator_;

    public:
        explicit random_base(unsigned long seed);
    };    
}

#endif