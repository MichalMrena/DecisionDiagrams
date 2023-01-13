#ifndef LIBTEDDY_TESTS_TRUTH_TABLE_HPP
#define LIBTEDDY_TESTS_TRUTH_TABLE_HPP

#include <libteddy/details/types.hpp>
#include <limits>
#include <vector>

namespace teddy
{
inline auto constexpr U = std::numeric_limits<int32>::max();

class truth_table
{
public:
    truth_table(
        std::vector<int32> vector, std::vector<int32> domains
    );

    auto get_var_count() const -> int32;
    auto get_vector() const -> std::vector<int32> const&;
    auto get_domains() const -> std::vector<int32> const&;
    auto get_offsets() const -> std::vector<int32> const&;
    auto get_max_val() const -> int32;

private:
    std::vector<int32> vector_;
    std::vector<int32> domains_;
    std::vector<int32> offset_;
    int32 maxValue_;
};

auto satisfy_count(truth_table const& table, int32 j) -> int64;
auto satisfy_all(truth_table const& table, int32 j)
    -> std::vector<std::vector<int32>>;
auto domain_size(truth_table const& table) -> int64;
auto evaluate(truth_table const& table, std::vector<int32> const& vars)
    -> int32;
} // namespace teddy

#endif