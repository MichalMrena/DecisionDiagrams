#include <libtsl/truth_table_reliability.hpp>

#include <numeric>
#include "libtsl/types.hpp"

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
        [systemState, &table, &totalprob, &probabilities] (auto const val, auto const& elem)
        {
            if (val == systemState)
            {
                auto localprob = 1.0;
                for (auto i = 0; i < table.get_var_count(); ++i)
                {
                    localprob
                        *= probabilities[as_uindex(i)][as_uindex(elem[as_uindex(i)])];
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

auto structural_importance (truth_table const& dpld, int32 componentIndex) -> double
{
    auto const& domains = dpld.get_domains();
    auto const domainSize
        = std::reduce(begin(domains), end(domains), int64 {1}, std::multiplies<>());
    auto const nominator   = satisfy_count(dpld, 1) / domains[as_uindex(componentIndex)];
    auto const denominator = domainSize / domains[as_uindex(componentIndex)];
    return static_cast<double>(nominator) / static_cast<double>(denominator);
}

auto birnbaum_importance (
    truth_table const& dpld, std::vector<std::vector<double>> const& probabilities
) -> double
{
    return probability(dpld, probabilities, 1);
}

} // namespace teddy::tsl