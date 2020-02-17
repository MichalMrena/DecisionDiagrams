#ifndef _MIX_UTILS_STOPWATCH_
#define _MIX_UTILS_STOPWATCH_

#include <chrono>
#include <functional>

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
        static auto avg_run_time ( const size_t replications
                                 , std::function<void()> code ) -> milliseconds;

        stopwatch();

        auto elapsed_time () -> milliseconds;

    };
}

#endif