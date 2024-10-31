#ifndef LIBTEDDY_DETAILS_STATS_HPP
#define LIBTEDDY_DETAILS_STATS_HPP

#include <libteddy/impl/config.hpp>
#ifdef LIBTEDDY_COLLECT_STATS

#    include <libteddy/impl/types.hpp>

#    include <chrono>
#    include <iostream>

namespace teddy::stats
{
struct teddy_stats
{
    struct query_frequency
    {
        int64 hitCount_ {0};
        int64 totalCount_ {0};
    };

    struct operation_duration
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> start_;
        std::chrono::nanoseconds total_ {std::chrono::nanoseconds::zero()};
    };

    int64 applyStepCalls_ {0};
    int64 maxUniqueNodes_ {0};
    int64 maxAllocatedNodes_ {0};
    query_frequency uniqueTableQueries_;
    query_frequency applyCacheQueries_;
    operation_duration collectGarbage_;
    operation_duration makeNode_;
};

inline auto get_stats () -> teddy_stats&
{
    static teddy_stats instance;
    return instance;
}

inline auto tick (teddy_stats::operation_duration& stat) -> void
{
    stat.start_ = std::chrono::high_resolution_clock::now();
}

inline auto tock (teddy_stats::operation_duration& stat) -> void
{
    stat.total_ += std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now() - stat.start_
    );
}
} // namespace teddy::stats

namespace teddy
{
inline auto dump_stats () -> void
{
    auto& stats = stats::get_stats();
    std::cout << "Unique table" << "\n"
              << "  hit   = " << stats.uniqueTableQueries_.hitCount_ << "\n"
              << "  total = " << stats.uniqueTableQueries_.totalCount_ << "\n"
              << "Apply cache" << "\n"
              << "  hit   = " << stats.applyCacheQueries_.hitCount_ << "\n"
              << "  total = " << stats.applyCacheQueries_.totalCount_ << "\n"
              << "Collect garbage" << "\n"
              << "  total = " << stats.collectGarbage_.total_.count() << "ns\n"
              << "Make node" << "\n"
              << "  total = " << stats.makeNode_.total_.count() << "ns\n"
              << "Apply step" << "\n"
              << "  calls = " << stats.applyStepCalls_ << "\n";
}
} // namespace teddy

#endif // LIBTEDDY_COLLECT_STATS
#endif // LIBTEDDY_DETAILS_STATS_HPP