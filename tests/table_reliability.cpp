#include "table_reliability.hpp"
#include "truth_table_utils.hpp"

namespace teddy
{
auto probability(
    truth_table const& table, std::vector<std::vector<double>> const& ps,
    unsigned int j
) -> double
{
    auto totalprob = 0.0;

    domain_for_each(
        table,
        [j, &table, &totalprob, &ps](auto const val, auto const& elem)
        {
            if (val == j)
            {
                auto localprob = 1.0;
                for (auto i = 0u; i < table.get_var_count(); ++i)
                {
                    localprob *= ps[i][elem[i]];
                }
                totalprob += localprob;
            }
        }
    );

    return totalprob;
}

auto availability(
    truth_table const& table, std::vector<std::vector<double>> const& ps,
    unsigned int j
) -> double
{
    auto a = 0.0;
    while (j <= table.get_max_val())
    {
        a += probability(table, ps, j);
        ++j;
    }
    return a;
}

auto unavailability(
    truth_table const& table, std::vector<std::vector<double>> const& ps,
    unsigned int j
) -> double
{
    auto u = 0.0;
    while (j > 0)
    {
        --j;
        u += probability(table, ps, j);
    }
    return u;
}

auto state_frequency(truth_table const& table, unsigned int j) -> double
{
    return static_cast<double>(satisfy_count(table, j)) /
           static_cast<double>(domain_size(table));
}
} // namespace teddy