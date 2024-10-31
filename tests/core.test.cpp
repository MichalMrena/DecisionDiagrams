#include <libteddy/inc/core.hpp>
#include <libteddy/inc/io.hpp>

#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <libtsl/iterators.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <fmt/core.h>

#include <filesystem>

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
  tsl::rng_t rng_;
  int32 maxValue_ {}; // TODO(michal): rename to codomainSize_
  // TODO(michal): move members to to child classes, remove this class
};

/**
 *  \brief BDD fixture
 */
struct bdd_fixture :
  fixture_base<bdd_manager_settings, minmax_expression_settings>
{
private:
  static auto constexpr VarCount  = 21;
  static auto constexpr NodeCount = 5'000;
  static auto constexpr TermCount = 20;
  static auto constexpr TermSize  = 5;
  static auto constexpr Seed      = 911;

public:
  bdd_fixture() :
    fixture_base<bdd_manager_settings, minmax_expression_settings> {
      .managerSettings_
      = bdd_manager_settings {VarCount, NodeCount, random_order_tag()},
      .expressionSettings_
      = minmax_expression_settings {VarCount, TermCount, TermSize          },
      .rng_      = tsl::rng_t(Seed),
      .maxValue_ = 2
  }
  {
  }
};

/**
 *  \brief MDD fixture
 */
struct mdd_fixture :
  fixture_base<mdd_manager_settings<3>, minmax_expression_settings>
{
private:
  static auto constexpr VarCount  = 15;
  static auto constexpr NodeCount = 5'000;
  static auto constexpr TermCount = 20;
  static auto constexpr TermSize  = 5;
  static auto constexpr Seed      = 911;

public:
  mdd_fixture() :
    fixture_base<mdd_manager_settings<3>, minmax_expression_settings> {
      .managerSettings_
      = mdd_manager_settings<3> {VarCount, NodeCount, random_order_tag()},
      .expressionSettings_
      = minmax_expression_settings {VarCount, TermCount, TermSize          },
      .rng_      = tsl::rng_t(Seed),
      .maxValue_ = 3
  }
  {
  }
};

/**
 *  \brief iMDD fixture
 */
struct imdd_fixture :
  fixture_base<imdd_manager_settings<3>, minmax_expression_settings>
{
private:
  static auto constexpr VarCount  = 15;
  static auto constexpr NodeCount = 5'000;
  static auto constexpr TermCount = 20;
  static auto constexpr TermSize  = 5;
  static auto constexpr Seed      = 911;

public:
  imdd_fixture() :
    fixture_base<imdd_manager_settings<3>, minmax_expression_settings> {
      .managerSettings_ = imdd_manager_settings<
        3> {{{VarCount, NodeCount, random_order_tag()}, random_domains_tag()}},
      .expressionSettings_
      = minmax_expression_settings {VarCount, TermCount, TermSize},
      .rng_      = tsl::rng_t(Seed),
      .maxValue_ = 3
  }
  {
  }
};

/**
 *  \brief ifMDD fixture
 */
struct ifmdd_fixture :
  fixture_base<ifmdd_manager_settings<3>, minmax_expression_settings>
{
private:
  static auto constexpr VarCount  = 15;
  static auto constexpr NodeCount = 5'000;
  static auto constexpr TermCount = 20;
  static auto constexpr TermSize  = 5;
  static auto constexpr Seed      = 911;

public:
  ifmdd_fixture() :
    fixture_base<ifmdd_manager_settings<3>, minmax_expression_settings> {
      .managerSettings_ = ifmdd_manager_settings<
        3> {{{VarCount, NodeCount, random_order_tag()}, random_domains_tag()}},
      .expressionSettings_
      = minmax_expression_settings {VarCount, TermCount, TermSize},
      .rng_      = tsl::rng_t(Seed),
      .maxValue_ = 3
  }
  {
  }
};

/**
 *  \brief Calculates frequency table for each possible value of \p expr .
 */
template<class Degree, class Domain>
auto expected_counts (
  diagram_manager<Degree, Domain>& manager,
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
template<class Expression, class Degree, class Domain>
auto test_compare_eval (
  tsl::evaluating_iterator<Expression> evalIt,
  diagram_manager<Degree, Domain>& manager,
  auto& diagram
) -> void
{
  auto evalEnd = tsl::evaluating_iterator_sentinel();
  while (evalIt != evalEnd)
  {
    auto const expectedVal = *evalIt;
    auto const diagramVal  = manager.evaluate(diagram, evalIt.get_var_vals());
    BOOST_REQUIRE_EQUAL(expectedVal, diagramVal);
    ++evalIt;
  }
}

using Fixtures = boost::mpl::vector<
  teddy::tests::bdd_fixture,
  teddy::tests::mdd_fixture,
  teddy::tests::imdd_fixture,
  teddy::tests::ifmdd_fixture>;

BOOST_AUTO_TEST_SUITE(core)

BOOST_FIXTURE_TEST_CASE_TEMPLATE(evaluate, Fixture, Fixtures, Fixture)
{
  auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
  auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
  auto diagram = tsl::make_diagram(expr, manager);
  BOOST_TEST_MESSAGE(
    fmt::format("Node count {}", manager.get_node_count(diagram))
  );
  auto domainit = make_domain_iterator(manager);
  auto evalit   = teddy::tsl::evaluating_iterator(domainit, expr);
  test_compare_eval(evalit, manager, diagram);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(fold, Fixture, Fixtures, Fixture)
{
  auto expr     = make_expression(Fixture::expressionSettings_, Fixture::rng_);
  auto manager  = make_manager(Fixture::managerSettings_, Fixture::rng_);
  auto diagram1 = tsl::make_diagram(expr, manager, fold_type::Left);
  auto diagram2 = tsl::make_diagram(expr, manager, fold_type::Tree);
  BOOST_TEST_MESSAGE(
    fmt::format("Node count {}", manager.get_node_count(diagram1))
  );
  BOOST_REQUIRE(diagram1.equals(diagram2));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(gc, Fixture, Fixtures, Fixture)
{
  auto expr     = make_expression(Fixture::expressionSettings_, Fixture::rng_);
  auto manager  = make_manager(Fixture::managerSettings_, Fixture::rng_);
  auto diagram1 = tsl::make_diagram(expr, manager, fold_type::Left);
  auto diagram2 = tsl::make_diagram(expr, manager, fold_type::Tree);
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
  auto diagram = tsl::make_diagram(expr, manager);
  BOOST_TEST_MESSAGE(
    fmt::format("Node count {}", manager.get_node_count(diagram))
  );
  auto expected = expected_counts(manager, expr);
  auto actual   = std::vector<teddy::longint>(expected.size(), 0);

  for (auto j = 0; j < ssize(actual); ++j)
  {
    actual[as_uindex(j)] = manager.satisfy_count(j, diagram);
  }

  for (auto k = 0; k < ssize(actual); ++k)
  {
    BOOST_REQUIRE_EQUAL(actual[as_uindex(k)], expected[as_uindex(k)]);
  }
}

#ifdef LIBTEDDY_ARBITRARY_PRECISION
BOOST_FIXTURE_TEST_CASE_TEMPLATE(satisfy_count_long, Fixture, Fixtures, Fixture)
{
  // TODO(michal): gmp test
}
#endif

BOOST_FIXTURE_TEST_CASE_TEMPLATE(satisfy_one, Fixture, Fixtures, Fixture)
{
  auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
  auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
  auto diagram = tsl::make_diagram(expr, manager);
  BOOST_TEST_MESSAGE(
    fmt::format("Node count {}", manager.get_node_count(diagram))
  );

  for (auto j = 0; j < Fixture::maxValue_; ++j)
  {
    auto const vars
      = manager.template satisfy_one<std::vector<int32>>(j, diagram);
    BOOST_REQUIRE(vars.has_value());
    BOOST_REQUIRE_EQUAL(j, manager.evaluate(diagram, *vars));
  }

  auto const justOne = manager.constant(1);
  auto const nullOpt
    = manager.template satisfy_one<std::vector<int32>>(0, justOne);

  auto const notNullOpt
    = manager.template satisfy_one<std::vector<int32>>(1, justOne);

  BOOST_REQUIRE(not nullOpt.has_value());
  BOOST_REQUIRE(notNullOpt.has_value());
  BOOST_REQUIRE_EQUAL(1, manager.evaluate(justOne, *notNullOpt));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(satisfy_all, Fixture, Fixtures, Fixture)
{
  auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
  auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
  auto diagram = tsl::make_diagram(expr, manager);
  BOOST_TEST_MESSAGE(
    fmt::format("Node count {}", manager.get_node_count(diagram))
  );
  auto expected = expected_counts(manager, expr);
  auto actual   = std::vector<int64>(expected.size(), 0);
  for (auto k = 0; k < ssize(expected); ++k)
  {
    using out_var_vals = std::vector<int32>;
    auto outf          = [&actual, k] (auto const&) { ++actual[as_uindex(k)]; };
    auto out           = tsl::forwarding_iterator<decltype(outf)>(outf);
    manager.template satisfy_all_g<out_var_vals>(k, diagram, out);
  }

  for (auto k = 0; k < ssize(actual); ++k)
  {
    BOOST_REQUIRE_EQUAL(actual[as_uindex(k)], expected[as_uindex(k)]);
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(operators_1, Fixture, Fixtures, Fixture)
{
  using namespace teddy::ops; // NOLINT
  auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
  auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
  auto diagram = tsl::make_diagram(expr, manager);
  BOOST_TEST_MESSAGE(
    fmt::format("Node count {}", manager.get_node_count(diagram))
  );
  auto const zero = manager.constant(0);
  auto const one  = manager.constant(1);
  auto const sup
    = manager.constant(std::ranges::max(manager.get_domains()) - 1);
  auto const boolValDiagram
    = manager.transform(diagram, [] (int32 const val) { return val != 0; });

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
    manager.template apply<XOR>(boolValDiagram, boolValDiagram).equals(zero),
    "XOR annihilate"
  );

  BOOST_REQUIRE_MESSAGE(
    manager.template apply<MULTIPLIES<2>>(boolValDiagram, zero).equals(zero),
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
    manager.template apply<LESS>(boolValDiagram, boolValDiagram).equals(zero),
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
    manager.template apply<MAX>(boolValDiagram, zero).equals(boolValDiagram),
    "MAX neutral"
  );
}

BOOST_AUTO_TEST_CASE(operators_2)
{
  using namespace teddy::ops;
  auto const N = Nondetermined;
  auto const U = Undefined;

  BOOST_REQUIRE_EQUAL(AND()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(AND()(0, 1), 0);
  BOOST_REQUIRE_EQUAL(AND()(0, N), 0);
  BOOST_REQUIRE_EQUAL(AND()(1, 0), 0);
  BOOST_REQUIRE_EQUAL(AND()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(AND()(1, N), N);
  BOOST_REQUIRE_EQUAL(AND()(N, 0), 0);
  BOOST_REQUIRE_EQUAL(AND()(N, 1), N);
  BOOST_REQUIRE_EQUAL(AND()(N, N), N);

  BOOST_REQUIRE_EQUAL(OR()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(OR()(0, 1), 1);
  BOOST_REQUIRE_EQUAL(OR()(0, N), N);
  BOOST_REQUIRE_EQUAL(OR()(1, 0), 1);
  BOOST_REQUIRE_EQUAL(OR()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(OR()(1, N), 1);
  BOOST_REQUIRE_EQUAL(OR()(N, 0), N);
  BOOST_REQUIRE_EQUAL(OR()(N, 1), 1);
  BOOST_REQUIRE_EQUAL(OR()(N, N), N);

  BOOST_REQUIRE_EQUAL(XOR()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(XOR()(0, 1), 1);
  BOOST_REQUIRE_EQUAL(XOR()(0, N), N);
  BOOST_REQUIRE_EQUAL(XOR()(1, 0), 1);
  BOOST_REQUIRE_EQUAL(XOR()(1, 1), 0);
  BOOST_REQUIRE_EQUAL(XOR()(1, N), N);
  BOOST_REQUIRE_EQUAL(XOR()(N, 0), N);
  BOOST_REQUIRE_EQUAL(XOR()(N, 1), N);
  BOOST_REQUIRE_EQUAL(XOR()(N, N), N);

  BOOST_REQUIRE_EQUAL(PI_CONJ()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(0, 1), 0);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(0, U), 0);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(0, N), 0);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(1, 0), 0);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(1, U), 1);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(1, N), N);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(U, 0), 0);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(U, 1), 1);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(U, U), U);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(U, N), N);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(N, 0), 0);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(N, 1), N);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(N, U), N);
  BOOST_REQUIRE_EQUAL(PI_CONJ()(N, N), N);

  BOOST_REQUIRE_EQUAL(NAND()(0, 0), 1);
  BOOST_REQUIRE_EQUAL(NAND()(0, 1), 1);
  BOOST_REQUIRE_EQUAL(NAND()(0, N), N);
  BOOST_REQUIRE_EQUAL(NAND()(1, 0), 1);
  BOOST_REQUIRE_EQUAL(NAND()(1, 1), 0);
  BOOST_REQUIRE_EQUAL(NAND()(1, N), N);
  BOOST_REQUIRE_EQUAL(NAND()(N, 0), N);
  BOOST_REQUIRE_EQUAL(NAND()(N, 1), N);
  BOOST_REQUIRE_EQUAL(NAND()(N, N), N);

  BOOST_REQUIRE_EQUAL(NOR()(0, 0), 1);
  BOOST_REQUIRE_EQUAL(NOR()(0, 1), 0);
  BOOST_REQUIRE_EQUAL(NOR()(0, N), N);
  BOOST_REQUIRE_EQUAL(NOR()(1, 0), 0);
  BOOST_REQUIRE_EQUAL(NOR()(1, 1), 0);
  BOOST_REQUIRE_EQUAL(NOR()(1, N), 0);
  BOOST_REQUIRE_EQUAL(NOR()(N, 0), N);
  BOOST_REQUIRE_EQUAL(NOR()(N, 1), 0);
  BOOST_REQUIRE_EQUAL(NOR()(N, N), N);

  BOOST_REQUIRE_EQUAL(XNOR()(0, 0), 1);
  BOOST_REQUIRE_EQUAL(XNOR()(0, 1), 0);
  BOOST_REQUIRE_EQUAL(XNOR()(0, N), N);
  BOOST_REQUIRE_EQUAL(XNOR()(1, 0), 0);
  BOOST_REQUIRE_EQUAL(XNOR()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(XNOR()(1, N), N);
  BOOST_REQUIRE_EQUAL(XNOR()(N, 0), N);
  BOOST_REQUIRE_EQUAL(XNOR()(N, 1), N);
  BOOST_REQUIRE_EQUAL(XNOR()(N, N), N);

  BOOST_REQUIRE_EQUAL(EQUAL_TO()(0, 0), 1);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(0, 1), 0);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(0, 2), 0);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(0, N), N);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(1, 0), 0);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(1, 2), 0);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(1, N), N);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(2, 0), 0);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(2, 1), 0);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(2, 2), 1);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(2, N), N);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(N, 0), N);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(N, 1), N);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(N, 2), N);
  BOOST_REQUIRE_EQUAL(EQUAL_TO()(N, N), N);

  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(0, 1), 1);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(0, 2), 1);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(0, N), N);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(1, 0), 1);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(1, 1), 0);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(1, 2), 1);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(1, N), N);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(2, 0), 1);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(2, 1), 1);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(2, 2), 0);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(2, N), N);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(N, 0), N);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(N, 1), N);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(N, 2), N);
  BOOST_REQUIRE_EQUAL(NOT_EQUAL_TO()(N, N), N);

  BOOST_REQUIRE_EQUAL(LESS()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(LESS()(0, 1), 1);
  BOOST_REQUIRE_EQUAL(LESS()(0, 2), 1);
  BOOST_REQUIRE_EQUAL(LESS()(0, N), N);
  BOOST_REQUIRE_EQUAL(LESS()(1, 0), 0);
  BOOST_REQUIRE_EQUAL(LESS()(1, 1), 0);
  BOOST_REQUIRE_EQUAL(LESS()(1, 2), 1);
  BOOST_REQUIRE_EQUAL(LESS()(1, N), N);
  BOOST_REQUIRE_EQUAL(LESS()(2, 0), 0);
  BOOST_REQUIRE_EQUAL(LESS()(2, 1), 0);
  BOOST_REQUIRE_EQUAL(LESS()(2, 2), 0);
  BOOST_REQUIRE_EQUAL(LESS()(2, N), N);
  BOOST_REQUIRE_EQUAL(LESS()(N, 0), N);
  BOOST_REQUIRE_EQUAL(LESS()(N, 1), N);
  BOOST_REQUIRE_EQUAL(LESS()(N, 2), N);
  BOOST_REQUIRE_EQUAL(LESS()(N, N), N);

  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(0, 0), 1);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(0, 1), 1);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(0, 2), 1);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(0, N), N);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(1, 0), 0);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(1, 2), 1);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(1, N), N);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(2, 0), 0);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(2, 1), 0);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(2, 2), 1);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(2, N), N);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(N, 0), N);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(N, 1), N);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(N, 2), N);
  BOOST_REQUIRE_EQUAL(LESS_EQUAL()(N, N), N);

  BOOST_REQUIRE_EQUAL(GREATER()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(GREATER()(0, 1), 0);
  BOOST_REQUIRE_EQUAL(GREATER()(0, 2), 0);
  BOOST_REQUIRE_EQUAL(GREATER()(0, N), N);
  BOOST_REQUIRE_EQUAL(GREATER()(1, 0), 1);
  BOOST_REQUIRE_EQUAL(GREATER()(1, 1), 0);
  BOOST_REQUIRE_EQUAL(GREATER()(1, 2), 0);
  BOOST_REQUIRE_EQUAL(GREATER()(1, N), N);
  BOOST_REQUIRE_EQUAL(GREATER()(2, 0), 1);
  BOOST_REQUIRE_EQUAL(GREATER()(2, 1), 1);
  BOOST_REQUIRE_EQUAL(GREATER()(2, 2), 0);
  BOOST_REQUIRE_EQUAL(GREATER()(2, N), N);
  BOOST_REQUIRE_EQUAL(GREATER()(N, 0), N);
  BOOST_REQUIRE_EQUAL(GREATER()(N, 1), N);
  BOOST_REQUIRE_EQUAL(GREATER()(N, 2), N);
  BOOST_REQUIRE_EQUAL(GREATER()(N, N), N);

  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(0, 0), 1);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(0, 1), 0);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(0, 2), 0);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(0, N), N);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(1, 0), 1);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(1, 2), 0);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(1, N), N);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(2, 0), 1);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(2, 1), 1);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(2, 2), 1);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(2, N), N);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(N, 0), N);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(N, 1), N);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(N, 2), N);
  BOOST_REQUIRE_EQUAL(GREATER_EQUAL()(N, N), N);

  BOOST_REQUIRE_EQUAL(MIN()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(MIN()(0, 1), 0);
  BOOST_REQUIRE_EQUAL(MIN()(0, 2), 0);
  BOOST_REQUIRE_EQUAL(MIN()(0, N), 0);
  BOOST_REQUIRE_EQUAL(MIN()(1, 0), 0);
  BOOST_REQUIRE_EQUAL(MIN()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(MIN()(1, 2), 1);
  BOOST_REQUIRE_EQUAL(MIN()(1, N), N);
  BOOST_REQUIRE_EQUAL(MIN()(2, 0), 0);
  BOOST_REQUIRE_EQUAL(MIN()(2, 1), 1);
  BOOST_REQUIRE_EQUAL(MIN()(2, 2), 2);
  BOOST_REQUIRE_EQUAL(MIN()(2, N), N);
  BOOST_REQUIRE_EQUAL(MIN()(N, 0), 0);
  BOOST_REQUIRE_EQUAL(MIN()(N, 1), N);
  BOOST_REQUIRE_EQUAL(MIN()(N, 2), N);
  BOOST_REQUIRE_EQUAL(MIN()(N, N), N);

  BOOST_REQUIRE_EQUAL(MAX()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(MAX()(0, 1), 1);
  BOOST_REQUIRE_EQUAL(MAX()(0, 2), 2);
  BOOST_REQUIRE_EQUAL(MAX()(0, N), N);
  BOOST_REQUIRE_EQUAL(MAX()(1, 0), 1);
  BOOST_REQUIRE_EQUAL(MAX()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(MAX()(1, 2), 2);
  BOOST_REQUIRE_EQUAL(MAX()(1, N), N);
  BOOST_REQUIRE_EQUAL(MAX()(2, 0), 2);
  BOOST_REQUIRE_EQUAL(MAX()(2, 1), 2);
  BOOST_REQUIRE_EQUAL(MAX()(2, 2), 2);
  BOOST_REQUIRE_EQUAL(MAX()(2, N), N);
  BOOST_REQUIRE_EQUAL(MAX()(N, 0), N);
  BOOST_REQUIRE_EQUAL(MAX()(N, 1), N);
  BOOST_REQUIRE_EQUAL(MAX()(N, 2), N);
  BOOST_REQUIRE_EQUAL(MAX()(N, N), N);
  BOOST_REQUIRE_EQUAL(MIN()(N, N), N);

  BOOST_REQUIRE_EQUAL(MAXB<3>()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(0, 1), 1);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(0, 2), 2);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(0, N), N);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(1, 0), 1);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(1, 2), 2);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(1, N), N);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(2, 0), 2);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(2, 1), 2);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(2, 2), 2);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(2, N), 2);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(N, 0), N);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(N, 1), N);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(N, 2), 2);
  BOOST_REQUIRE_EQUAL(MAXB<3>()(N, N), N);

  BOOST_REQUIRE_EQUAL(PLUS<3>()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(0, 1), 1);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(0, 2), 2);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(0, N), N);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(1, 0), 1);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(1, 1), 2);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(1, 2), 0);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(1, N), N);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(2, 0), 2);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(2, 1), 0);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(2, 2), 1);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(2, N), N);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(N, 0), N);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(N, 1), N);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(N, 2), N);
  BOOST_REQUIRE_EQUAL(PLUS<3>()(N, N), N);

  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(0, 0), 0);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(0, 1), 0);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(0, 2), 0);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(0, N), 0);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(1, 0), 0);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(1, 2), 2);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(1, N), N);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(2, 0), 0);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(2, 1), 2);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(2, 2), 1);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(2, N), N);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(N, 0), 0);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(N, 1), N);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(N, 2), N);
  BOOST_REQUIRE_EQUAL(MULTIPLIES<3>()(N, N), N);

  BOOST_REQUIRE_EQUAL(IMPLIES()(0, 0), 1);
  BOOST_REQUIRE_EQUAL(IMPLIES()(0, 1), 1);
  BOOST_REQUIRE_EQUAL(IMPLIES()(0, N), 1);
  BOOST_REQUIRE_EQUAL(IMPLIES()(1, 0), 0);
  BOOST_REQUIRE_EQUAL(IMPLIES()(1, 1), 1);
  BOOST_REQUIRE_EQUAL(IMPLIES()(1, N), N);
  BOOST_REQUIRE_EQUAL(IMPLIES()(N, 0), N);
  BOOST_REQUIRE_EQUAL(IMPLIES()(N, 1), 1);
  BOOST_REQUIRE_EQUAL(IMPLIES()(N, N), N);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(cofactor, Fixture, Fixtures, Fixture)
{
  auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
  auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
  auto diagram = tsl::make_diagram(expr, manager);
  BOOST_TEST_MESSAGE(
    fmt::format("Node count {}", manager.get_node_count(diagram))
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
  auto const value1 = int32 {0};
  auto const value2 = int32 {1};
  auto const intermediateDiagram
    = manager.get_cofactor(diagram, index1, value1);
  auto const cofactoredDiagram1
    = manager.get_cofactor(intermediateDiagram, index2, value2);
  auto const cofactoredDiagram2 = manager.get_cofactor(
    diagram,
    {
      {index1, value1},
      {index2, value2}
  }
  );

  auto domainIt = tsl::domain_iterator(
    manager.get_domains(),
    manager.get_order(),
    {std::make_pair(index1, value1), std::make_pair(index2, value2)}
  );
  auto evalIt = tsl::evaluating_iterator(domainIt, expr);
  test_compare_eval(evalIt, manager, cofactoredDiagram1);
  test_compare_eval(evalIt, manager, cofactoredDiagram2);
  BOOST_REQUIRE(cofactoredDiagram1.equals(cofactoredDiagram2));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(one_var_sift, Fixture, Fixtures, Fixture)
{
  auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
  auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
  auto diagram = tsl::make_diagram(expr, manager);
  BOOST_TEST_MESSAGE(
    fmt::format("Node count {}", manager.get_node_count(diagram))
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
  auto diagram = tsl::make_diagram(expr, manager);
  manager.force_gc();
  auto const actual   = manager.get_node_count();
  auto const expected = manager.get_node_count(diagram);
  BOOST_REQUIRE_EQUAL(expected, actual);
  auto domainit = make_domain_iterator(manager);
  auto evalit   = tsl::evaluating_iterator(domainit, expr);
  test_compare_eval(evalit, manager, diagram);
}

// TODO add transform test

BOOST_AUTO_TEST_SUITE_END()

// TODO move to io tests file
// TODO remove warnings...
BOOST_AUTO_TEST_SUITE(core_io)

BOOST_AUTO_TEST_CASE(from_pla)
{
  std::stringstream iost;
  iost << ".i 5\n";
  iost << ".o 1\n";
  iost << ".ilb d c b a e\n";
  iost << ".ob xor5\n";
  iost << ".p 16\n";
  iost << "11111 1\n";
  iost << "01110 1\n";
  iost << "10110 1\n";
  iost << "00111 1\n";
  iost << "11010 1\n";
  iost << "01011 1\n";
  iost << "10011 1\n";
  iost << "00010 1\n";
  iost << "11100 1\n";
  iost << "01101 1\n";
  iost << "10101 1\n";
  iost << "00100 1\n";
  iost << "11001 1\n";
  iost << "01000 1\n";
  iost << "10000 1\n";
  iost << "00001 1\n";
  iost << ".e";

  std::optional<pla_file> file = pla_file::load_file(iost, true);
  BOOST_REQUIRE_MESSAGE(file.has_value(), "Load simple PLA.");

  BOOST_REQUIRE_EQUAL(file->get_variable_count(), 5);
  BOOST_REQUIRE_EQUAL(file->get_function_count(), 1);
  BOOST_REQUIRE_EQUAL(file->get_line_count(), 16);
  BOOST_REQUIRE_EQUAL(file->get_input_labels()[0], "d");
  BOOST_REQUIRE_EQUAL(file->get_input_labels()[1], "c");
  BOOST_REQUIRE_EQUAL(file->get_input_labels()[2], "b");
  BOOST_REQUIRE_EQUAL(file->get_input_labels()[3], "a");
  BOOST_REQUIRE_EQUAL(file->get_input_labels()[4], "e");
  BOOST_REQUIRE_EQUAL(file->get_output_labels()[0], "xor5");

  bdd_manager manager(file->get_variable_count(), 1'000);
  std::vector<bdd_manager::diagram_t> diagrams = io::from_pla(manager, *file);
  BOOST_REQUIRE_EQUAL(diagrams.size(), 1);
  auto const& d = diagrams.front();

  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{1,1,1,1,1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{0,1,1,1,0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{1,0,1,1,0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{0,0,1,1,1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{1,1,0,1,0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{0,1,0,1,1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{1,0,0,1,1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{0,0,0,1,0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{1,1,1,0,0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{0,1,1,0,1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{1,0,1,0,1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{0,0,1,0,0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{1,1,0,0,1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{0,1,0,0,0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{1,0,0,0,0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array{0,0,0,0,1}), 1);
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

BOOST_FIXTURE_TEST_CASE_TEMPLATE(from_vector, Fixture, Fixtures, Fixture)
{
  // TODO(michal): test na corner-case konstantna funcia, 0 premennych a
  // podobne...

  auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
  auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
  auto diagram = tsl::make_diagram(expr, manager);
  BOOST_TEST_MESSAGE(
    fmt::format("Node count {}", manager.get_node_count(diagram))
  );
  auto domainit = make_domain_iterator(manager);
  auto evalit   = tsl::evaluating_iterator(domainit, expr);
  auto evalend  = tsl::evaluating_iterator_sentinel();
  auto vectord  = io::from_vector(manager, evalit, evalend);
  BOOST_REQUIRE_MESSAGE(
    diagram.equals(vectord),
    "From-vector created the same diagram"
  );
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(to_vector, Fixture, Fixtures, Fixture)
{
  auto expr    = make_expression(Fixture::expressionSettings_, Fixture::rng_);
  auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
  auto diagram = tsl::make_diagram(expr, manager);
  BOOST_TEST_MESSAGE(
    fmt::format("Node count {}", manager.get_node_count(diagram))
  );
  auto vector  = io::to_vector(manager, diagram);
  auto vectord = io::from_vector(manager, vector);
  BOOST_REQUIRE_MESSAGE(
    diagram.equals(vectord),
    "From-vector from to-vectored vector created the same diagram"
  );
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace teddy::tests