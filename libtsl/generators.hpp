#ifndef LIBTEDDY_TSL_GENERATORS_HPP
#define LIBTEDDY_TSL_GENERATORS_HPP

#include <libteddy/core.hpp>
#include <libteddy/details/probabilities.hpp>

#include <libtsl/expressions.hpp>

#include <array>
#include <random>
#include <vector>

namespace teddy::tsl
{
template<class Dat, class Deg, class Dom>
auto make_diagram (
    minmax_expr const& expr,
    diagram_manager<Dat, Deg, Dom>& manager,
    fold_type const foldtype = fold_type::Left
)
{
    auto const min_fold = [&manager, foldtype] (auto& diagrams)
    {
        return foldtype == fold_type::Left
                 ? manager.template left_fold<ops::MIN>(diagrams)
                 : manager.template tree_fold<ops::MIN>(diagrams);
    };

    auto const max_fold = [&manager, foldtype] (auto& diagrams)
    {
        return foldtype == fold_type::Left
                 ? manager.template left_fold<ops::MAX>(diagrams)
                 : manager.template tree_fold<ops::MAX>(diagrams);
    };

    using diagram_t = typename diagram_manager<Dat, Deg, Dom>::diagram_t;
    std::vector<diagram_t> termDs;
    for (auto const& eTerm : expr.terms_)
    {
        auto vars = manager.variables(eTerm);
        termDs.push_back(min_fold(vars));
    }
    return max_fold(termDs);
}

template<class Dat, class Deg, class Dom>
auto make_diagram (
    std::unique_ptr<tsl::expr_node> const& expr,
    diagram_manager<Dat, Deg, Dom>& manager
)
{
    return manager.from_expression_tree(*expr);
}

template<class Dat, class Deg, class Dom>
auto make_diagram (
    tsl::expr_node const& expr,
    diagram_manager<Dat, Deg, Dom>& manager
)
{
    return manager.from_expression_tree(expr);
}

inline auto make_probability_vector (int32 const varCount, std::ranlux48& rng)
    -> std::vector<double>
{
    std::vector<double> probs(as_usize(varCount));
    for (int32 i = 0; i < ssize(probs); ++i)
    {
        std::uniform_real_distribution<double> dist(.0, 1.0);
        probs[as_uindex(i)] = dist(rng);
    }
    return probs;
}

template<std::size_t M>
auto make_probability_matrix (
    std::vector<int32> const& domains,
    std::ranlux48& rng
) -> std::vector<std::array<double, M>>
{
    std::vector<std::array<double, M>> probs(domains.size());
    for (int32 i = 0; i < ssize(probs); ++i)
    {
        std::uniform_real_distribution<double> dist(.0, 1.0);
        for (int32 j = 0; j < domains[as_uindex(i)]; ++j)
        {
            probs[as_uindex(i)][as_uindex(j)] = dist(rng);
        }
        double const sum = std::reduce(
            begin(probs[as_uindex(i)]),
            end(probs[as_uindex(i)]),
            0.0,
            std::plus<>()
        );
        for (int32 j = 0; j < domains[as_uindex(i)]; ++j)
        {
            probs[as_uindex(i)][as_uindex(j)] /= sum;
        }
    }
    return probs;
}

inline auto make_time_probability_vector (
    int32 const varCount,
    std::ranlux48& rng
) -> std::vector<probs::prob_dist>
{
    std::vector<probs::prob_dist> probs;

    auto const mkExponential = [] (std::ranlux48& gen) -> probs::prob_dist
    {
        std::uniform_real_distribution<double> distRate(0.5, 1.5);
        return probs::prob_dist(probs::exponential(distRate(gen)));
    };

    auto const mkWeibull = [] (std::ranlux48& gen) -> probs::prob_dist
    {
        std::uniform_real_distribution<double> distShape(0.5, 1.0);
        std::uniform_real_distribution<double> distScale(0.9, 1.0);
        return probs::prob_dist(probs::weibull(distScale(gen), distShape(gen)));
    };

    auto const mkConstant = [] (std::ranlux48& gen) -> probs::prob_dist
    {
        std::uniform_real_distribution<double> distProb(0.2, 1.0);
        return probs::prob_dist(probs::constant(distProb(gen)));
    };

    std::vector<probs::prob_dist (*)(std::ranlux48&)> distGenerators(
        {+mkExponential, +mkWeibull, +mkConstant}
    );

    std::uniform_int_distribution<std::size_t> distGen(
        std::size_t {0},
        distGenerators.size() - 1
    );
    for (int i = 0; i < varCount; ++i)
    {
        auto const gen = distGenerators[distGen(rng)];
        probs.push_back(gen(rng));
    }

    return probs;
}

inline auto make_time_symprobability_vector (
    int32 const varCount,
    std::ranlux48& rng
) -> std::vector<symprobs::expression>
{
    std::vector<symprobs::expression> probs;

    auto const mkExponential = [] (std::ranlux48& gen) -> symprobs::expression
    {
        std::uniform_real_distribution<double> distRate(0.5, 1.5);
        return symprobs::exponential(distRate(gen));
    };

    auto const mkWeibull = [] (std::ranlux48& gen) -> symprobs::expression
    {
        std::uniform_real_distribution<double> distShape(0.5, 1.0);
        return symprobs::weibull(1.0, distShape(gen));
    };

    auto const mkConstant = [] (std::ranlux48& gen) -> symprobs::expression
    {
        std::uniform_real_distribution<double> distProb(0.2, 1.0);
        return symprobs::constant(distProb(gen));
    };

    std::vector<symprobs::expression (*)(std::ranlux48&)> distGenerators(
        {+mkExponential, +mkWeibull, +mkConstant}
    );

    std::uniform_int_distribution<std::size_t> distGen(
        std::size_t {0},
        distGenerators.size() - 1
    );
    for (int i = 0; i < varCount; ++i)
    {
        auto const gen = distGenerators[distGen(rng)];
        probs.push_back(gen(rng));
    }

    return probs;
}
} // namespace teddy::tsl

#endif