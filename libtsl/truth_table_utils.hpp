#ifndef LIBTEDDY_TESTS_TRUTH_TABLE_UTILS_HPP
#define LIBTEDDY_TESTS_TRUTH_TABLE_UTILS_HPP

#include "libteddy/details/types.hpp"
#include "truth_table.hpp"
#include <cassert>
#include <functional>

namespace teddy::tsl
{
/**
 *  \brief Invokes \p f with each element of the domain.
 */
template<class F>
auto domain_for_each(
    int32 const varcount,
    std::vector<int32> const& vector,
    std::vector<int32> const& domains,
    F f
) -> void
{
    auto element = std::vector<int32>(as_usize(varcount), 0);
    auto wasLast = false;
    auto k       = 0;
    do
    {
        // Invoke f.
        std::invoke(f, vector[as_uindex(k)], element);

        // Move to the next element of the domain.
        auto overflow = true;
        auto i        = varcount;
        while (i > 0 && overflow)
        {
            --i;
            ++element[as_uindex(i)];
            overflow = element[as_uindex(i)] == domains[as_uindex(i)];
            if (overflow)
            {
                element[as_uindex(i)] = 0;
            }
        }

        wasLast = overflow && i == 0;
        ++k;
    } while (not wasLast);
}

/**
 *  \brief Invokes \p f with each element of the domain.
 */
template<class F>
auto domain_for_each(truth_table const& table, F f) -> void
{
    domain_for_each(
        table.get_var_count(), table.get_vector(), table.get_domains(), f
    );
}

/**
 *  \brief Maps values of variables to index in the vector.
 */
inline auto to_index(
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

#endif