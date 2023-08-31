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
 *  \brief Calculates probability of BSS state \p 1
 *  \param table truth table of structure function
 *  \param probabilities component state probabilities
 *  \return system state probability
 */
auto probability (
    truth_table const& table,
    std::vector<double> const& probabilities
) -> double;

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
auto structural_importance (truth_table const& dpld, int32 componentIndex)
    -> double;

/**
 *  \brief Calculcates birnbaum importance using \p dpld
 *  \param dpld derivative to use for the calculation.
 *  \param probabilities component state probabilities
 *  \return birnbaum importance
 */
auto birnbaum_importance (
    truth_table const& dpld,
    std::vector<std::vector<double>> const& probabilities
) -> double;

/**
 *  \brief Calculates fussel-vesely importance of a component
 *  \param structureFunction structure function
 *  \param probabilities component state probabilities
 *  \param componentIndex component index
 *  \param componentState component state
 *  \param systemState system state
 *  \return fussel-vesely importance
 */
auto fussell_vesely_importance (
    truth_table const& structureFunction,
    std::vector<std::vector<double>> const& probabilities,
    int32 componentIndex,
    int32 componetnState,
    int32 systemState
) -> double;

/**
 *  \brief Returns lambda that can be used in basic \c dpld
 */
inline static auto constexpr dpld_basic = [] (auto const ffrom, auto const fto)
{
    return [=] (auto const lhs, auto const rhs)
    {
        return lhs == ffrom && rhs == fto;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 1
 */
inline static auto constexpr type_1_decrease = [] (auto const val)
{
    return [val] (auto const lhs, auto const rhs)
    {
        return lhs == val && rhs < val;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 1
 */
inline static auto constexpr type_1_increase = [] (auto const val)
{
    return [val] (auto const lhs, auto const rhs)
    {
        return lhs == val && rhs > val;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 2
 */
inline static auto constexpr type_2_decrease = [] ()
{
    return [] (auto const lhs, auto const rhs)
    {
        return lhs > rhs;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 2
 */
inline static auto constexpr type_2_increase = [] ()
{
    return [] (auto const lhs, auto const rhs)
    {
        return lhs < rhs;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 3
 */
inline static auto constexpr type_3_decrease = [] (auto const val)
{
    return [val] (auto const lhs, auto const rhs)
    {
        return lhs >= val && rhs < val;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 3
 */
inline static auto constexpr type_3_increase = [] (auto const val)
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
auto dpld (truth_table const& table, var_change const var, F change)
    -> truth_table
{
    auto result = std::vector<int32>(table.get_vector().size());

    domain_for_each(
        table,
        [&,
         tmpElem
         = std::vector<int32>()] (auto const fFrom, auto const& elem) mutable
        {
            if (elem[as_uindex(var.index)] == var.from)
            {
                auto const varIndex  = as_uindex(var.index);
                tmpElem              = elem;
                tmpElem[varIndex]    = var.to;
                auto const fTo       = evaluate(table, tmpElem);
                auto const derValue  = change(fFrom, fTo) ? 1 : 0;
                auto const varDomain = table.get_domains()[varIndex];
                for (auto varValue = 0; varValue < varDomain; ++varValue)
                {
                    tmpElem[varIndex]   = varValue;
                    auto const derIndex = as_uindex(to_index(table, tmpElem));
                    result[derIndex]    = derValue;
                }
            }
        }
    );

    return {std::move(result), table.get_domains()};
}

/**
 *  \brief Calculates extened DPLD
 *  \param table function to derivate
 *  \param var change in the value of the variable
 *  \param d function that check whether change satisfies the derivative
 *  \return new truth table representing DPLD
 */
template<class F>
auto dpld_e (truth_table const& table, var_change const var, F change)
    -> truth_table
{
    auto result = std::vector<int32>(table.get_vector().size());

    domain_for_each(
        table,
        [&,
         index = 0U,
         tmpelem
         = std::vector<int32>()] (auto const ffrom, auto const& elem) mutable
        {
            if (elem[as_uindex(var.index)] != var.from)
            {
                result[index] = Undefined;
            }
            else
            {
                tmpelem                       = elem;
                tmpelem[as_uindex(var.index)] = var.to;
                auto const fto                = evaluate(table, tmpelem);
                result[index]                 = change(ffrom, fto) ? 1 : 0;
            }
            ++index;
        }
    );

    return {std::move(result), table.get_domains()};
}

/**
 *  \brief Calculates all MCVs for system \p state
 */
auto calculate_mcvs (truth_table const& table, int32 state)
    -> std::vector<std::vector<int32>>;

/**
 *  \brief Calculates all MPVs for system \p state
 */
auto calculate_mpvs (truth_table const& table, int32 state)
    -> std::vector<std::vector<int32>>;

/**
 *  \brief Calculates probability of the \p vector
 *  \param vector state vector
 *  \param probabilities component state probabilities
 *  \return state vector probability
 */
auto vector_probability (
    std::vector<int32> const& vector,
    std::vector<std::vector<double>> const& probabilities
) -> double;

} // namespace teddy::tsl

#endif