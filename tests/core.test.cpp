#include <libteddy/core.hpp>

#include <libtsl/expressions.hpp>
#include <libtsl/iterators.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

#include <fmt/core.h>

#include <concepts>
#include <cstddef>

#include "libteddy/reliability.hpp"
#include "setup.hpp"

namespace teddy::tests
{
/**
 *  \brief Fixture base
 */
template<class ManagerSettings, class ExpressionSettings>
struct fixture_base
{
    ManagerSettings managerSettings_;
    ExpressionSettings expressionSettings_;
    std::mt19937_64 rng_;
};

/**
 *  \brief BDD fixture base
 */
struct bdd_fixture :
    fixture_base<bdd_manager_settings, minmax_expression_settings>
{
private:
    inline static auto constexpr VarCount  = 15;
    inline static auto constexpr NodeCount = 5'000;
    inline static auto constexpr TermCount = 20;
    inline static auto constexpr TermSize  = 5;
    inline static auto constexpr Seed      = 911;

public:
    bdd_fixture() :
        fixture_base<bdd_manager_settings, minmax_expression_settings> {
            {bdd_manager_settings {VarCount, NodeCount, random_order_tag()}},
            {minmax_expression_settings {VarCount, TermCount, TermSize}},
            {std::mt19937_64(Seed)}}
    {
    }
};

/**
 *  \brief MDD fixture base
 */
struct mdd_fixture :
    fixture_base<mdd_manager_settings<3>, minmax_expression_settings>
{
private:
    inline static auto constexpr VarCount  = 15;
    inline static auto constexpr NodeCount = 5'000;
    inline static auto constexpr TermCount = 20;
    inline static auto constexpr TermSize  = 5;
    inline static auto constexpr Seed      = 911;

public:
    mdd_fixture() :
        fixture_base<mdd_manager_settings<3>, minmax_expression_settings> {
            {mdd_manager_settings<3> {VarCount, NodeCount, random_order_tag()}},
            {minmax_expression_settings {VarCount, TermCount, TermSize}},
            {std::mt19937_64(Seed)}}
    {
    }
};

/**
 *  \brief iMDD fixture base
 */
struct imdd_fixture :
    fixture_base<imdd_manager_settings<3>, minmax_expression_settings>
{
private:
    inline static auto constexpr VarCount  = 15;
    inline static auto constexpr NodeCount = 5'000;
    inline static auto constexpr TermCount = 20;
    inline static auto constexpr TermSize  = 5;
    inline static auto constexpr Seed      = 911;

public:
    imdd_fixture() :
        fixture_base<imdd_manager_settings<3>, minmax_expression_settings> {
            {imdd_manager_settings<3> {
                {{VarCount, NodeCount, random_order_tag()},
                 random_domains_tag()}}},
            minmax_expression_settings {VarCount, TermCount, TermSize},
            {std::mt19937_64(Seed)}
    }
    {
    }
};

/**
 *  \brief ifMDD fixture base
 */
struct ifmdd_fixture :
    fixture_base<ifmdd_manager_settings<3>, minmax_expression_settings>
{
private:
    inline static auto constexpr VarCount  = 15;
    inline static auto constexpr NodeCount = 5'000;
    inline static auto constexpr TermCount = 20;
    inline static auto constexpr TermSize  = 5;
    inline static auto constexpr Seed      = 911;

public:
    ifmdd_fixture() :
        fixture_base<ifmdd_manager_settings<3>, minmax_expression_settings> {
            {ifmdd_manager_settings<3> {
                {{VarCount, NodeCount, random_order_tag()},
                 random_domains_tag()}}},
            {minmax_expression_settings {VarCount, TermCount, TermSize}},
            {std::mt19937_64(Seed)}}
    {
    }
};

/**
 *  \brief Calculates frequency table for each possible value of \p expr .
 */
template<class Dat, class Deg, class Dom>
auto expected_counts (
    diagram_manager<Dat, Deg, Dom>& manager,
    tsl::minmax_expr const& expr
)
{
    auto counts   = std::vector<int64>();
    auto domainIt = tsl::domain_iterator(manager.get_domains());
    auto evalIt   = tsl::evaluating_iterator(domainIt, expr);
    auto evalEnd  = tsl::evaluating_iterator_sentinel();
    while (evalIt != evalEnd)
    {
        auto const value = *evalIt;
        if (value >= ssize(counts))
        {
            counts.resize(as_usize(value + 1), 0);
        }
        ++counts[as_uindex(value)];
        ++evalIt;
    }
    return counts;
}

/**
 *  \brief Compares diagram output with \p evalIt for each possible input
 */
template<class Expression, class Dat, class Deg, class Dom>
auto test_compare_eval (
    tsl::evaluating_iterator<Expression> evalIt,
    diagram_manager<Dat, Deg, Dom>& manager,
    auto& diagram
) -> void
{
    auto evalEnd = tsl::evaluating_iterator_sentinel();
    while (evalIt != evalEnd)
    {
        auto const expectedVal = *evalIt;
        auto const diagramVal
            = manager.evaluate(diagram, evalIt.get_var_vals());
        BOOST_REQUIRE_EQUAL(expectedVal, diagramVal);
        ++evalIt;
    }
}

using Fixtures = boost::mpl::vector<
    teddy::tests::bdd_fixture,
    teddy::tests::mdd_fixture,
    teddy::tests::imdd_fixture,
    teddy::tests::ifmdd_fixture
>;

BOOST_TEST_DECORATOR(* boost::unit_test::disabled())
BOOST_AUTO_TEST_SUITE(core_test)

BOOST_FIXTURE_TEST_CASE_TEMPLATE(evaluate, Fixture, Fixtures, Fixture)
{
    auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram = make_diagram(expr, manager);
    BOOST_TEST_MESSAGE(fmt::format("Node count {}", manager.get_node_count(diagram))
    );
    auto domainit = make_domain_iterator(manager);
    auto evalit   = teddy::tsl::evaluating_iterator(domainit, expr);
    test_compare_eval(evalit, manager, diagram);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(fold, Fixture, Fixtures, Fixture)
{
    auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram1 = make_diagram(expr, manager, fold_type::Left);
    auto diagram2 = make_diagram(expr, manager, fold_type::Tree);
    BOOST_TEST_MESSAGE(
        fmt::format("Node count {}", manager.get_node_count(diagram1))
    );
    BOOST_REQUIRE(diagram1.equals(diagram2));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(gc, Fixture, Fixtures, Fixture)
{
    auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram1 = make_diagram(expr, manager, fold_type::Left);
    auto diagram2 = make_diagram(expr, manager, fold_type::Tree);
    BOOST_TEST_MESSAGE(
        fmt::format("Node count {}", manager.get_node_count(diagram1))
    );
    manager.force_gc();
    auto const expected = manager.get_node_count(diagram1);
    auto const actual   = manager.get_node_count();
    BOOST_REQUIRE_EQUAL(expected, actual);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(satisfy_count, Fixture, Fixtures, Fixture)
{
    auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram = make_diagram(expr, manager);
    BOOST_TEST_MESSAGE(fmt::format("Node count {}", manager.get_node_count(diagram))
    );
    auto expected = expected_counts(manager, expr);
    auto actual   = std::vector<int64>(expected.size(), 0);

    for (auto j = 0; j < ssize(actual); ++j)
    {
        if constexpr (std::same_as<decltype(manager), teddy::bss_manager>)
        {
            actual[as_uindex(j)] = manager.satisfy_count(diagram);
        }
        else
        {
            actual[as_uindex(j)] = manager.satisfy_count(j, diagram);
        }
    }

    for (auto k = 0; k < ssize(actual); ++k)
    {
        BOOST_REQUIRE_EQUAL(actual[as_uindex(k)], expected[as_uindex(k)]);
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(satisfy_all, Fixture, Fixtures, Fixture)
{
    auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram = make_diagram(expr, manager);
    BOOST_TEST_MESSAGE(fmt::format("Node count {}", manager.get_node_count(diagram))
    );
    auto expected = expected_counts(manager, expr);
    auto actual   = std::vector<int64>(expected.size(), 0);
    for (auto k = 0; k < ssize(expected); ++k)
    {
        using out_var_vals = std::vector<int32>;
        auto outf          = [&actual, k] (auto const&)
        {
            ++actual[as_uindex(k)];
        };
        auto out = tsl::forwarding_iterator<decltype(outf)>(outf);
        if constexpr (std::same_as<decltype(manager), teddy::bss_manager>)
        {
            manager.template satisfy_all_g<out_var_vals>(diagram, out);
        }
        else
        {
            manager.template satisfy_all_g<out_var_vals>(k, diagram, out);
        }
    }

    for (auto k = 0; k < ssize(actual); ++k)
    {
        BOOST_REQUIRE_EQUAL(actual[as_uindex(k)], expected[as_uindex(k)]);
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(operators, Fixture, Fixtures, Fixture)
{
    using namespace teddy::ops;
    auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram = make_diagram(expr, manager);
    BOOST_TEST_MESSAGE(fmt::format("Node count {}", manager.get_node_count(diagram))
    );
    auto const zero = manager.constant(0);
    auto const one  = manager.constant(1);
    auto const sup  = manager.constant(std::ranges::max(manager.get_domains()) - 1);
    auto const boolValDiagram = manager.transform(diagram, utils::not_zero);

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<AND>(boolValDiagram, zero).equals(zero),
        "AND absorbing"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<AND>(boolValDiagram, one).equals(boolValDiagram),
        "AND neutral"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<OR>(boolValDiagram, one).equals(one),
        "OR absorbing"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<OR>(boolValDiagram, zero).equals(boolValDiagram),
        "OR neutral"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<XOR>(boolValDiagram, boolValDiagram)
            .equals(zero),
        "XOR annihilate"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<MULTIPLIES<2>>(boolValDiagram, zero)
            .equals(zero),
        "MULTIPLIES absorbing"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<MULTIPLIES<4>>(boolValDiagram, one)
            .equals(boolValDiagram),
        "MULTIPLIES neutral"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<PLUS<4>>(boolValDiagram, zero)
            .equals(boolValDiagram),
        "PLUS neutral"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<EQUAL_TO>(boolValDiagram, boolValDiagram)
            .equals(one),
        "EQUAL_TO annihilate"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<NOT_EQUAL_TO>(boolValDiagram, boolValDiagram)
            .equals(zero),
        "NOT_EQUAL_TO annihilate"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<LESS>(boolValDiagram, boolValDiagram)
            .equals(zero),
        "LESS annihilate"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<GREATER>(boolValDiagram, boolValDiagram)
            .equals(zero),
        "GREATER annihilate"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<LESS_EQUAL>(boolValDiagram, boolValDiagram)
            .equals(one),
        "LESS_EQUAL annihilate"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<GREATER_EQUAL>(boolValDiagram, boolValDiagram)
            .equals(one),
        "GREATER_EQUAL annihilate"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<MIN>(boolValDiagram, zero).equals(zero),
        "MIN absorbing"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<MIN>(boolValDiagram, sup).equals(boolValDiagram),
        "MIN neutral"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<MAX>(boolValDiagram, sup).equals(sup),
        "MAX absoring"
    );

    BOOST_REQUIRE_MESSAGE(
        manager.template apply<MAX>(boolValDiagram, zero)
            .equals(boolValDiagram),
        "MAX neutral"
    );
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(cofactor, Fixture, Fixtures, Fixture)
{
    auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram = make_diagram(expr, manager);
    BOOST_TEST_MESSAGE(fmt::format("Node count {}", manager.get_node_count(diagram))
    );
    auto const maxIndex = manager.get_var_count() - 1;
    auto indexDist      = std::uniform_int_distribution<int32>(0, maxIndex);
    auto const index1   = indexDist(Fixture::rng_);
    auto const index2   = [this, &indexDist, index1] ()
    {
        for (;;)
        {
            // I know ... but should be ok...
            auto const randomIndex = indexDist(Fixture::rng_);
            if (randomIndex != index1)
            {
                return randomIndex;
            }
        }
    }();
    auto const value1              = int32 {0};
    auto const value2              = int32 {1};
    auto const intermediateDiagram = manager.get_cofactor(diagram, index1, value1);
    auto const cofactoredDiagram
        = manager.get_cofactor(intermediateDiagram, index2, value2);

    auto domainIt = tsl::domain_iterator(
        manager.get_domains(),
        manager.get_order(),
        {std::make_pair(index1, value1), std::make_pair(index2, value2)}
    );
    auto evalIt = tsl::evaluating_iterator(domainIt, expr);
    test_compare_eval(evalIt, manager, diagram);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(one_var_sift, Fixture, Fixtures, Fixture)
{
    auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram = make_diagram(expr, manager);
    BOOST_TEST_MESSAGE(fmt::format("Node count {}", manager.get_node_count(diagram))
    );
    manager.force_gc();
    manager.force_reorder();
    manager.force_gc();
    auto const actual   = manager.get_node_count();
    auto const expected = manager.get_node_count(diagram);
    BOOST_TEST_MESSAGE(fmt::format("Node count after {}", actual));
    BOOST_REQUIRE_EQUAL(expected, actual);
    auto domainit = make_domain_iterator(manager);
    auto evalit   = tsl::evaluating_iterator(domainit, expr);
    test_compare_eval(evalit, manager, diagram);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(auto_var_sift, Fixture, Fixtures, Fixture)
{
    auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    manager.set_auto_reorder(true);
    auto diagram = make_diagram(expr, manager);
    manager.force_gc();
    auto const actual   = manager.get_node_count();
    auto const expected = manager.get_node_count(diagram);
    BOOST_REQUIRE_EQUAL(expected, actual);
    auto domainit = make_domain_iterator(manager);
    auto evalit   = tsl::evaluating_iterator(domainit, expr);
    test_compare_eval(evalit, manager, diagram);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(from_vector, Fixture, Fixtures, Fixture)
{
    auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram = make_diagram(expr, manager);
    BOOST_TEST_MESSAGE(fmt::format("Node count {}", manager.get_node_count(diagram))
    );
    auto domainit = make_domain_iterator(manager);
    auto evalit   = tsl::evaluating_iterator(domainit, expr);
    auto evalend  = tsl::evaluating_iterator_sentinel();
    auto vectord  = manager.from_vector(evalit, evalend);
    BOOST_REQUIRE_MESSAGE(
        diagram.equals(vectord),
        "From-vector created the same diagram"
    );
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(to_vector, Fixture, Fixtures, Fixture)
{
    auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram = make_diagram(expr, manager);
    BOOST_TEST_MESSAGE(fmt::format("Node count {}", manager.get_node_count(diagram))
    );
    auto vector  = manager.to_vector(diagram);
    auto vectord = manager.from_vector(vector);
    BOOST_REQUIRE_MESSAGE(
        diagram.equals(vectord),
        "From-vector from to-vectored vector created the same diagram"
    );
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(from_expression, Fixture, Fixtures, Fixture)
{
    auto manager  = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto exprtree = tsl::make_expression_tree(
        manager.get_var_count(),
        Fixture::rng_,
        Fixture::rng_
    );
    auto diagram  = manager.from_expression_tree(*exprtree);
    auto domainit = tsl::domain_iterator(manager.get_domains());
    auto evalit   = teddy::tsl::evaluating_iterator(domainit, *exprtree);
    test_compare_eval(evalit, manager, diagram);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace teddy::tests