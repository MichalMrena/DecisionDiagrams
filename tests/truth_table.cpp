#include "truth_table.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

namespace teddy
{
namespace
{
/**
 *  \brief Invokes \p f with each element of the domain.
 */
template<class F>
auto domain_for_each(truth_table const& table, F const f) -> void
{
    auto const& vector  = table.get_vector();
    auto const varcount = table.get_var_count();
    auto element        = std::vector<unsigned int>(varcount, 0);
    auto wasLast        = false;
    auto k              = 0u;
    do
    {
        // Invoke f.
        f(vector[k], element);

        // Move to the next element of the domain.
        auto overflow = true;
        auto i        = varcount;
        while (i > 0 && overflow)
        {
            --i;
            ++element[i];
            overflow = element[i] == domains_[i];
            if (overflow)
            {
                element[i] = 0;
            }
        }

        wasLast = overflow && i == 0;
        ++k;
    } while (not wasLast);
}

/**
 *  \brief Maps values of variables to index in the vector.
 */
auto to_index(truth_table const& table, std::vector<unsigned int> const& vars)
    -> unsigned int
{
    assert(vars.size() == table.get_var_count());
    auto index = 0u;
    for (auto i = 0u; i < table.get_var_count(); ++i)
    {
        index += vars[i] * table.get_offsets()[i];
    }
    return index;
}
} // namespace

truth_table::truth_table(
    std::vector<unsigned int> vector, std::vector<unsigned int> domains
)
    : vector_(std::move(vector)), domains_(std::move(domains_)),
      offset_(this->get_var_count()), maxValue_(std::ranges::max(vector_))
{
    assert(
        vector_.size() ==
        std::reduce(begin(domains_), end(domains_), 1u, std::multiplies<>())
    );

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

auto satisfy_count(truth_table const& table, unsigned int j) -> std::size_t
{
    auto result = 0u;
    for (auto const e : table.get_vector())
    {
        result += static_cast<unsigned int>(e == j);
    }
    return result;
}

auto satisfy_all(truth_table const& table)
    -> std::vector<std::vector<unsigned int>>
{
    auto elems = std::vector<std::vector<unsigned int>>();
    domain_for_each(
        table,
        [&elems](auto const val, auto elem)
        {
            if (val == 1)
            {
                elems.emplace_back(std::move(elem));
            }
        }
    );
    return elems;
}
} // namespace teddy