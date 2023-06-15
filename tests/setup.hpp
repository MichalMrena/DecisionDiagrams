#ifndef LIBTEDDY_COMMON_TEST_SETUP_HPP
#define LIBTEDDY_COMMON_TEST_SETUP_HPP

#include <libteddy/core.hpp>
#include <libteddy/reliability.hpp>

#include <libtsl/expressions.hpp>
#include <libtsl/iterators.hpp>

#include <functional>
#include <iterator>
#include <numeric>
#include <random>
#include <variant>
#include <vector>

#include <librog/rog.hpp>

namespace teddy::tests
{
/**
 *  \brief Specifies that order should be randomly generated.
 */
struct random_order_tag
{
};

/**
 *  \brief Specifies that order should simply follow the indices.
 */
struct default_order_tag
{
};

/**
 *  \brief Explicitly gives the order.
 */
struct given_order_tag
{
    std::vector<int32> order_;
};

/**
 *  \brief Specifies that domains should be randomly generated.
 */
struct random_domains_tag
{
};

/**
 *  \brief Explicitly gives the domains.
 */
struct given_domains_tag
{
    std::vector<int32> domains_;
};

/**
 *  \brief Settings common for all managers.
 */
struct manager_settings
{
    int32 varcount_;
    int32 nodecount_;
    std::variant<random_order_tag, default_order_tag, given_order_tag> order_;
};

/**
 *  \brief Settings common for managers of nonhomogeneous functions / systems.
 */
template<int32 M>
struct nonhomogeneous_manager_settings : manager_settings
{
    std::variant<random_domains_tag, given_domains_tag> domains_;
};

/**
 *  \brief Describes how to initialize a bdd_manager.
 */
struct bdd_manager_settings : manager_settings
{
};

/**
 *  \brief Describes how to initialize a mdd_manager.
 */
template<int32 M>
struct mdd_manager_settings : manager_settings
{
};

/**
 *  \brief Describes how to initialize imdd_manager.
 */
template<int32 M>
struct imdd_manager_settings : nonhomogeneous_manager_settings<M>
{
};

/**
 *  \brief Describes how to initialize a ifmdd_manager.
 */
template<int32 M>
struct ifmdd_manager_settings : nonhomogeneous_manager_settings<M>
{
};

/**
 *  \brief Describes how to initialize a bss_manager.
 */
struct bss_manager_settings : manager_settings
{
};

/**
 *  \brief Describes how to initialize a mss_manager.
 */
template<int32 M>
struct mss_manager_settings : manager_settings
{
};

/**
 *  \brief Describes how to initialize imss_manager.
 */
template<int32 M>
struct imss_manager_settings : nonhomogeneous_manager_settings<M>
{
};

/**
 *  \brief Describes how to initialize a ifmss_manager.
 */
template<int32 M>
struct ifmss_manager_settings : nonhomogeneous_manager_settings<M>
{
};

/**
 *  \brief Settings for generation of minmax expression.
 */
struct minmax_expression_settings
{
    int32 varcount_;
    int32 termcount_;
    int32 termsize_;
};

/**
 *  \brief Settings for generations of expression tree.
 *  (for now, it only servses as a tag)
 */
struct expression_tree_settings
{
    int32 varcount_;
};

/**
 *  \brief Settings used for most of the tests.
 */
template<class ManagerSettings, class ExpressionSettings>
struct test_settings
{
    std::size_t seed_;
    ManagerSettings manager_;
    ExpressionSettings expression_;
};

/**
 *  \brief Helper type to implement wannabe pattern matching.
 */
template<class... Ts>
struct match : Ts...
{
    using Ts::operator()...;
};

template<class... Ts>
match(Ts...) -> match<Ts...>;

inline auto make_order (manager_settings const& settings, std::mt19937_64& rng)
    -> std::vector<int32>
{
    return std::visit(
        match {
            [&] (random_order_tag)
            {
                auto indices
                    = utils::fill_vector(settings.varcount_, utils::identity);
                std::ranges::shuffle(indices, rng);
                return indices;
            },
            [&] (default_order_tag)
            {
                auto indices
                    = utils::fill_vector(settings.varcount_, utils::identity);
                return indices;
            },
            [] (given_order_tag const& indices)
            {
                return indices.order_;
            }},
        settings.order_
    );
}

/**
 *  \brief Makes domains for a manager.
 */
template<int32 M>
auto make_domains (
    nonhomogeneous_manager_settings<M> const& settings, std::mt19937_64& rng
) -> std::vector<int32>
{
    return std::visit(
        match {
            [&] (random_domains_tag)
            {
                auto dist = std::uniform_int_distribution<int32>(2, M);
                return utils::fill_vector(
                    settings.varcount_,
                    [&rng, &dist] (auto)
                    {
                        return dist(rng);
                    }
                );
            },
            [] (given_domains_tag const& tag)
            {
                return tag.domains_;
            }},
        settings.domains_
    );
}

/**
 *  \brief Makes BDD manager.
 */
inline auto make_manager (
    bdd_manager_settings const& settings, std::mt19937_64& rng
) -> bdd_manager
{
    return {settings.varcount_, settings.nodecount_, make_order(settings, rng)};
}

/**
 *  \brief Makes MDD manager.
 */
template<int32 M>
auto make_manager (
    mdd_manager_settings<M> const& settings, std::mt19937_64& rng
) -> mdd_manager<M>
{
    return {settings.varcount_, settings.nodecount_, make_order(settings, rng)};
}

/**
 *  \brief Makes iMDD manager.
 */
template<int32 M>
auto make_manager (
    imdd_manager_settings<M> const& settings, std::mt19937_64& rng
) -> imdd_manager
{
    return {
        settings.varcount_,
        settings.nodecount_,
        make_domains(settings, rng),
        make_order(settings, rng)};
}

/**
 *  \brief Makes ifMDD manager.
 */
template<int32 M>
auto make_manager (
    ifmdd_manager_settings<M> const& settings, std::mt19937_64& rng
) -> ifmdd_manager<M>
{
    return {
        settings.varcount_,
        settings.nodecount_,
        make_domains(settings, rng),
        make_order(settings, rng)};
}

/**
 *  \brief Makes bss_manager.
 */
inline auto make_manager (
    bss_manager_settings const& settings, std::mt19937_64& rng
) -> bss_manager
{
    return {settings.varcount_, settings.nodecount_, make_order(settings, rng)};
}

/**
 *  \brief Makes mss_manager.
 */
template<int32 M>
auto make_manager (
    mss_manager_settings<M> const& settings, std::mt19937_64& rng
) -> mss_manager<M>
{
    return {settings.varcount_, settings.nodecount_, make_order(settings, rng)};
}

/**
 *  \brief Makes imss_manager.
 */
template<int32 M>
auto make_manager (
    imss_manager_settings<M> const& settings, std::mt19937_64& rng
) -> imss_manager
{
    return imss_manager(
        settings.varcount_,
        settings.nodecount_,
        make_domains(settings, rng),
        make_order(settings, rng)
    );
}

/**
 *  \brief Makes ifmss_manager.
 */
template<int32 M>
auto make_manager (
    ifmss_manager_settings<M> const& settings, std::mt19937_64& rng
) -> ifmss_manager<M>
{
    return ifmss_manager<M>(
        settings.varcount_,
        settings.nodecount_,
        make_domains(settings, rng),
        make_order(settings, rng)
    );
}

/**
 *  \brief Makes manager for a test.
 */
template<class Man, class Expr>
auto make_manager (
    test_settings<Man, Expr> const& settings, std::mt19937_64& rng
)
{
    return make_manager(settings.manager_, rng);
}

/**
 *  \brief Makes diagram representing \p expr .
 */
template<class Dat, class Deg, class Dom>
auto make_diagram (
    tsl::minmax_expr const& expr,
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
    auto termDs     = std::vector<diagram_t>();
    for (auto const& eTerm : expr.terms_)
    {
        auto vars = manager.variables(eTerm);
        termDs.emplace_back(min_fold(vars));
    }
    return max_fold(termDs);
}

/**
 *  \brief Makes diagram representing \p expr .
 */
template<class Dat, class Deg, class Dom>
auto make_diagram (
    std::unique_ptr<tsl::expr_node> const& expr,
    diagram_manager<Dat, Deg, Dom>& manager
)
{
    return manager.from_expression_tree(*expr);
}

/**
 *  \brief Makes minmax expression with given settings.
 */
inline auto make_expression (
    minmax_expression_settings const& settings, std::mt19937_64& rng
) -> tsl::minmax_expr
{
    return tsl::make_minmax_expression(
        rng, settings.varcount_, settings.termcount_, settings.termsize_
    );
}

/**
 *  \brief Makes expression tree with given settings.
 */
inline auto make_expression (
    expression_tree_settings const& settings, std::mt19937_64& rng
) -> std::unique_ptr<tsl::expr_node>
{
    return tsl::make_expression_tree(settings.varcount_, rng, rng);
}

/**
 *  \brief Makes expression for a test.
 */
template<class Man, class Expr>
auto make_expression (
    test_settings<Man, Expr> const& settings, std::mt19937_64& rng
)
{
    return make_expression(
        settings.manager_.varcount_, settings.expression_, rng
    );
}

/**
 *  \brief Makes random component state probabilities
 */
template<class Dat, class Deg, class Dom>
auto make_probabilities (
    diagram_manager<Dat, Deg, Dom> const& manager, std::mt19937_64& rng
) -> std::vector<std::vector<double>>
{
    auto const domains = manager.get_domains();
    auto probs
        = std::vector<std::vector<double>>(as_usize(manager.get_var_count()));
    for (auto i = 0; i < ssize(probs); ++i)
    {
        auto dist = std::uniform_real_distribution<double>(.0, 1.0);
        probs[as_uindex(i)].resize(as_usize(domains[as_uindex(i)]));
        for (auto j = 0; j < domains[as_uindex(i)]; ++j)
        {
            probs[as_uindex(i)][as_uindex(j)] = dist(rng);
        }
        auto const sum = std::reduce(
            begin(probs[as_uindex(i)]),
            end(probs[as_uindex(i)]),
            0.0,
            std::plus<>()
        );
        for (auto j = 0; j < domains[as_uindex(i)]; ++j)
        {
            probs[as_uindex(i)][as_uindex(j)] /= sum;
        }
    }
    return probs;
}

inline auto make_vector (
    std::unique_ptr<tsl::expr_node> const& root,
    std::vector<int32> const& domains
) -> std::vector<int32>
{
    auto vector = std::vector<int32>();
    vector.reserve(as_usize(
        std::reduce(begin(domains), end(domains), 1, std::multiplies<>())
    ));
    auto domainit = tsl::domain_iterator(domains);
    auto evalit   = tsl::evaluating_iterator(domainit, *root);
    auto evalend  = tsl::evaluating_iterator_sentinel();
    auto output   = std::back_inserter(vector);
    while (evalit != evalend)
    {
        *output++ = *evalit++;
    }
    return vector;
}

template<class Dat, class Deg, class Dom>
auto make_domain_iterator (diagram_manager<Dat, Deg, Dom> const& manager)
{
    return tsl::domain_iterator(manager.get_domains(), manager.get_order());
}

/**
 *  \brief Holds data common for all tests.
 *  Each test uses some settings and random generator.
 */
template<class Settings>
class test_base : public rog::LeafTest
{
public:
    test_base(std::string name, Settings settings) :
        rog::LeafTest(std::move(name), rog::AssertPolicy::RunAll),
        settings_(std::move(settings)),
        rng_(settings_.seed_)
    {
    }

protected:
    auto settings () const -> Settings const&
    {
        return settings_;
    }

    auto rng () -> std::mt19937_64&
    {
        return rng_;
    }

private:
    Settings settings_;
    std::mt19937_64 rng_;
};

} // namespace teddy::tests

#endif