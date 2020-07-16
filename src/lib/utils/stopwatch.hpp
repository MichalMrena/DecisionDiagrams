#ifndef MIX_UTILS_STOPWATCH_
#define MIX_UTILS_STOPWATCH_

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
    auto run_time (Func&& function) -> double
    {
        auto watch = stopwatch {};
        function();
        return watch.elapsed_time().count();
    }

    template<class Func>
    auto avg_run_time ( std::size_t const replications
                      , Func&& function ) -> double
    {
        auto sum = 0.0;
        for (auto i = 0u; i < replications; ++i)
        {
            sum += run_time(function);
        }
        return sum / replications;
    }
}

#endif