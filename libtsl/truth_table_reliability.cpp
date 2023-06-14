#include <libtsl/truth_table_reliability.hpp>
#include <numeric>

namespace teddy::tsl
{
auto probability(
    truth_table const& table,
    std::vector<std::vector<double>> const& ps,
    int32 j
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
                for (auto i = 0; i < table.get_var_count(); ++i)
                {
                    localprob *= ps[as_uindex(i)][as_uindex(elem[as_uindex(i)])];
                }
                totalprob += localprob;
            }
        }
    );

    return totalprob;
}

auto availability(
    truth_table const& table,
    std::vector<std::vector<double>> const& ps,
    int32 j
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
    truth_table const& table,
    std::vector<std::vector<double>> const& ps,
    int32 j
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

auto state_frequency(truth_table const& table, int32 j) -> double
{
    return static_cast<double>(satisfy_count(table, j)) /
           static_cast<double>(domain_size(table));
}

auto structural_importance(truth_table const& dpld, int32 i) -> double
{
    auto const& ds = dpld.get_domains();
    auto const domainsize = std::reduce(
        begin(ds),
        end(ds),
        int64{1},
        std::multiplies<>()
    );
    auto const nominator = satisfy_count(dpld, 1);
    auto const denominator = domainsize / ds[as_uindex(i)];
    return static_cast<double>(nominator) / static_cast<double>(denominator);
}

auto birnbaum_importance(
    truth_table const& dpld,
    std::vector<std::vector<double>> const& ps
) -> double
{
    return probability(dpld, ps, 1);
}

} // namespace teddy