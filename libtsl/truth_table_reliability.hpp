#ifndef LIBTEDDY_TSL_TRUTH_TABLE_RELIABILITY_HPP
#define LIBTEDDY_TSL_TRUTH_TABLE_RELIABILITY_HPP

#include <libteddy/details/types.hpp>

#include <libtsl/truth_table.hpp>

#include <limits>
#include <vector>

namespace teddy::tsl
{
struct var_change
{
    int32 index;
    int32 from;
    int32 to;
};

/**
 *  \brief Calculates probability of system state \p val
 *  \param table truth table of structure function
 *  \param probabilities component state probabilities
 *  \param systemState system state
 *  \return system state probability
 */
auto probability (
    truth_table const& table,
    std::vector<std::vector<double>> const& probabilities,
    int32 systemState
) -> double;

/**
 *  \brief Calculates availability with respect to system state \p val
 *  \param table truth table of structure function
 *  \param probabilities component state probabilities
 *  \param systemState system state
 *  \return system availability
 */
auto availability (
    truth_table const& table,
    std::vector<std::vector<double>> const& probabilities,
    int32 systemState
) -> double;

/**
 *  \brief Calculates unavailability with respect to system state \p val
 *  \param table truth table of structure function
 *  \param probabilities component state probabilities
 *  \param val system state
 */
auto unavailability (
    truth_table const& table,
    std::vector<std::vector<double>> const& probabilities,
    int32 systemState
) -> double;

/**
 *  \brief Calculates state frequency of system state \p val
 *  \param table truth table of structure function
 *  \param val system state
 *  \return system state frequency
 */
auto state_frequency (truth_table const& table, int32 systemState) -> double;

/**
 *  \brief Calculcates structural importance using \p dpld
 *  \param dpld derivative to use for the calculation.
 *  \param index index of the variable
 *  \return structural importance
 */
auto structural_importance (truth_table const& dpld, int32 componentIndex) -> double;

/**
 *  \brief Calculcates birnbaum importance using \p dpld
 *  \param dpld derivative to use for the calculation.
 *  \param probabilities component state probabilities
 *  \return birnbaum importance
 */
auto birnbaum_importance (
    truth_table const& dpld, std::vector<std::vector<double>> const& probabilities
) -> double;

/**
 *  \brief Returns lambda that can be used in basic @c dpld
 */
inline static auto constexpr dpld_basic = [] (auto const ffrom, auto const fto)
{
    return [=] (auto const lhs, auto const rhs)
    {
        return lhs == ffrom && rhs == fto;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 1
 */
inline static auto constexpr dpld_i_1_decrease = [] (auto const val)
{
    return [val] (auto const lhs, auto const rhs)
    {
        return lhs == val && rhs < val;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 1
 */
inline static auto constexpr dpld_i_1_increase = [] (auto const val)
{
    return [val] (auto const lhs, auto const rhs)
    {
        return lhs == val && rhs > val;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 2
 */
inline static auto constexpr dpld_i_2_decrease = [] ()
{
    return [] (auto const lhs, auto const rhs)
    {
        return lhs > rhs;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 2
 */
inline static auto constexpr dpld_i_2_increase = [] ()
{
    return [] (auto const lhs, auto const rhs)
    {
        return lhs < rhs;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 3
 */
inline static auto constexpr dpld_i_3_decrease = [] (auto const val)
{
    return [val] (auto const lhs, auto const rhs)
    {
        return lhs >= val && rhs < val;
    };
};

/**
 *  \brief Returns lambda that can be used in @c dpld of type 3
 */
inline static auto constexpr dpld_i_3_increase = [] (auto const val)
{
    return [val] (auto const lhs, auto const rhs)
    {
        return lhs < val && rhs >= val;
    };
};

/**
 *  \brief Calculates DPLD
 *  \param table function to derivate
 *  \param var change in the value of the variable
 *  \param d function that check whether change satisfies the derivative
 *  \return new truth table representing DPLD
 */
template<class F>
auto dpld (truth_table const& table, var_change const var, F change) -> truth_table
{
    auto dpldVector = std::vector<int32>(table.get_vector().size());

    domain_for_each(
        table,
        [&, index = 0U, tmpelem = std::vector<int32>()] (
            auto const ffrom, auto const& elem
        ) mutable
        {
            if (elem[as_uindex(var.index)] != var.from)
            {
                dpldVector[index] = U;
            }
            else
            {
                tmpelem                       = elem;
                tmpelem[as_uindex(var.index)] = var.to;
                auto const fto                = evaluate(table, tmpelem);
                dpldVector[index]             = change(ffrom, fto) ? 1 : 0;
            }
            ++index;
        }
    );

    return {std::move(dpldVector), table.get_domains()};
}
} // namespace teddy::tsl

#endif