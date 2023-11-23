#ifndef LIBTEDDY_EXPERIMENTS_UTILS_HPP
#define LIBTEDDY_EXPERIMENTS_UTILS_HPP

#include <chrono>

namespace teddy::utils
{
    char const* unit_str(std::chrono::nanoseconds) { return "ns"; }
    char const* unit_str(std::chrono::microseconds){ return "µs"; }
    char const* unit_str(std::chrono::milliseconds){ return "ms"; }
    char const* unit_str(std::chrono::seconds)     { return "s";  }

    struct duration_measurement
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> start_;
        std::chrono::nanoseconds total_ {std::chrono::nanoseconds::zero()};
    };

    inline auto tick (duration_measurement& stat) -> void
    {
        stat.start_ = std::chrono::high_resolution_clock::now();
    }

    inline auto tock (duration_measurement& stat) -> void
    {
        stat.total_ += std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - stat.start_
        );
    }

    template<class Duration>
    auto duration_as (duration_measurement const& duration)
    {
        return std::chrono::duration_cast<Duration>(duration.total_).count();
    }
}

#endif