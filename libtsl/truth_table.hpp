#ifndef LIBTEDDY_TSL_TRUTH_TABLE_HPP
#define LIBTEDDY_TSL_TRUTH_TABLE_HPP

#include <libtsl/types.hpp>

#include <functional>
#include <limits>
#include <vector>

namespace teddy::tsl
{
inline auto constexpr U = std::numeric_limits<int32>::max();

/**
 *  \brief TODO
 */
class truth_table
{
public:
    truth_table(std::vector<int32> vector, std::vector<int32> domains);

    auto get_var_count () const -> int32;
    auto get_vector () const -> std::vector<int32> const&;
    auto get_domains () const -> std::vector<int32> const&;
    auto get_offsets () const -> std::vector<int32> const&;
    auto get_max_val () const -> int32;

private:
    std::vector<int32> vector_;
    std::vector<int32> domain_;
    std::vector<int32> offset_;
    int32 maxValue_;
};

/**
 *  \brief TODO
 */
auto satisfy_count (truth_table const& table, int32 j) -> int64;

/**
 *  \brief TODO
 */
auto satisfy_all (truth_table const& table, int32 j)
    -> std::vector<std::vector<int32>>;

/**
 *  \brief TODO
 */
auto domain_size (truth_table const& table) -> int64;

/**
 *  \brief TODO
 */
auto evaluate (truth_table const& table, std::vector<int32> const& vars)
    -> int32;

/**
 *  \brief Maps values of variables to index in the vector.
 */
auto to_index (truth_table const& table, std::vector<int32> const& vars)
    -> int32;

/**
 *  \brief Invokes \p f with each element of the domain.
 */
template<class F>
auto domain_for_each (
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
auto domain_for_each (truth_table const& table, F f) -> void
{
    domain_for_each(
        table.get_var_count(), table.get_vector(), table.get_domains(), f
    );
}

} // namespace teddy::tsl

#endif