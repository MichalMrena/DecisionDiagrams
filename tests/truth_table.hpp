#ifndef LIBTEDDY_TESTS_TRUTH_TABLE_HPP
#define LIBTEDDY_TESTS_TRUTH_TABLE_HPP

#include <vector>

namespace teddy
{
class truth_table
{
public:
    truth_table(
        std::vector<unsigned int> vector, std::vector<unsigned int> domains
    );

    auto get_var_count() const -> std::size_t;
    auto get_vector() const -> std::vector<unsigned int> const&;
    auto get_domains() const -> std::vector<unsigned int> const&;
    auto get_offsets() const -> std::vector<unsigned int> const&;

private:
    std::vector<unsigned int> vector_;
    std::vector<unsigned int> domains_;
    std::vector<unsigned int> offset_;
    unsigned int maxValue_;
};

auto satisfy_count(truth_table const& table, unsigned int j) -> std::size_t;
auto satisfy_all(truth_table const& table, unsigned int j)
    -> std::vector<std::vector<unsigned int>>;
} // namespace teddy

#endif