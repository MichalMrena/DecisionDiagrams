#include <libtsl/truth_table.hpp>
#include <algorithm>
#include <cassert>
#include <functional>
#include <numeric>
#include <ranges>

namespace teddy::tsl
{
truth_table::truth_table(
    std::vector<int32> vector, std::vector<int32> domains
)
    : vector_(std::move(vector)), domain_(std::move(domains)),
      offset_(as_usize(this->get_var_count())),
      maxValue_(std::ranges::max(vector_ | std::ranges::views::filter(
                                                   [](auto v)
                                                   {
                                                       return v != U;
                                                   }
                                               )
                                 ))
{
    assert(
        vector_.size() ==
        std::reduce(begin(domain_), end(domain_), 1ull, std::multiplies<>())
    );

    assert(this->get_var_count() > 0);

    offset_[as_uindex(this->get_var_count() - 1)] = 1;
    if (this->get_var_count() > 1)
    {
        auto i = this->get_var_count() - 1;
        while (i > 0)
        {
            --i;
            offset_[as_uindex(i)] = domain_[as_uindex(i + 1)]
                                  * offset_[as_uindex(i + 1)];
        }
    }
}

auto truth_table::get_var_count() const -> int32
{
    return static_cast<int32>(ssize(domain_));
}

auto truth_table::get_vector() const -> std::vector<int32> const&
{
    return vector_;
}

auto truth_table::get_domains() const -> std::vector<int32> const&
{
    return domain_;
}

auto truth_table::get_offsets() const -> std::vector<int32> const&
{
    return offset_;
}

auto truth_table::get_max_val() const -> int32
{
    return maxValue_;
}

auto satisfy_count(truth_table const& table, int32 j) -> int64
{
    auto result = int64(0);
    for (auto const e : table.get_vector())
    {
        result += static_cast<int32>(e == j);
    }
    return result;
}

auto satisfy_all(truth_table const& table, int32 j)
    -> std::vector<std::vector<int32>>
{
    auto elems = std::vector<std::vector<int32>>();
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

auto domain_size(truth_table const& table) -> int64
{
    return ssize(table.get_vector());
}

auto evaluate(truth_table const& table, std::vector<int32> const& vars)
    -> int32
{
    return table.get_vector()[as_uindex(to_index(table, vars))];
}

/**
 *  \brief Maps values of variables to index in the vector.
 */
auto to_index(
    truth_table const& table, std::vector<int32> const& vars
) -> int32
{
    assert(ssize(vars) == table.get_var_count());
    auto index = 0;
    for (auto i = 0; i < table.get_var_count(); ++i)
    {
        index += vars[as_uindex(i)] * table.get_offsets()[as_uindex(i)];
    }
    return index;
}
} // namespace teddy