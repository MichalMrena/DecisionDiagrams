#ifndef _MIX_UTILS_STOPWATCH_
#define _MIX_UTILS_STOPWATCH_

#include <chrono>

namespace mix::utils
{
    class stopwatch
    {
    public:
        using clock        = std::chrono::steady_clock;
        using milliseconds = std::chrono::milliseconds;

    private:
        std::chrono::time_point<clock> timeZero;

    public:
        stopwatch();

        auto elapsed_time () -> milliseconds;
    };
}

#endif