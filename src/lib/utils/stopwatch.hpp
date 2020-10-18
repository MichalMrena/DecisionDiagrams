#ifndef MIX_UTILS_STOPWATCH_HPP
#define MIX_UTILS_STOPWATCH_HPP

#include <chrono>
#include <functional>

namespace mix::utils
{
    /**
     *  Simple class for measuring elapsed time.
     */
    class stopwatch
    {
    public:
        using clock        = std::chrono::steady_clock;
        using milliseconds = std::chrono::milliseconds;
        using time_point_t = std::chrono::time_point<clock>;

    public:
        auto reset () -> void;

        /**
         *  Returns time since the initialization of the object
         *  or since the last call to reset.
         *  @return time in std::chrono::milliseconds.
         */
        auto elapsed_time () const -> milliseconds;

    private:
        time_point_t timeZero_ = clock::now();
    };

    inline auto stopwatch::reset
        () -> void
    {
        timeZero_ = clock::now();
    }

    inline auto stopwatch::elapsed_time
        () const -> milliseconds
    {
        return std::chrono::duration_cast<milliseconds>(clock::now() - timeZero_);
    }

    /**
     *  Measures how long it takes to execute given function.
     *  @return time in milliseconds.
     */
    template<class Function>
    auto run_time (Function&& function) -> double
    {
        auto watch = stopwatch();
        function();
        return watch.elapsed_time().count();
    }

    /**
     *  Measures how long it takes to execute given function.
     *  Executes the function @p replications times and returns
     *  the average time for one execution.
     *  @return time in milliseconds.
     */
    template<class Function>
    auto avg_run_time ( std::size_t const replications
                      , Function&& function ) -> double
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