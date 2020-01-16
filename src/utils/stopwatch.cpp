#include "stopwatch.hpp"

namespace mix::utils
{
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