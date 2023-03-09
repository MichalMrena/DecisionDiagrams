#ifndef LIBTEDDY_TESTS_TABLE_RELIABILITY_HPP
#define LIBTEDDY_TESTS_TABLE_RELIABILITY_HPP

#include "truth_table.hpp"
#include "truth_table_utils.hpp"

#include <libteddy/details/types.hpp>
#include <limits>
#include <vector>

namespace teddy
{
struct var_change
{
    int32 index;
    int32 from;
    int32 to;
};

/**
 *  \brief Calculates probability of system state \p j .
 */
auto probability(
    truth_table const& table,
    std::vector<std::vector<double>> const& ps,
    int32 j
) -> double;

/**
 *  \brief Calculates availability with respect to system state \p j .
 *  \param table truth table of structure function
 *  \param ps component state probabilities
 *  \param j system state
 */
auto availability(
    truth_table const& table,
    std::vector<std::vector<double>> const& ps,
    int32 j
) -> double;

/**
 *  \brief Calculates unavailability with respect to system state \p j .
 *  \param table truth table of structure function
 *  \param ps component state probabilities
 *  \param j system state
 */
auto unavailability(
    truth_table const& table,
    std::vector<std::vector<double>> const& ps,
    int32 j
) -> double;

/**
 *  \brief Calculates state frequency of system state \p j .
 *  \param table truth table of structure function
 *  \param j system state
 *  \return system state frequency
 */
auto state_frequency(truth_table const& table, int32 j) -> double;

/**
 *  \brief Calculcates structural importance using \p dpld .
 *  \param dpld derivative to use for the calculation.
 *  \param i index of the variable
 *  \return structural importance
 */
auto structural_importance(truth_table const& dpld, int32 i) -> double;

/**
 *  \brief Calculcates birnbaum importance using \p dpld .
 *  \param dpld derivative to use for the calculation.
 *  \param ps component state probabilities
 *  \return birnbaum importance
 */
auto birnbaum_importance(
    truth_table const& dpld,
    std::vector<std::vector<double>> const& ps
) -> double;

/**
 *  \brief Returns lambda that can be used in basic @c dpld .
 */
inline static auto constexpr dpld_basic = [](auto const ffrom, auto const fto)
{
    return [=](auto const l, auto const r)
    {
        return l == ffrom && r == fto;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 1.
 */
inline static auto constexpr dpld_i_1_decrease = [](auto const j)
{
    return [j](auto const l, auto const r)
    {
        return l == j && r < j;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 1.
 */
inline static auto constexpr dpld_i_1_increase = [](auto const j)
{
    return [j](auto const l, auto const r)
    {
        return l == j && r > j;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 2.
 */
inline static auto constexpr dpld_i_2_decrease = []()
{
    return [](auto const l, auto const r)
    {
        return l > r;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 2.
 */
inline static auto constexpr dpld_i_2_increase = []()
{
    return [](auto const l, auto const r)
    {
        return l < r;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 3.
 */
inline static auto constexpr dpld_i_3_decrease = [](auto const j)
{
    return [j](auto const l, auto const r)
    {
        return l >= j && r < j;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 3.
 */
inline static auto constexpr dpld_i_3_increase = [](auto const j)
{
    return [j](auto const l, auto const r)
    {
        return l < j && r >= j;
    };
};

template<class F>
auto dpld(truth_table const& table, var_change const var, F d) -> truth_table
{
    auto dpbdvector = std::vector<int32>(table.get_vector().size());

    domain_for_each(
        table,
        [&, k = 0u, tmpelem = std::vector<int32>()](
            auto const ffrom, auto const& elem
        ) mutable
        {
            if (elem[var.index] != var.from)
            {
                dpbdvector[k] = U;
            }
            else
            {
                tmpelem            = elem;
                tmpelem[var.index] = var.to;
                auto const fto     = evaluate(table, tmpelem);
                dpbdvector[k]      = d(ffrom, fto) ? 1 : 0;
            }
            ++k;
        }
    );

    return truth_table(std::move(dpbdvector), table.get_domains());
}
} // namespace teddy

#endif