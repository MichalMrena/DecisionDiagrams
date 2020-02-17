#include "stopwatch.hpp"

namespace mix::utils
{
    auto stopwatch::avg_run_time 
        ( const size_t replications
        , std::function<void()> code ) -> milliseconds
    {
        code();
        
        stopwatch watch;

        for (size_t i {0}; i < replications; i++)
        {
            code();
        }
        
        return watch.elapsed_time() / replications;
    }

    stopwatch::stopwatch() :
        timeZero {clock::now()}
    {
    }

    auto stopwatch::elapsed_time
        () -> milliseconds
    {
        return std::chrono::duration_cast<milliseconds>(clock::now() - this->timeZero);
    }
}