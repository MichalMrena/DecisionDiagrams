#include "truth_table.hpp"
#include "truth_table_utils.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <numeric>
#include <ranges>

namespace teddy
{
truth_table::truth_table(
    std::vector<unsigned int> vector, std::vector<unsigned int> domains
)
    : vector_(std::move(vector)), domains_(std::move(domains)),
      offset_(this->get_var_count()),
      maxValue_(std::ranges::max(vector_ | std::ranges::views::filter(
        [](auto v){ return v != U; }))
      )
{
    assert(
        vector_.size() ==
        std::reduce(begin(domains_), end(domains_), 1ull, std::multiplies<>())
    );

    assert(this->get_var_count() > 0);

    offset_[this->get_var_count() - 1] = 1;
    if (this->get_var_count() > 1)
    {
        auto i = this->get_var_count() - 1;
        while (i > 0)
        {
            --i;
            offset_[i] = domains_[i + 1] * offset_[i + 1];
        }
    }
}

auto truth_table::get_var_count() const -> std::size_t
{
    return domains_.size();
}

auto truth_table::get_vector() const -> std::vector<unsigned int> const&
{
    return vector_;
}

auto truth_table::get_domains() const -> std::vector<unsigned int> const&
{
    return domains_;
}

auto truth_table::get_offsets() const -> std::vector<unsigned int> const&
{
    return offset_;
}

auto truth_table::get_max_val() const -> unsigned int
{
    return maxValue_;
}

auto satisfy_count(truth_table const& table, unsigned int j) -> std::size_t
{
    auto result = 0u;
    for (auto const e : table.get_vector())
    {
        result += static_cast<unsigned int>(e == j);
    }
    return result;
}

auto satisfy_all(truth_table const& table, unsigned int j)
    -> std::vector<std::vector<unsigned int>>
{
    auto elems = std::vector<std::vector<unsigned int>>();
    domain_for_each(
        table,
        [&elems, j](auto const val, auto elem)
        {
            if (val == j)
            {
                elems.emplace_back(std::move(elem));
            }
        }
    );
    return elems;
}

auto domain_size(truth_table const& table) -> std::size_t
{
    return size(table.get_vector());
}

auto evaluate(
    truth_table const& table,
    std::vector<unsigned int> const& vars
) -> unsigned int
{
    return table.get_vector()[to_index(table, vars)];
}
} // namespace teddy