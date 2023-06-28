#include <libtsl/truth_table_reliability.hpp>

#include <algorithm>
#include <functional>
#include <numeric>
#include <ranges>
#include "libtsl/truth_table.hpp"
#include "libtsl/types.hpp"
#include <bits/ranges_algo.h>

namespace teddy::tsl
{
auto probability (
    truth_table const& table,
    std::vector<std::vector<double>> const& probabilities,
    int32 systemState
) -> double
{
    auto totalprob = 0.0;

    domain_for_each(
        table,
        [systemState, &table, &totalprob, &probabilities] (
            auto const val, auto const& elem
        )
        {
            if (val == systemState)
            {
                auto localprob = 1.0;
                for (auto i = 0; i < table.get_var_count(); ++i)
                {
                    localprob *= probabilities[as_uindex(i)]
                                              [as_uindex(elem[as_uindex(i)])];
                }
                totalprob += localprob;
            }
        }
    );

    return totalprob;
}

auto availability (
    truth_table const& table,
    std::vector<std::vector<double>> const& probabilities,
    int32 systemState
) -> double
{
    auto result = 0.0;
    while (systemState <= table.get_max_val())
    {
        result += probability(table, probabilities, systemState);
        ++systemState;
    }
    return result;
}

auto unavailability (
    truth_table const& table,
    std::vector<std::vector<double>> const& probabilities,
    int32 systemState
) -> double
{
    auto result = 0.0;
    while (systemState > 0)
    {
        --systemState;
        result += probability(table, probabilities, systemState);
    }
    return result;
}

auto state_frequency (truth_table const& table, int32 systemState) -> double
{
    return static_cast<double>(satisfy_count(table, systemState))
         / static_cast<double>(domain_size(table));
}

auto structural_importance (truth_table const& dpld, int32 componentIndex)
    -> double
{
    auto const& domains   = dpld.get_domains();
    auto const domainSize = std::reduce(
        begin(domains), end(domains), int64 {1}, std::multiplies<>()
    );
    auto const nominator
        = satisfy_count(dpld, 1) / domains[as_uindex(componentIndex)];
    auto const denominator = domainSize / domains[as_uindex(componentIndex)];
    return static_cast<double>(nominator) / static_cast<double>(denominator);
}

auto birnbaum_importance (
    truth_table const& dpld,
    std::vector<std::vector<double>> const& probabilities
) -> double
{
    return probability(dpld, probabilities, 1);
}

auto fussel_vesely_importance(
    truth_table const& structureFunction,
    std::vector<std::vector<double>> const& probabilities,
    int32 const componentIndex,
    int32 const componetnState,
    int32 const systemState
) -> double
{
    auto const allMcvs = mcvs(structureFunction, systemState);
    auto relevantMcvs = std::ranges::views::filter(allMcvs,
        [componentIndex, componetnState](std::vector<int32> const& mcv)
    {
        return mcv[as_uindex(componentIndex)] == componetnState - 1;
    });

    auto result = .0;

    domain_for_each(structureFunction,
        [&](int32 const, std::vector<int32> const& elem)
    {
        for (auto const& mcv : relevantMcvs)
        {
            if (compare(elem, mcv, std::less_equal<>()))
            {
                result += vector_probability(elem, probabilities);
                continue;
            }
        }
    });

    return result;
}

auto mcvs(truth_table const& table, int32 state) -> std::vector<std::vector<int32>>
{
    auto constexpr PiConj = [](auto const lhs, auto const rhs)
    {
        return std::min({lhs, rhs, Undefined});
    };

    auto dplds = std::vector<truth_table>();
    for (auto varIndex = 0; varIndex < table.get_var_count(); ++varIndex)
    {
        auto const varDomain = table.get_domains()[as_uindex(varIndex)];
        for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom)
        {
            auto const varChange = var_change {varIndex, varFrom, varFrom + 1};
            dplds.push_back(dpld_e(table, varChange, dpld_i_3_increase(state)));
        }
    }

    auto mcvFunction = dplds.front();
    for (auto i = 1; i < ssize(dplds); ++i)
    {
        apply_mutable(mcvFunction, dplds[as_uindex(i)], PiConj);
    }

    return satisfy_all(mcvFunction, 1);
}

auto vector_probability(
    std::vector<int32> const& vector,
    std::vector<std::vector<double>> const& probabilities
) -> double
{
    auto result = 1.;
    for (auto index = 0; index < ssize(vector); ++index)
    {
        auto const componentState = vector[as_uindex(index)];
        result *= probabilities[as_uindex(index)][as_uindex(componentState)];
    }
    return result;
}

} // namespace teddy::tsl