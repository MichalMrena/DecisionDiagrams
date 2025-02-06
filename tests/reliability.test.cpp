#include <libteddy/inc/core.hpp>
#include <libteddy/inc/io.hpp>

#include <libtsl/expressions.hpp>
#include <libtsl/generators.hpp>
#include <libtsl/iterators.hpp>
#include <libtsl/probabilities.hpp>
#include <libtsl/system_description.hpp>
#include <libtsl/truth_table.hpp>
#include <libtsl/truth_table_reliability.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/tools/fpc_tolerance.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <gmpxx.h>

#include <algorithm>
#include <array>
#include <vector>

#include "setup.hpp"

#ifdef LIBTEDDY_ARBITRARY_PRECISION

template<>
struct fmt::formatter<mpz_class> : formatter<std::string_view> {
  auto format (const mpz_class &c, format_context &ctx) const
    -> format_context::iterator {
    return formatter<string_view>::format(c.get_str(), ctx);
  }
};

#endif

namespace teddy::tests {

namespace  {

auto compare_dplds (auto &manager, auto const &tableDpld, auto diagramDpld) {
  auto result = true;
  domain_for_each(
    tableDpld,
    [&manager, &result, &diagramDpld] (auto const val, auto const &elem) {
      if (manager.evaluate(diagramDpld, elem) != val) {
        result = false;
      }
    }
  );
  return result;
}

using Change = struct {
  int from_;
  int to_;
};

auto switch_direction (Change change) -> Change {
  return {change.to_, change.from_};
}

auto add_opposite_directions (std::vector<Change> &changes) {
  auto const changeCount = ssize(changes);
  for (auto i = 0; i < changeCount; ++i) {
    changes.push_back(switch_direction(changes[as_uindex(i)]));
  }
}

}  // namespace

/**
 * \brief BSS fixture
 */
struct bss_fixture {
  bss_manager_settings manager_cfg_ {
    .varcount_  = 21,
    .nodecount_ = 2'000'000,
    .order_     = random_order_tag()
  };

  expression_tree_settings expr_cfg_ {
    .varcount_ = 21
  };

  tsl::rng_t rng_ {911};

  int state_count_ {2};
};

/**
 * \brief MSS fixture
 */
template<int32 M>
struct mss_fixture {
  mss_manager_settings<3> manager_cfg_ {
    .varcount_  = 15,
    .nodecount_ = 2'000'000,
    .order_     = random_order_tag()
  };

  expression_tree_settings expr_cfg_ {
    .varcount_ = 15
  };

  tsl::rng_t rng_ {911};

  int state_count_ {M};
};

/**
 * \brief iMSS fixture
 */
template<int32 M>
struct imss_fixture {
  imss_manager_settings<3> manager_cfg_ {
    .varcount_  = 15,
    .nodecount_ = 5'000,
    .order_     = random_order_tag(),
    .domains_   = random_domains_tag()
  };

  expression_tree_settings expr_cfg_ {
    .varcount_ = 15
  };

  tsl::rng_t rng_ {911};

  int state_count_ {M};
};

/**
 * \brief ifMSS fixture base
 */
template<int32 M>
struct ifmss_fixture {
  ifmss_manager_settings<3> manager_cfg_ {
    .varcount_  = 15,
    .nodecount_ = 5'000,
    .order_     = random_order_tag(),
    .domains_   = random_domains_tag()
  };

  expression_tree_settings expr_cfg_ {
    .varcount_ = 15
  };

  tsl::rng_t rng_ {911};

  int state_count_ {M};
};

auto constexpr FloatingTolerance = 0.00000001;

using Fixtures                   = boost::mpl::vector<
                    teddy::tests::bss_fixture,
                    teddy::tests::mss_fixture<3>,
                    teddy::tests::imss_fixture<3>,
                    teddy::tests::ifmss_fixture<3>>;

BOOST_AUTO_TEST_SUITE(reliability)

BOOST_FIXTURE_TEST_CASE(probabilities_bss, teddy::tests::bss_fixture) {
  auto const expr    = make_expression(expr_cfg_, rng_);
  auto manager       = make_manager(manager_cfg_, rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const probVec = tsl::make_prob_vector(manager, rng_);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);
  auto expected      = std::vector<double>(as_uindex(state_count_));
  auto actual        = std::vector<double>(as_uindex(state_count_));
  expected[1]        = probability(table, probVec);
  expected[0]        = 1 - expected[1];
  double const A     = manager.calculate_availability(probVec, diagram);
  double const U     = manager.calculate_unavailability(probVec, diagram);
  actual[0]          = U;
  actual[1]          = A;

  BOOST_TEST(
    actual[0] == expected[0],
    boost::test_tools::tolerance(FloatingTolerance)
  );
  BOOST_TEST(
    actual[1] == expected[1],
    boost::test_tools::tolerance(FloatingTolerance)
  );
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(probabilities, Fixture, Fixtures, Fixture) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager       = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const probs   = tsl::make_probabilities(manager, Fixture::rng_);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);
  auto expected      = std::vector<double>(as_uindex(Fixture::state_count_));
  auto actual        = manager.calculate_probabilities(probs, diagram);

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    expected[as_uindex(j)] = probability(table, probs, j);
  }

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    BOOST_TEST(
      actual[as_uindex(j)] == expected[as_uindex(j)],
      boost::test_tools::tolerance(FloatingTolerance)
    );
  }

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    actual[as_uindex(j)] = manager.calculate_probability(j, probs, diagram);
  }

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    BOOST_TEST(
      actual[as_uindex(j)] == expected[as_uindex(j)],
      boost::test_tools::tolerance(FloatingTolerance)
    );
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(availabilities, Fixture, Fixtures, Fixture) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager  = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto diagram  = tsl::make_diagram(expr, manager);
  auto probs    = tsl::make_probabilities(manager, Fixture::rng_);
  auto domains  = manager.get_domains();
  auto table    = tsl::truth_table(make_vector(expr, domains), domains);
  auto expected = std::vector<double>(as_uindex(Fixture::state_count_));
  auto actual   = std::vector<double>(as_uindex(Fixture::state_count_));

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    expected[as_uindex(j)] = availability(table, probs, j);
  }

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    actual[as_uindex(j)] = manager.calculate_availability(j, probs, diagram);
  }

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    BOOST_TEST(
      actual[as_uindex(j)] == expected[as_uindex(j)],
      boost::test_tools::tolerance(FloatingTolerance)
    );
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(unavailabilities, Fixture, Fixtures, Fixture) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager       = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const probs   = tsl::make_probabilities(manager, Fixture::rng_);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);
  auto expected      = std::vector<double>(as_uindex(Fixture::state_count_));
  auto actual        = std::vector<double>(as_uindex(Fixture::state_count_));

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    expected[as_uindex(j)] = unavailability(table, probs, j);
  }

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    actual[as_uindex(j)] = manager.calculate_unavailability(j, probs, diagram);
  }

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    BOOST_TEST(
      expected[as_uindex(j)] == actual[as_uindex(j)],
      boost::test_tools::tolerance(FloatingTolerance)
    );
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(states_frequency, Fixture, Fixtures, Fixture) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager       = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);
  auto expected      = std::vector<double>(as_uindex(Fixture::state_count_));
  auto actual        = std::vector<double>(as_uindex(Fixture::state_count_));

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    expected[as_uindex(j)] = state_frequency(table, j);
  }

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    actual[as_uindex(j)] = manager.state_frequency(diagram, j);
  }

  for (auto j = 0; j < Fixture::state_count_; ++j) {
    BOOST_TEST(
      actual[as_uindex(j)] == expected[as_uindex(j)],
      boost::test_tools::tolerance(FloatingTolerance)
    );
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(
  structural_importances,
  Fixture,
  Fixtures,
  Fixture
) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager       = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

  for (auto systemState = 1; systemState < Fixture::state_count_;
       ++systemState) {
    for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex) {
      for (auto varVal = 1; varVal < domains[as_uindex(varIndex)]; ++varVal) {
        auto const tableDpld = tsl::dpld(
          table,
          {varIndex, varVal, varVal - 1},
          tsl::type_3_decrease(systemState)
        );
        auto const diagramDpld = manager.dpld(
          {varIndex, varVal, varVal - 1},
          dpld::type_3_decrease(systemState),
          diagram
        );
        auto const expected = tsl::structural_importance(tableDpld, varIndex);
        auto const actual   = manager.structural_importance(diagramDpld);
        BOOST_TEST(
          expected == actual,
          boost::test_tools::tolerance(FloatingTolerance)
        );
      }
    }
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(
  birnbaum_importances,
  Fixture,
  Fixtures,
  Fixture
) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager       = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const probs   = tsl::make_probabilities(manager, Fixture::rng_);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

  for (auto systemState = 1; systemState < Fixture::state_count_;
       ++systemState) {
    for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex) {
      for (auto varVal = 1; varVal < manager.get_domains()[as_uindex(varIndex)];
           ++varVal) {
        auto const tableDpld = tsl::dpld(
          table,
          {varIndex, varVal, varVal - 1},
          tsl::type_3_decrease(systemState)
        );
        auto const diagramDpld = manager.dpld(
          {varIndex, varVal, varVal - 1},
          dpld::type_3_decrease(systemState),
          diagram
        );
        auto const expected = tsl::birnbaum_importance(tableDpld, probs);
        auto const actual   = manager.birnbaum_importance(probs, diagramDpld);
        BOOST_TEST(
          expected == actual,
          boost::test_tools::tolerance(FloatingTolerance)
        );
      }
    }
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(
  fussell_vesely_importances,
  Fixture,
  Fixtures,
  Fixture
) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager       = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const probs   = tsl::make_probabilities(manager, Fixture::rng_);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

  for (auto systemState = 1; systemState < Fixture::state_count_;
       ++systemState) {
    auto const unavail = tsl::unavailability(table, probs, systemState);
    for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex) {
      for (auto varVal = 1; varVal < manager.get_domains()[as_uindex(varIndex)];
           ++varVal) {
        auto const diagramDpld = manager.dpld(
          {varIndex, varVal, varVal - 1},
          dpld::type_3_decrease(systemState),
          diagram
        );
        auto const expected = tsl::fussell_vesely_importance(
          table,
          probs,
          varIndex,
          varVal,
          systemState
        );
        auto const actual = manager.fussell_vesely_importance(
          probs,
          diagramDpld,
          unavail,
          varVal,
          varIndex
        );
        BOOST_TEST(
          expected == actual,
          boost::test_tools::tolerance(FloatingTolerance)
        );
      }
    }
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mcvs, Fixture, Fixtures, Fixture) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager       = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

  for (auto state = 1; state < Fixture::state_count_; ++state) {
    auto const tableMcvs = tsl::calculate_mcvs(table, state);
    auto const diagramMcvs
      = manager.template mcvs<std::vector<int32>>(diagram, state);
    BOOST_REQUIRE(std::ranges::is_permutation(tableMcvs, diagramMcvs));
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(basic_dpld, Fixture, Fixtures, Fixture) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager       = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

  // for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
  for (auto varIndex = 0; varIndex < 1; ++varIndex) {
    auto const varDomain = manager.get_domains()[as_uindex(varIndex)];
    auto varChanges      = std::vector<Change>();
    auto fChanges        = std::vector<Change>();

    for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom) {
      for (auto varTo = varFrom + 1; varTo < varDomain; ++varTo) {
        varChanges.push_back(Change {varFrom, varTo});
      }
    }
    add_opposite_directions(varChanges);

    for (auto fFrom = 0; fFrom < Fixture::state_count_ - 1; ++fFrom) {
      for (auto fTo = fFrom; fTo < Fixture::state_count_; ++fTo) {
        fChanges.push_back(Change {fFrom, fTo});
      }
    }
    add_opposite_directions(fChanges);

    for (auto const &varChange : varChanges) {
      for (auto const &fChange : fChanges) {
        auto const tableDpld = tsl::dpld(
          table,
          tsl::var_change {varIndex, varChange.from_, varChange.to_},
          tsl::dpld_basic(fChange.from_, fChange.to_)
        );
        auto const tableDpldExtended = tsl::dpld_e(
          table,
          tsl::var_change {varIndex, varChange.from_, varChange.to_},
          tsl::dpld_basic(fChange.from_, fChange.to_)
        );

        auto const diagramDpld = manager.dpld(
          {varIndex, varChange.from_, varChange.to_},
          dpld::basic(fChange.from_, fChange.to_),
          diagram
        );
        auto const diagramDpldExtended
          = manager.to_dpld_e(varChange.from_, varIndex, diagramDpld);
        longint const oneCount = manager.satisfy_count(1, diagramDpld);

        BOOST_TEST_MESSAGE(fmt::format(
          "Basic dpld f({} -> {}) / x{}({} -> {})",
          fChange.from_,
          fChange.to_,
          varIndex,
          varChange.from_,
          varChange.to_
        ));
        BOOST_TEST_MESSAGE(fmt::format("One count = {}", oneCount));
        BOOST_REQUIRE(compare_dplds(manager, tableDpld, diagramDpld));
        BOOST_REQUIRE(compare_dplds(
          manager,
          tableDpldExtended,
          diagramDpldExtended
        ));
      }
    }
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(
  integrated_dpld_1,
  Fixture,
  Fixtures,
  Fixture
) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager       = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

  // for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
  for (auto varIndex = 0; varIndex < 1; ++varIndex) {
    auto const varDomain = manager.get_domains()[as_uindex(varIndex)];
    auto varChanges      = std::vector<Change>();

    for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom) {
      for (auto varTo = varFrom + 1; varTo < varDomain; ++varTo) {
        varChanges.push_back(Change {varFrom, varTo});
      }
    }
    add_opposite_directions(varChanges);

    for (auto fValue = 0; fValue < Fixture::state_count_ - 1; ++fValue) {
      for (auto const &varChange : varChanges) {
        auto const tableDpldDecrease = tsl::dpld(
          table,
          tsl::var_change {varIndex, varChange.from_, varChange.to_},
          tsl::type_1_decrease(fValue + 1)
        );
        auto const tableDpldDecreaseExtended = tsl::dpld_e(
          table,
          tsl::var_change {varIndex, varChange.from_, varChange.to_},
          tsl::type_1_decrease(fValue + 1)
        );
        auto const tableDpldIncrease = tsl::dpld(
          table,
          tsl::var_change {varIndex, varChange.from_, varChange.to_},
          tsl::type_1_increase(fValue)
        );
        auto const tableDpldIncreaseExtended = tsl::dpld_e(
          table,
          tsl::var_change {varIndex, varChange.from_, varChange.to_},
          tsl::type_1_increase(fValue)
        );

        auto const diagramDpldDecrease = manager.dpld(
          {varIndex, varChange.from_, varChange.to_},
          dpld::type_1_decrease(fValue + 1),
          diagram
        );
        auto const diagramDpldDecreaseExtended
          = manager.to_dpld_e(varChange.from_, varIndex, diagramDpldDecrease);
        auto const diagramDpldIncrease = manager.dpld(
          {varIndex, varChange.from_, varChange.to_},
          dpld::type_1_increase(fValue),
          diagram
        );
        auto const diagramDpldIncreaseExtended
          = manager.to_dpld_e(varChange.from_, varIndex, diagramDpldIncrease);
        auto const oneCountDecrease
          = manager.satisfy_count(1, diagramDpldDecrease);
        auto const oneCountIncrease
          = manager.satisfy_count(1, diagramDpldIncrease);

        BOOST_TEST_MESSAGE(fmt::format(
          "idpld_type_1_decrease f({} -> <{}) / x{}({} -> {})",
          fValue + 1,
          fValue + 1,
          varIndex,
          varChange.from_,
          varChange.to_
        ));
        BOOST_TEST_MESSAGE(fmt::format(
          "idpld_type_1_increase f({} -> >{}) / x{}({} -> {})",
          fValue,
          fValue,
          varIndex,
          varChange.from_,
          varChange.to_
        ));
        BOOST_TEST_MESSAGE(
          fmt::format("One count decrease = {}", oneCountDecrease)
        );
        BOOST_TEST_MESSAGE(
          fmt::format("One count increase = {}", oneCountIncrease)
        );
        BOOST_REQUIRE(compare_dplds(
          manager,
          tableDpldDecrease,
          diagramDpldDecrease
        ));
        BOOST_REQUIRE(compare_dplds(
          manager,
          tableDpldDecreaseExtended,
          diagramDpldDecreaseExtended
        ));
        BOOST_REQUIRE(compare_dplds(
          manager,
          tableDpldIncrease,
          diagramDpldIncrease
        ));
        BOOST_REQUIRE(compare_dplds(
          manager,
          tableDpldIncreaseExtended,
          diagramDpldIncreaseExtended
        ));
      }
    }
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(
  integrated_dpld_2,
  Fixture,
  Fixtures,
  Fixture
) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager       = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

  // for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
  for (auto varIndex = 0; varIndex < 1; ++varIndex) {
    auto const varDomain = domains[as_uindex(varIndex)];
    auto varChanges      = std::vector<Change>();

    for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom) {
      for (auto varTo = varFrom + 1; varTo < varDomain; ++varTo) {
        varChanges.push_back(Change {varFrom, varTo});
      }
    }
    add_opposite_directions(varChanges);

    for (auto const &varChange : varChanges) {
      auto const tableDpldDecrease = tsl::dpld(
        table,
        tsl::var_change {varIndex, varChange.from_, varChange.to_},
        tsl::type_2_decrease()
      );
      auto const tableDpldDecreaseExtended = tsl::dpld_e(
        table,
        tsl::var_change {varIndex, varChange.from_, varChange.to_},
        tsl::type_2_decrease()
      );
      auto const tableDpldIncrease = tsl::dpld(
        table,
        tsl::var_change {varIndex, varChange.from_, varChange.to_},
        tsl::type_2_increase()
      );
      auto const tableDpldIncreaseExtended = tsl::dpld_e(
        table,
        tsl::var_change {varIndex, varChange.from_, varChange.to_},
        tsl::type_2_increase()
      );
      auto const diagramDpldDecrease = manager.dpld(
        {varIndex, varChange.from_, varChange.to_},
        dpld::type_2_decrease(),
        diagram
      );
      auto const diagramDpldDecreaseExtended
        = manager.to_dpld_e(varChange.from_, varIndex, diagramDpldDecrease);
      auto const diagramDpldIncrease = manager.dpld(
        {varIndex, varChange.from_, varChange.to_},
        dpld::type_2_increase(),
        diagram
      );
      auto const diagramDpldIncreaseExtended
        = manager.to_dpld_e(varChange.from_, varIndex, diagramDpldIncrease);
      auto const oneCountDecrease
        = manager.satisfy_count(1, diagramDpldDecrease);
      auto const oneCountIncrease
        = manager.satisfy_count(1, diagramDpldIncrease);

      BOOST_TEST_MESSAGE(fmt::format(
        "idpld_type_2_decrease f( < ) / x{}({} -> {})",
        varIndex,
        varChange.from_,
        varChange.to_
      ));
      BOOST_TEST_MESSAGE(fmt::format(
        "idpld_type_2_increase f( > ) / x{}({} -> {})",
        varIndex,
        varChange.from_,
        varChange.to_
      ));
      BOOST_TEST_MESSAGE(
        fmt::format("One count decrease = {}", oneCountDecrease)
      );
      BOOST_TEST_MESSAGE(
        fmt::format("One count increase = {}", oneCountIncrease)
      );
      BOOST_REQUIRE(
        compare_dplds(manager, tableDpldDecrease, diagramDpldDecrease)
      );
      BOOST_REQUIRE(compare_dplds(
        manager,
        tableDpldDecreaseExtended,
        diagramDpldDecreaseExtended
      ));
      BOOST_REQUIRE(
        compare_dplds(manager, tableDpldIncrease, diagramDpldIncrease)
      );
      BOOST_REQUIRE(compare_dplds(
        manager,
        tableDpldIncreaseExtended,
        diagramDpldIncreaseExtended
      ));
    }
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(
  integrated_dpld_3,
  Fixture,
  Fixtures,
  Fixture
) {
  auto const expr
    = make_expression(Fixture::expr_cfg_, Fixture::rng_);
  auto manager       = make_manager(Fixture::manager_cfg_, Fixture::rng_);
  auto const diagram = tsl::make_diagram(expr, manager);
  auto const domains = manager.get_domains();
  auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

  // for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
  for (auto varIndex = 0; varIndex < 1; ++varIndex) {
    auto const varDomain = manager.get_domains()[as_uindex(varIndex)];
    auto varChanges      = std::vector<Change>();

    for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom) {
      for (auto varTo = varFrom + 1; varTo < varDomain; ++varTo) {
        varChanges.push_back(Change {varFrom, varTo});
      }
    }
    add_opposite_directions(varChanges);

    for (auto fValue = 1; fValue < Fixture::state_count_; ++fValue) {
      for (auto const &varChange : varChanges) {
        auto tableDpldDecrease = tsl::dpld(
          table,
          tsl::var_change {varIndex, varChange.from_, varChange.to_},
          tsl::type_3_decrease(fValue)
        );
        auto tableDpldDecreaseExtended = tsl::dpld_e(
          table,
          tsl::var_change {varIndex, varChange.from_, varChange.to_},
          tsl::type_3_decrease(fValue)
        );
        auto tableDpldIncrease = tsl::dpld(
          table,
          tsl::var_change {varIndex, varChange.from_, varChange.to_},
          tsl::type_3_increase(fValue)
        );
        auto tableDpldIncreaseExtended = tsl::dpld_e(
          table,
          tsl::var_change {varIndex, varChange.from_, varChange.to_},
          tsl::type_3_increase(fValue)
        );

        auto const diagramDpldDecrease = manager.dpld(
          {varIndex, varChange.from_, varChange.to_},
          dpld::type_3_decrease(fValue),
          diagram
        );
        auto const diagramDpldDecreaseExtended
          = manager.to_dpld_e(varChange.from_, varIndex, diagramDpldDecrease);
        auto const diagramDpldIncrease = manager.dpld(
          {varIndex, varChange.from_, varChange.to_},
          dpld::type_3_increase(fValue),
          diagram
        );
        auto const diagramDpldIncreaseExtended
          = manager.to_dpld_e(varChange.from_, varIndex, diagramDpldIncrease);
        auto const oneCountDecrease
          = manager.satisfy_count(1, diagramDpldDecrease);
        auto const oneCountIncrease
          = manager.satisfy_count(1, diagramDpldIncrease);

        BOOST_TEST_MESSAGE(fmt::format(
          "idpld_type_3_decrease f(>={} -> <{}) / x{}({} -> {})",
          fValue,
          fValue,
          varIndex,
          varChange.from_,
          varChange.to_
        ));
        BOOST_TEST_MESSAGE(fmt::format(
          "idpld_type_3_increase f(<{} -> >={}) / x{}({} -> {})",
          fValue,
          fValue,
          varIndex,
          varChange.from_,
          varChange.to_
        ));
        BOOST_TEST_MESSAGE(
          fmt::format("One count decrease = {}", oneCountDecrease)
        );
        BOOST_TEST_MESSAGE(
          fmt::format("One count increase = {}", oneCountIncrease)
        );
        BOOST_REQUIRE(compare_dplds(
          manager,
          tableDpldDecrease,
          diagramDpldDecrease
        ));
        BOOST_REQUIRE(compare_dplds(
          manager,
          tableDpldDecreaseExtended,
          diagramDpldDecreaseExtended
        ));
        BOOST_REQUIRE(compare_dplds(
          manager,
          tableDpldIncrease,
          diagramDpldIncrease
        ));
        BOOST_REQUIRE(compare_dplds(
          manager,
          tableDpldIncreaseExtended,
          diagramDpldIncreaseExtended
        ));
      }
    }
  }
}

std::array const systems { // NOLINT
  tsl::system_description {
    .systemId_ = 1,
    .stateCount_ = 2,
    .componentCount_ = 5,
    .structureFunction_ = {
        0,1,0,1,0,1,1,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1
    },
    .domains_ = {
        2,2,2,2,2
    },
    .componentProbabilities_ = {
        {.1,.9},
        {.2,.8},
        {.3,.7},
        {.1,.9},
        {.1,.9},
    },
    .stateProbabilities_ = {.01036,.98964},
    .availabilities_ = {1,.98964},
    .unavailabilities_ = {0,.01036},
    .mcvs_ = {
        /* state 0 */
        {
        },

        /* state 1 */
        {
            {0,1,0,1,0},
            {0,1,1,0,0},
            {1,0,0,1,0},
            {1,0,1,0,0}
        }
    },
    .mpvs_ = {
        /* state 0 */
        {
        },

        /* state 1 */
        {
            {0,0,0,0,1},
            {0,0,1,1,0},
            {1,1,0,0,0}
        }

    },
    .structuralImportances_ = {
        {{}, {-1, .18750}}, /* x0 */
        {{}, {-1, .18750}}, /* x1 */
        {{}, {-1, .18750}}, /* x2 */
        {{}, {-1, .18750}}, /* x3 */
        {{}, {-1, .56250}}  /* x4 */
    },
    .birnbaumImportances_ = {
        {{}, {-1, .02960}}, /* x0 */
        {{}, {-1, .03330}}, /* x1 */
        {{}, {-1, .02520}}, /* x2 */
        {{}, {-1, .01960}}, /* x3 */
        {{}, {-1, .10360}}  /* x4 */
    },
    .fussellVeselyImportances_ = {
        {{}, {-1, .35714}}, /* x0 */
        {{}, {-1, .71429}}, /* x1 */
        {{}, {-1, .81081}}, /* x2 */
        {{}, {-1, .27027}}, /* x3 */
        {{}, {-1, 1.0000}}  /* x4 */
    },

    .floatingTolerance_ = 0.00001
  }
};

BOOST_DATA_TEST_CASE(system_test, systems, system) {
  auto const table
    = tsl::truth_table(system.structureFunction_, system.domains_);
  auto constexpr InitNodeCount = 10'000;
  auto manager                 = teddy::imss_manager(
    system.componentCount_,
    InitNodeCount,
    system.domains_
  );
  auto const diagram = io::from_vector(manager, system.structureFunction_);

  // System State Probabilities
  for (auto state = 0; state < system.stateCount_; ++state) {
    BOOST_TEST(
      system.stateProbabilities_[tsl::as_uindex(state)]
        == tsl::probability(table, system.componentProbabilities_, state),
      boost::test_tools::tolerance(FloatingTolerance)
    );
    BOOST_TEST(
      system.stateProbabilities_[tsl::as_uindex(state)]
        == manager.calculate_probability(
          state,
          system.componentProbabilities_,
          diagram
        ),
      boost::test_tools::tolerance(FloatingTolerance)
    );
  }

  // Availabilities
  for (auto state = 0; state < system.stateCount_; ++state) {
    BOOST_TEST(
      system.availabilities_[tsl::as_uindex(state)]
        == tsl::availability(table, system.componentProbabilities_, state),
      boost::test_tools::tolerance(FloatingTolerance)
    );
    BOOST_TEST(
      system.availabilities_[tsl::as_uindex(state)]
        == manager.calculate_availability(
          state,
          system.componentProbabilities_,
          diagram
        ),
      boost::test_tools::tolerance(FloatingTolerance)
    );
  }

  // Unavailabilities
  for (auto state = 0; state < system.stateCount_; ++state) {
    BOOST_TEST(
      system.unavailabilities_[tsl::as_uindex(state)]
        == tsl::unavailability(table, system.componentProbabilities_, state),
      boost::test_tools::tolerance(FloatingTolerance)
    );
    BOOST_TEST(
      system.unavailabilities_[tsl::as_uindex(state)]
        == manager.calculate_unavailability(
          state,
          system.componentProbabilities_,
          diagram
        ),
      boost::test_tools::tolerance(FloatingTolerance)
    );
  }

  // Minimal Cut Vectors
  for (auto state = 1; state < system.stateCount_; ++state) {
    BOOST_REQUIRE(std::ranges::is_permutation(
      system.mcvs_[as_uindex(state)],
      tsl::calculate_mcvs(table, state)
    ));
    BOOST_REQUIRE(std::ranges::is_permutation(
      system.mcvs_[as_uindex(state)],
      manager.mcvs<std::vector<int32>>(diagram, state)
    ));
  }

  // Minimal Path Vectors
  for (auto state = 1; state < system.stateCount_; ++state) {
    BOOST_REQUIRE(std::ranges::is_permutation(
      system.mpvs_[as_uindex(state)],
      tsl::calculate_mpvs(table, state)
    ));
    BOOST_REQUIRE(std::ranges::is_permutation(
      system.mcvs_[as_uindex(state)],
      manager.mpvs<std::vector<int32>>(diagram, state)
    ));
  }

  // Structural Importances (Integrated DPLD Type III)
  for (auto varIndex = 0; varIndex < system.componentCount_; ++varIndex) {
    for (auto systemState = 1; systemState < system.stateCount_;
         ++systemState) {
      auto const domain = system.domains_[as_uindex(varIndex)];
      for (auto componentState = 1; componentState < domain; ++componentState) {
        auto const tableDpld = tsl::dpld(
          table,
          {.index=varIndex, .from=componentState, .to=componentState - 1},
          tsl::type_3_decrease(systemState)
        );
        auto const diagramDpld = manager.dpld(
          {varIndex, componentState, componentState - 1},
          dpld::type_3_decrease(systemState),
          diagram
        );
        auto const realSI    = system.structuralImportances_[as_uindex(varIndex
        )][as_uindex(systemState)][as_uindex(componentState)];
        auto const tableSI   = tsl::structural_importance(tableDpld, varIndex);
        auto const diagramSI = manager.structural_importance(diagramDpld);

        BOOST_TEST(
          realSI == tableSI,
          boost::test_tools::tolerance(FloatingTolerance)
        );
        BOOST_TEST(
          realSI == diagramSI,
          boost::test_tools::tolerance(FloatingTolerance)
        );
      }
    }
  }

  // Birnbaum Importances (Integrated DPLD Type III)
  for (auto varIndex = 0; varIndex < system.componentCount_; ++varIndex) {
    for (auto systemState = 1; systemState < system.stateCount_;
         ++systemState) {
      auto const domain = system.domains_[as_uindex(varIndex)];
      for (auto componentState = 1; componentState < domain; ++componentState) {
        auto const tableDpld = tsl::dpld(
          table,
          {.index=varIndex, .from=componentState, .to=componentState - 1},
          tsl::type_3_decrease(systemState)
        );
        auto const diagramDpld = manager.dpld(
          {varIndex, componentState, componentState - 1},
          dpld::type_3_decrease(systemState),
          diagram
        );
        auto const realBI = system.birnbaumImportances_[as_uindex(varIndex
        )][as_uindex(systemState)][as_uindex(componentState)];
        auto const tableBI
          = tsl::birnbaum_importance(tableDpld, system.componentProbabilities_);
        auto const diagramBI = manager.birnbaum_importance(
          system.componentProbabilities_,
          diagramDpld
        );

        BOOST_TEST(
          realBI == tableBI,
          boost::test_tools::tolerance(FloatingTolerance)
        );
        BOOST_TEST(
          realBI == diagramBI,
          boost::test_tools::tolerance(FloatingTolerance)
        );
      }
    }
  }

  // Fussell-Vesely Importances (Integrated DPLD Type III)
  for (auto varIndex = 0; varIndex < system.componentCount_; ++varIndex) {
    for (auto systemState = 1; systemState < system.stateCount_;
         ++systemState) {
      auto const domain = system.domains_[as_uindex(varIndex)];
      for (auto componentState = 1; componentState < domain; ++componentState) {
        auto const realFVI = system.fussellVeselyImportances_[as_uindex(varIndex
        )][as_uindex(systemState)][as_uindex(componentState)];
        auto const tableFVI = tsl::fussell_vesely_importance(
          table,
          system.componentProbabilities_,
          varIndex,
          componentState,
          systemState
        );
        auto const diagramUnavailability = manager.calculate_unavailability(
          systemState,
          system.componentProbabilities_,
          diagram
        );
        auto const diagramDpld = manager.dpld(
          {varIndex, componentState - 1, componentState},
          dpld::type_3_increase(systemState),
          diagram
        );
        auto const diagramFVI = manager.fussell_vesely_importance(
          system.componentProbabilities_,
          diagramDpld,
          diagramUnavailability,
          componentState,
          varIndex
        );

        BOOST_TEST(
          realFVI == tableFVI,
          boost::test_tools::tolerance(system.floatingTolerance_)
        );

        BOOST_TEST(
          realFVI == diagramFVI,
          boost::test_tools::tolerance(system.floatingTolerance_)
        );
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace teddy::tests
