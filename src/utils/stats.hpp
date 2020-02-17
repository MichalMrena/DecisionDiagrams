#ifndef _MIX_UTILS_STATS_
#define _MIX_UTILS_STATS_

#include <cstddef>

namespace mix::utils
{
    class averager
    {
    private:
        double accSUm   {0.0};
        size_t valCount {0};

    public:
        auto add_value (const double val) -> void;
        auto average   () const           -> double;
    };    
}

#endif