#ifndef LIBTEDDY_TESTS_CORE_SETTUP_HPP
#define LIBTEDDY_TESTS_CORE_SETTUP_HPP

#include "expressions.hpp"
#include <libteddy/teddy.hpp>
#include <random>
#include <variant>
#include <vector>

namespace teddy
{
/**
 *  \brief Specifies that order should be randomly generated.
 */
struct random_order_tag
{
};

/**
 *  \brief Explicitly gives the order.
 */
struct given_order
{
    std::vector<index_t> order_;
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
struct given_domains
{
    std::vector<index_t> domains_;
};

/**
 *  \brief Settings common for all managers.
 */
struct manager_settings
{
    uint_t varcount_;
    uint_t nodecount_;
    std::variant<random_order_tag, given_order> order_;
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
template<uint_t M>
struct mdd_manager_settings : manager_settings
{
};

/**
 *  \brief Describes how to initialize imdd_manager.
 */
template<uint_t M>
struct imdd_manager_settings : manager_settings
{
    std::variant<random_domains_tag, given_domains> domains_;
};

/**
 *  \brief Describes how to initialize a ifmdd_manager.
 */
template<uint_t M>
struct ifmdd_manager_settings : manager_settings
{
    std::variant<random_domains_tag, given_domains> domains_;
};

/**
 *  \brief Settings for generation of minmax expression.
 */
struct minmax_expression_settings
{
    uint_t termcount_;
    uint_t termsize_;
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

inline auto make_order(manager_settings const& s, std::mt19937_64& rng)
    -> std::vector<index_t>
{
    return std::visit(
        match {
            [&](random_order_tag)
            {
                auto is = utils::fill_vector(s.varcount_, utils::identity);
                std::ranges::shuffle(is, rng);
                return is;
            },
            [](given_order const& is)
            {
                return is.order_;
            }},
        s.order_
    );
}

/**
 *  \brief Makes order of variables for a manager.
 */
template<uint_t M>
auto make_domains(
    uint_t const varcount,
    std::variant<random_domains_tag, given_domains> const& s,
    std::mt19937_64& rng
) -> std::vector<index_t>
{
    return std::visit(
        match {
            [&](random_domains_tag)
            {
                auto dist = std::uniform_int_distribution<uint_t>(2u, M - 1);
                return utils::fill_vector(
                    varcount,
                    [&rng, &dist](auto)
                    {
                        return dist(rng);
                    }
                );
            },
            [](given_domains const& ds)
            {
                return ds.domains_;
            }},
        s
    );
}

/**
 *  \brief Makes domains for a manager.
 */
template<uint_t M>
auto make_domains(imdd_manager_settings<M> const& s, std::mt19937_64& rng)
    -> std::vector<index_t>
{
    return make_domains<M>(s.varcount_, s.domains_, rng);
}

/**
 *  \brief Makes domains for a manager.
 */
template<uint_t M>
auto make_domains(ifmdd_manager_settings<M> const& s, std::mt19937_64& rng)
    -> std::vector<index_t>
{
    return make_domains<M>(s.varcount_, s.domains_, rng);
}

/**
 *  \brief Creates BDD manager.
 */
inline auto create_manager(bdd_manager_settings const& s, std::mt19937_64& rng)
    -> bdd_manager
{
    return bdd_manager(s.varcount_, s.nodecount_, make_order(s, rng));
}

/**
 *  \brief Creates MDD manager.
 */
template<uint_t M>
auto create_manager(mdd_manager_settings<M> const& s, std::mt19937_64& rng)
    -> mdd_manager<M>
{
    return mdd_manager<M>(s.varcount_, s.nodecount_, make_order(s, rng));
}

/**
 *  \brief Creates iMDD manager.
 */
template<uint_t M>
auto create_manager(imdd_manager_settings<M> const& s, std::mt19937_64& rng)
    -> imdd_manager
{
    return imdd_manager(
        s.varcount_, s.nodecount_, make_domains(s, rng), make_order(s, rng)
    );
}

/**
 *  \brief Creates ifMDD manager.
 */
template<uint_t M>
auto create_manager(ifmdd_manager_settings<M> const& s, std::mt19937_64& rng)
    -> ifmdd_manager<M>
{
    return ifmdd_manager<M>(
        s.varcount_, s.nodecount_, make_domains(s, rng), make_order(s, rng)
    );
}

/**
 *  \brief Creates manager for a test.
 */
template<class Man, class Expr>
auto create_manager(test_settings<Man, Expr> const& s, std::mt19937_64& rng)
{
    return create_manager(s.manager_, rng);
}

/**
 *  \brief Creates diagram representing \p expr .
 */
template<class Dat, class Deg, class Dom>
auto create_diagram(
    minmax_expr const& expr, diagram_manager<Dat, Deg, Dom>& manager,
    fold_type const foldtype = fold_type::Left
)
{
    auto const min_fold = [&manager, foldtype](auto& xs)
    {
        return foldtype == fold_type::Left
                   ? manager.template left_fold<ops::MIN>(xs)
                   : manager.template tree_fold<ops::MIN>(xs);
    };

    auto const max_fold = [&manager, foldtype](auto& xs)
    {
        return foldtype == fold_type::Left
                   ? manager.template left_fold<ops::MAX>(xs)
                   : manager.template tree_fold<ops::MAX>(xs);
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
 *  \brief Creates minmax expression with given settings.
 */
inline auto create_expression(
    uint_t const varcount, minmax_expression_settings const& s,
    std::mt19937_64& rng
) -> minmax_expr
{
    return generate_minmax_expression(rng, varcount, s.termcount_, s.termsize_);
}

/**
 *  \brief Creates expression for a test.
 */
template<class Man, class Expr>
auto create_expression(test_settings<Man, Expr> const& s, std::mt19937_64& rng)
    -> minmax_expr
{
    return create_expression(s.manager_.varcount_, s.expression_, rng);
}
} // namespace teddy

#endif