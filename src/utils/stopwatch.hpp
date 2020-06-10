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
        
    public:
        auto start        ()       -> void; 
        auto elapsed_time () const -> milliseconds;
    
    private:
        std::chrono::time_point<clock> timeZero_ = clock::now();
    };

    inline auto stopwatch::start
        () -> void
    {
        timeZero_ = clock::now();
    }

    inline auto stopwatch::elapsed_time
        () const -> milliseconds
    {
        return std::chrono::duration_cast<milliseconds>(clock::now() - timeZero_);
    }

    template<class Func>
    auto avg_run_time ( size_t const replications
                      , Func code ) -> typename stopwatch::milliseconds
    {
        auto watch = stopwatch {};

        for (auto i = 0u; i < replications; ++i)
        {
            code();
        }
        
        return watch.elapsed_time() / replications;
    }
}

#endif