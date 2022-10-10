#ifndef LIBTEDDY_TESTS_TRUTH_TABLE_UTILS_HPP
#define LIBTEDDY_TESTS_TRUTH_TABLE_UTILS_HPP

#include "truth_table.hpp"
#include <cassert>
#include <functional>

namespace teddy
{
/**
 *  \brief Invokes \p f with each element of the domain.
 */
template<class F>
auto domain_for_each(truth_table const& table, F const f) -> void
{
    auto const& vector  = table.get_vector();
    auto const varcount = table.get_var_count();
    auto const domains  = table.get_domains();
    auto element        = std::vector<unsigned int>(varcount, 0);
    auto wasLast        = false;
    auto k              = 0u;
    do
    {
        // Invoke f.
        std::invoke(f, vector[k], element);

        // Move to the next element of the domain.
        auto overflow = true;
        auto i        = varcount;
        while (i > 0 && overflow)
        {
            --i;
            ++element[i];
            overflow = element[i] == domains[i];
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
inline auto to_index(
    truth_table const& table, std::vector<unsigned int> const& vars
) -> unsigned int
{
    assert(vars.size() == table.get_var_count());
    auto index = 0u;
    for (auto i = 0u; i < table.get_var_count(); ++i)
    {
        index += vars[i] * table.get_offsets()[i];
    }
    return index;
}
} // namespace teddy

#endif