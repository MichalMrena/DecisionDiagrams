#include <libteddy/core.hpp>

#include <libtsl/expressions.hpp>
#include <libtsl/iterators.hpp>
#include <libtsl/truth_table.hpp>
#include <libtsl/truth_table_reliability.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <fmt/core.h>

#include <cstddef>
#include <vector>

#include "libteddy/details/types.hpp"
#include "libtsl/types.hpp"
#include "setup.hpp"

namespace teddy::tests
{
namespace details
{
auto compare_dplds (auto& manager, auto const& tableDpld, auto diagramDpld)
{
    auto result = true;
    domain_for_each(
        tableDpld,
        [&manager, &result, &diagramDpld] (auto const val, auto const& elem)
        {
            if (manager.evaluate(diagramDpld, elem) != val)
            {
                result = false;
            }
        }
    );
    return result;
};

using Change = struct
{
    int32 from;
    int32 to;
};

auto switch_direction (Change change) -> Change
{
    return {change.to, change.from};
}

auto add_opposite_directions (std::vector<Change>& changes)
{
    auto const changeCount = ssize(changes);
    for (auto i = 0; i < changeCount; ++i)
    {
        changes.push_back(switch_direction(changes[as_uindex(i)]));
    }
}
} // namespace details

/**
 *  \brief Fixture base
 */
template<class ManagerSettings, class ExpressionSettings>
struct fixture_base
{
    ManagerSettings managerSettings_;
    ExpressionSettings expressionSettings_;
    std::mt19937_64 rng_;
    int32 stateCount_ {};
};

/**
 *  \brief BSS fixture base
 */
struct bss_fixture :
    fixture_base<bss_manager_settings, expression_tree_settings>
{
private:
    inline static auto constexpr VarCount  = 21;
    inline static auto constexpr NodeCount = 2'000'000;
    inline static auto constexpr Seed      = 911;

public:
    bss_fixture() :
        fixture_base<bss_manager_settings, expression_tree_settings> {
            {bss_manager_settings {VarCount, NodeCount, random_order_tag()}},
            {expression_tree_settings {VarCount}},
            {std::mt19937_64(Seed)},
            2}
    {
    }
};

/**
 *  \brief MSS fixture base
 */
template<int32 M>
struct mss_fixture :
    fixture_base<mss_manager_settings<M>, expression_tree_settings>
{
private:
    inline static auto constexpr VarCount  = 15;
    inline static auto constexpr NodeCount = 2'000'000;
    inline static auto constexpr Seed      = 911;

public:
    mss_fixture() :
        fixture_base<mss_manager_settings<M>, expression_tree_settings> {
            {mss_manager_settings<M> {VarCount, NodeCount, random_order_tag()}},
            {expression_tree_settings {VarCount}},
            {std::mt19937_64(Seed)},
            M}
    {
    }
};

/**
 *  \brief iMSS fixture base
 */
template<int32 M>
struct imss_fixture :
    fixture_base<imss_manager_settings<3>, expression_tree_settings>
{
private:
    inline static auto constexpr VarCount  = 15;
    inline static auto constexpr NodeCount = 2'000'000;
    inline static auto constexpr Seed      = 911;

public:
    imss_fixture() :
        fixture_base<imss_manager_settings<M>, expression_tree_settings> {
            {imss_manager_settings<M> {
                {{VarCount, NodeCount, random_order_tag()},
                 random_domains_tag()}}},
            {expression_tree_settings {VarCount}},
            {std::mt19937_64(Seed)},
            M}
    {
    }
};

/**
 *  \brief ifMSS fixture base
 */
template<int32 M>
struct ifmss_fixture :
    fixture_base<ifmss_manager_settings<3>, expression_tree_settings>
{
private:
    inline static auto constexpr VarCount  = 15;
    inline static auto constexpr NodeCount = 5'000;
    inline static auto constexpr Seed      = 911;

public:
    ifmss_fixture() :
        fixture_base<ifmss_manager_settings<M>, expression_tree_settings> {
            {ifmss_manager_settings<M> {
                {{VarCount, NodeCount, random_order_tag()},
                 random_domains_tag()}}},
            {expression_tree_settings {VarCount}},
            {std::mt19937_64(Seed)},
            M}
    {
    }
};

constexpr auto FloatingTolerance = 0.00000001;

using Fixtures                   = boost::mpl::vector<
    teddy::tests::bss_fixture,
    teddy::tests::mss_fixture<3>,
    teddy::tests::imss_fixture<3>,
    teddy::tests::ifmss_fixture<3>>;

BOOST_AUTO_TEST_SUITE(reliability_test)

BOOST_FIXTURE_TEST_CASE_TEMPLATE(probabilities, Fixture, Fixtures, Fixture)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager       = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto const diagram = make_diagram(expr, manager);
    auto const probs   = make_probabilities(manager, Fixture::rng_);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);
    auto expected      = std::vector<double>(as_uindex(Fixture::stateCount_));
    auto actual        = std::vector<double>(as_uindex(Fixture::stateCount_));

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        expected[as_uindex(j)] = probability(table, probs, j);
    }

    manager.calculate_probabilities(probs, diagram);
    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        actual[as_uindex(j)] = manager.get_probability(j);
    }

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        BOOST_TEST(
            actual[as_uindex(j)] == expected[as_uindex(j)],
            boost::test_tools::tolerance(FloatingTolerance)
        );
    }

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        actual[as_uindex(j)] = manager.probability(j, probs, diagram);
    }

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        BOOST_TEST(
            actual[as_uindex(j)] == expected[as_uindex(j)],
            boost::test_tools::tolerance(FloatingTolerance)
        );
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(availabilities, Fixture, Fixtures, Fixture)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager  = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram  = make_diagram(expr, manager);
    auto probs    = make_probabilities(manager, Fixture::rng_);
    auto domains  = manager.get_domains();
    auto table    = tsl::truth_table(make_vector(expr, domains), domains);
    auto expected = std::vector<double>(as_uindex(Fixture::stateCount_));
    auto actual   = std::vector<double>(as_uindex(Fixture::stateCount_));

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        expected[as_uindex(j)] = availability(table, probs, j);
    }

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        actual[as_uindex(j)] = manager.availability(j, probs, diagram);
    }

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        BOOST_TEST(
            actual[as_uindex(j)] == expected[as_uindex(j)],
            boost::test_tools::tolerance(FloatingTolerance)
        );
    }

    manager.calculate_probabilities(probs, diagram);
    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        actual[as_uindex(j)] = manager.get_availability(j);
    }

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        BOOST_TEST(
            actual[as_uindex(j)] == expected[as_uindex(j)],
            boost::test_tools::tolerance(FloatingTolerance)
        );
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(unavailabilities, Fixture, Fixtures, Fixture)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager       = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto const diagram = make_diagram(expr, manager);
    auto const probs   = make_probabilities(manager, Fixture::rng_);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);
    auto expected      = std::vector<double>(as_uindex(Fixture::stateCount_));
    auto actual        = std::vector<double>(as_uindex(Fixture::stateCount_));

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        expected[as_uindex(j)] = unavailability(table, probs, j);
    }

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        actual[as_uindex(j)] = manager.unavailability(j, probs, diagram);
    }

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        BOOST_TEST(
            expected[as_uindex(j)] == actual[as_uindex(j)],
            boost::test_tools::tolerance(FloatingTolerance)
        );
    }

    manager.calculate_probabilities(probs, diagram);
    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        actual[as_uindex(j)] = manager.get_unavailability(j);
    }

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        BOOST_TEST(
            actual[as_uindex(j)] == expected[as_uindex(j)],
            boost::test_tools::tolerance(FloatingTolerance)
        );
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(states_frequency, Fixture, Fixtures, Fixture)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager       = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto const diagram = make_diagram(expr, manager);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);
    auto expected      = std::vector<double>(as_uindex(Fixture::stateCount_));
    auto actual        = std::vector<double>(as_uindex(Fixture::stateCount_));

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        expected[as_uindex(j)] = state_frequency(table, j);
    }

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        actual[as_uindex(j)] = manager.state_frequency(diagram, j);
    }

    for (auto j = 0; j < Fixture::stateCount_; ++j)
    {
        BOOST_TEST(
            actual[as_uindex(j)] == expected[as_uindex(j)],
            boost::test_tools::tolerance(FloatingTolerance)
        );
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(basic_dpld, Fixture, Fixtures, Fixture)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager       = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto const diagram = make_diagram(expr, manager);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

    for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
    {
        auto const varDomain = manager.get_domains()[as_uindex(varIndex)];
        auto varChanges      = std::vector<details::Change>();
        auto fChanges        = std::vector<details::Change>();

        for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom)
        {
            for (auto varTo = varFrom + 1; varTo < varDomain; ++varTo)
            {
                varChanges.push_back(details::Change {varFrom, varTo});
            }
        }
        details::add_opposite_directions(varChanges);

        for (auto fFrom = 0; fFrom < Fixture::stateCount_ - 1; ++fFrom)
        {
            for (auto fTo = fFrom; fTo < Fixture::stateCount_; ++fTo)
            {
                fChanges.push_back(details::Change {fFrom, fTo});
            }
        }
        details::add_opposite_directions(fChanges);

        for (auto const& varChange : varChanges)
        {
            for (auto const& fChange : fChanges)
            {
                auto tableDpld = tsl::dpld(
                    table,
                    tsl::var_change {varIndex, varChange.from, varChange.to},
                    tsl::dpld_basic(fChange.from, fChange.to)
                );
                auto tableDpldExtended = tsl::dpld_e(
                    table,
                    tsl::var_change {varIndex, varChange.from, varChange.to},
                    tsl::dpld_basic(fChange.from, fChange.to)
                );

                auto const diagramDpld = manager.dpld(
                    {varChange.from, varChange.to},
                    {fChange.from, fChange.to},
                    diagram,
                    varIndex
                );
                auto const diagramDpldExtended
                    = manager.to_dpld_e(varChange.from, varIndex, diagramDpld);
                auto const oneCount = manager.satisfy_count(1, diagramDpld);

                BOOST_TEST_MESSAGE(fmt::format(
                    "Basic dpld f({} -> {}) / x{}({} -> {})",
                    fChange.from,
                    fChange.to,
                    varIndex,
                    varChange.from,
                    varChange.to
                ));
                BOOST_TEST_MESSAGE(fmt::format("One count = {}", oneCount));
                BOOST_REQUIRE(
                    details::compare_dplds(manager, tableDpld, diagramDpld)
                );
                BOOST_REQUIRE(details::compare_dplds(
                    manager, tableDpldExtended, diagramDpldExtended
                ));
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(integrated_dpld_1, Fixture, Fixtures, Fixture)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager       = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto const diagram = make_diagram(expr, manager);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

    for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
    {
        auto const varDomain = manager.get_domains()[as_uindex(varIndex)];
        auto varChanges      = std::vector<details::Change>();

        for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom)
        {
            for (auto varTo = varFrom + 1; varTo < varDomain; ++varTo)
            {
                varChanges.push_back(details::Change {varFrom, varTo});
            }
        }
        details::add_opposite_directions(varChanges);

        for (auto fValue = 0; fValue < Fixture::stateCount_ - 1; ++fValue)
        {
            for (auto const& varChange : varChanges)
            {
                auto tableDpldDecrease = tsl::dpld(
                    table,
                    tsl::var_change {varIndex, varChange.from, varChange.to},
                    tsl::dpld_i_1_decrease(fValue + 1)
                );
                auto tableDpldDecreaseExtended = tsl::dpld_e(
                    table,
                    tsl::var_change {varIndex, varChange.from, varChange.to},
                    tsl::dpld_i_1_decrease(fValue + 1)
                );
                auto tableDpldIncrease = tsl::dpld(
                    table,
                    tsl::var_change {varIndex, varChange.from, varChange.to},
                    tsl::dpld_i_1_increase(fValue)
                );
                auto tableDpldIncreaseExtended = tsl::dpld_e(
                    table,
                    tsl::var_change {varIndex, varChange.from, varChange.to},
                    tsl::dpld_i_1_increase(fValue)
                );

                auto const diagramDpldDecrease = manager.idpld_type_1_decrease(
                    {varChange.from, varChange.to},
                    fValue + 1,
                    diagram,
                    varIndex
                );
                auto const diagramDpldDecreaseExtended = manager.to_dpld_e(
                    varChange.from, varIndex, diagramDpldDecrease
                );
                auto const diagramDpldIncrease = manager.idpld_type_1_increase(
                    {varChange.from, varChange.to}, fValue, diagram, varIndex
                );
                auto const diagramDpldIncreaseExtended = manager.to_dpld_e(
                    varChange.from, varIndex, diagramDpldIncrease
                );
                auto const oneCountDecrease
                    = manager.satisfy_count(1, diagramDpldDecrease);
                auto const oneCountIncrease
                    = manager.satisfy_count(1, diagramDpldIncrease);

                BOOST_TEST_MESSAGE(fmt::format(
                    "idpld_type_1_decrease f({} -> <{}) / x{}({} -> {})",
                    fValue + 1,
                    fValue + 1,
                    varIndex,
                    varChange.from,
                    varChange.to
                ));
                BOOST_TEST_MESSAGE(fmt::format(
                    "idpld_type_1_increase f({} -> >{}) / x{}({} -> {})",
                    fValue,
                    fValue,
                    varIndex,
                    varChange.from,
                    varChange.to
                ));
                BOOST_TEST_MESSAGE(
                    fmt::format("One count decrease = {}", oneCountDecrease)
                );
                BOOST_TEST_MESSAGE(
                    fmt::format("One count increase = {}", oneCountIncrease)
                );
                BOOST_REQUIRE(details::compare_dplds(
                    manager, tableDpldDecrease, diagramDpldDecrease
                ));
                BOOST_REQUIRE(details::compare_dplds(
                    manager,
                    tableDpldDecreaseExtended,
                    diagramDpldDecreaseExtended
                ));
                BOOST_REQUIRE(details::compare_dplds(
                    manager, tableDpldIncrease, diagramDpldIncrease
                ));
                BOOST_REQUIRE(details::compare_dplds(
                    manager,
                    tableDpldIncreaseExtended,
                    diagramDpldIncreaseExtended
                ));
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(integrated_dpld_2, Fixture, Fixtures, Fixture)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager       = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto const diagram = make_diagram(expr, manager);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

    for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
    {
        auto const varDomain = domains[as_uindex(varIndex)];
        auto varChanges      = std::vector<details::Change>();

        for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom)
        {
            for (auto varTo = varFrom + 1; varTo < varDomain; ++varTo)
            {
                varChanges.push_back(details::Change {varFrom, varTo});
            }
        }
        details::add_opposite_directions(varChanges);

        for (auto const& varChange : varChanges)
        {
            auto const tableDpldDecrease = tsl::dpld(
                table,
                tsl::var_change {varIndex, varChange.from, varChange.to},
                tsl::dpld_i_2_decrease()
            );
            auto const tableDpldDecreaseExtended = tsl::dpld_e(
                table,
                tsl::var_change {varIndex, varChange.from, varChange.to},
                tsl::dpld_i_2_decrease()
            );
            auto const tableDpldIncrease = tsl::dpld(
                table,
                tsl::var_change {varIndex, varChange.from, varChange.to},
                tsl::dpld_i_2_increase()
            );
            auto const tableDpldIncreaseExtended = tsl::dpld_e(
                table,
                tsl::var_change {varIndex, varChange.from, varChange.to},
                tsl::dpld_i_2_increase()
            );
            auto const diagramDpldDecrease = manager.idpld_type_2_decrease(
                {varChange.from, varChange.to}, diagram, varIndex
            );
            auto const diagramDpldDecreaseExtended = manager.to_dpld_e(
                varChange.from, varIndex, diagramDpldDecrease
            );
            auto const diagramDpldIncrease = manager.idpld_type_2_increase(
                {varChange.from, varChange.to}, diagram, varIndex
            );
            auto const diagramDpldIncreaseExtended = manager.to_dpld_e(
                varChange.from, varIndex, diagramDpldIncrease
            );
            auto const oneCountDecrease
                = manager.satisfy_count(1, diagramDpldDecrease);
            auto const oneCountIncrease
                = manager.satisfy_count(1, diagramDpldIncrease);

            BOOST_TEST_MESSAGE(fmt::format(
                "idpld_type_2_decrease f( < ) / x{}({} -> {})",
                varIndex,
                varChange.from,
                varChange.to
            ));
            BOOST_TEST_MESSAGE(fmt::format(
                "idpld_type_2_increase f( > ) / x{}({} -> {})",
                varIndex,
                varChange.from,
                varChange.to
            ));
            BOOST_TEST_MESSAGE(
                fmt::format("One count decrease = {}", oneCountDecrease)
            );
            BOOST_TEST_MESSAGE(
                fmt::format("One count increase = {}", oneCountIncrease)
            );
            BOOST_REQUIRE(details::compare_dplds(
                manager, tableDpldDecrease, diagramDpldDecrease
            ));
            BOOST_REQUIRE(details::compare_dplds(
                manager, tableDpldDecreaseExtended, diagramDpldDecreaseExtended
            ));
            BOOST_REQUIRE(details::compare_dplds(
                manager, tableDpldIncrease, diagramDpldIncrease
            ));
            BOOST_REQUIRE(details::compare_dplds(
                manager, tableDpldIncreaseExtended, diagramDpldIncreaseExtended
            ));
        }
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(integrated_dpld_3, Fixture, Fixtures, Fixture)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager       = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto const diagram = make_diagram(expr, manager);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

    for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
    {
        auto const varDomain = manager.get_domains()[as_uindex(varIndex)];
        auto varChanges      = std::vector<details::Change>();

        for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom)
        {
            for (auto varTo = varFrom + 1; varTo < varDomain; ++varTo)
            {
                varChanges.push_back(details::Change {varFrom, varTo});
            }
        }
        details::add_opposite_directions(varChanges);

        for (auto fValue = 1; fValue < Fixture::stateCount_; ++fValue)
        {
            for (auto const& varChange : varChanges)
            {
                auto tableDpldDecrease = tsl::dpld(
                    table,
                    tsl::var_change {varIndex, varChange.from, varChange.to},
                    tsl::dpld_i_3_decrease(fValue)
                );
                auto tableDpldDecreaseExtended = tsl::dpld_e(
                    table,
                    tsl::var_change {varIndex, varChange.from, varChange.to},
                    tsl::dpld_i_3_decrease(fValue)
                );
                auto tableDpldIncrease = tsl::dpld(
                    table,
                    tsl::var_change {varIndex, varChange.from, varChange.to},
                    tsl::dpld_i_3_increase(fValue)
                );
                auto tableDpldIncreaseExtended = tsl::dpld_e(
                    table,
                    tsl::var_change {varIndex, varChange.from, varChange.to},
                    tsl::dpld_i_3_increase(fValue)
                );

                auto const diagramDpldDecrease = manager.idpld_type_3_decrease(
                    {varChange.from, varChange.to}, fValue, diagram, varIndex
                );
                auto const diagramDpldDecreaseExtended = manager.to_dpld_e(
                    varChange.from, varIndex, diagramDpldDecrease
                );
                auto const diagramDpldIncrease = manager.idpld_type_3_increase(
                    {varChange.from, varChange.to}, fValue, diagram, varIndex
                );
                auto const diagramDpldIncreaseExtended = manager.to_dpld_e(
                    varChange.from, varIndex, diagramDpldIncrease
                );
                auto const oneCountDecrease
                    = manager.satisfy_count(1, diagramDpldDecrease);
                auto const oneCountIncrease
                    = manager.satisfy_count(1, diagramDpldIncrease);

                BOOST_TEST_MESSAGE(fmt::format(
                    "idpld_type_3_decrease f(>={} -> <{}) / x{}({} -> {})",
                    fValue,
                    fValue,
                    varIndex,
                    varChange.from,
                    varChange.to
                ));
                BOOST_TEST_MESSAGE(fmt::format(
                    "idpld_type_3_increase f(<{} -> >={}) / x{}({} -> {})",
                    fValue,
                    fValue,
                    varIndex,
                    varChange.from,
                    varChange.to
                ));
                BOOST_TEST_MESSAGE(
                    fmt::format("One count decrease = {}", oneCountDecrease)
                );
                BOOST_TEST_MESSAGE(
                    fmt::format("One count increase = {}", oneCountIncrease)
                );
                BOOST_REQUIRE(details::compare_dplds(
                    manager, tableDpldDecrease, diagramDpldDecrease
                ));
                BOOST_REQUIRE(details::compare_dplds(
                    manager,
                    tableDpldDecreaseExtended,
                    diagramDpldDecreaseExtended
                ));
                BOOST_REQUIRE(details::compare_dplds(
                    manager, tableDpldIncrease, diagramDpldIncrease
                ));
                BOOST_REQUIRE(details::compare_dplds(
                    manager,
                    tableDpldIncreaseExtended,
                    diagramDpldIncreaseExtended
                ));
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(
    structural_importances, Fixture, Fixtures, Fixture
)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager       = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto const diagram = make_diagram(expr, manager);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

    for (auto systemState = 1; systemState < Fixture::stateCount_;
         ++systemState)
    {
        for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
        {
            for (auto varVal = 1; varVal < domains[as_uindex(varIndex)];
                 ++varVal)
            {
                auto const tableDpld = tsl::dpld(
                    table,
                    {varIndex, varVal, varVal - 1},
                    tsl::dpld_i_3_decrease(systemState)
                );
                auto const diagramDpld = manager.idpld_type_3_decrease(
                    {varVal, varVal - 1}, systemState, diagram, varIndex
                );
                auto const expected
                    = tsl::structural_importance(tableDpld, varIndex);
                auto const actual = manager.structural_importance(diagramDpld);
                BOOST_TEST(
                    expected == actual,
                    boost::test_tools::tolerance(FloatingTolerance)
                );
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(
    birnbaum_importances, Fixture, Fixtures, Fixture
)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager       = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto const diagram = make_diagram(expr, manager);
    auto const probs   = make_probabilities(manager, Fixture::rng_);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);

    for (auto systemState = 1; systemState < Fixture::stateCount_;
         ++systemState)
    {
        for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
        {
            for (auto varVal = 1;
                 varVal < manager.get_domains()[as_uindex(varIndex)];
                 ++varVal)
            {
                auto const tableDpld = tsl::dpld_e(
                    table,
                    {varIndex, varVal, varVal - 1},
                    tsl::dpld_i_3_decrease(systemState)
                );
                auto const diagramDpld = manager.idpld_type_3_decrease(
                    {varVal, varVal - 1}, systemState, diagram, varIndex
                );
                auto const expected
                    = tsl::birnbaum_importance(tableDpld, probs);
                auto const actual = manager.birnbaum_importance(
                    probs, {varVal, varVal - 1}, diagramDpld, varIndex
                );
                BOOST_TEST(
                    expected == actual,
                    boost::test_tools::tolerance(FloatingTolerance)
                );
            }
        }
    }
}

// TODO add test for specific systems computed elsewhere

BOOST_AUTO_TEST_SUITE_END()
} // namespace teddy::tests