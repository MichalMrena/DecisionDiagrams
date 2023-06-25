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
}

/**
 *  \brief Fixture base
 */
template<class ManagerSettings, class ExpressionSettings>
struct fixture_base
{
    ManagerSettings managerSettings_;
    ExpressionSettings expressionSettings_;
    std::mt19937_64 rng_;
    int32 stateCount_{};
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
    auto diagram       = make_diagram(expr, manager);
    auto const probs   = make_probabilities(manager, Fixture::rng_);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);
    auto const maxDomain = std::ranges::max(manager.get_domains());
    auto expected        = std::vector<double>(as_uindex(maxDomain));
    auto actual          = std::vector<double>(as_uindex(maxDomain));

    for (auto j = 0; j < maxDomain; ++j)
    {
        expected[as_uindex(j)] = probability(table, probs, j);
    }

    manager.calculate_probabilities(probs, diagram);
    for (auto j = 0; j < maxDomain; ++j)
    {
        actual[as_uindex(j)] = manager.get_probability(j);
    }

    for (auto j = 0; j < maxDomain; ++j)
    {
        BOOST_TEST(
            actual[as_uindex(j)] == expected[as_uindex(j)],
            boost::test_tools::tolerance(FloatingTolerance)
        );
    }

    for (auto j = 0; j < maxDomain; ++j)
    {
        actual[as_uindex(j)] = manager.probability(j, probs, diagram);
    }

    for (auto j = 0; j < maxDomain; ++j)
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
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram = make_diagram(expr, manager);
    auto probs   = make_probabilities(manager, Fixture::rng_);
    auto domains = manager.get_domains();
    auto table   = tsl::truth_table(make_vector(expr, domains), domains);
    auto const maxDomain = std::ranges::max(manager.get_domains());
    auto expected        = std::vector<double>(as_uindex(maxDomain));
    auto actual          = std::vector<double>(as_uindex(maxDomain));

    for (auto j = 0; j < maxDomain; ++j)
    {
        expected[as_uindex(j)] = availability(table, probs, j);
    }

    for (auto j = 0; j < maxDomain; ++j)
    {
        actual[as_uindex(j)] = manager.availability(j, probs, diagram);
    }

    for (auto j = 0; j < maxDomain; ++j)
    {
        BOOST_TEST(
            actual[as_uindex(j)] == expected[as_uindex(j)],
            boost::test_tools::tolerance(FloatingTolerance)
        );
    }

    manager.calculate_probabilities(probs, diagram);
    for (auto j = 0; j < maxDomain; ++j)
    {
        actual[as_uindex(j)] = manager.get_availability(j);
    }

    for (auto j = 0; j < maxDomain; ++j)
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
    auto manager  = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram  = make_diagram(expr, manager);
    auto probs    = make_probabilities(manager, Fixture::rng_);
    auto domains  = manager.get_domains();
    auto table    = tsl::truth_table(make_vector(expr, domains), domains);
    auto const maxDomain = std::ranges::max(manager.get_domains());
    auto expected = std::vector<double>(as_uindex(maxDomain));
    auto actual   = std::vector<double>(as_uindex(maxDomain));

    for (auto j = 0; j < maxDomain; ++j)
    {
        expected[as_uindex(j)] = unavailability(table, probs, j);
    }

    for (auto j = 0; j < maxDomain; ++j)
    {
        actual[as_uindex(j)] = manager.unavailability(j, probs, diagram);
    }

    for (auto j = 0; j < maxDomain; ++j)
    {
        BOOST_TEST(
            expected[as_uindex(j)] == actual[as_uindex(j)],
            boost::test_tools::tolerance(FloatingTolerance)
        );
    }

    manager.calculate_probabilities(probs, diagram);
    for (auto j = 0; j < maxDomain; ++j)
    {
        actual[as_uindex(j)] = manager.get_unavailability(j);
    }

    for (auto j = 0; j < maxDomain; ++j)
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
    auto manager  = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram  = make_diagram(expr, manager);
    auto domains  = manager.get_domains();
    auto table    = tsl::truth_table(make_vector(expr, domains), domains);
    auto const maxDomain = std::ranges::max(manager.get_domains());
    auto expected = std::vector<double>(as_uindex(maxDomain));
    auto actual   = std::vector<double>(as_uindex(maxDomain));

    for (auto j = 0; j < maxDomain; ++j)
    {
        expected[as_uindex(j)] = state_frequency(table, j);
    }

    for (auto j = 0; j < maxDomain; ++j)
    {
        actual[as_uindex(j)] = manager.state_frequency(diagram, j);
    }

    for (auto j = 0; j < maxDomain; ++j)
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
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram = make_diagram(expr, manager);
    auto domains = manager.get_domains();
    auto table   = tsl::truth_table(make_vector(expr, domains), domains);

    for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
    {
        auto const varDomain = manager.get_domains()[as_uindex(varIndex)];
        auto varChanges = std::vector<details::Change>();
        auto fChanges = std::vector<details::Change>();

        for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom)
        {
            for (auto varTo = varFrom + 1; varTo < varDomain; ++varTo)
            {
                varChanges.emplace_back(varFrom, varTo);
            }
        }

        for (auto fFrom = 0; fFrom < Fixture::stateCount_ - 1; ++fFrom)
        {
            for (auto fTo = fFrom; fTo < Fixture::stateCount_; ++fTo)
            {
                fChanges.emplace_back(fFrom, fTo);
            }
        }

        // TODO also consider opposite changes
    }
}

// TODO add test for specific systems computed elsewhere

BOOST_FIXTURE_TEST_CASE_TEMPLATE(dpld, Fixture, Fixtures, Fixture)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram = make_diagram(expr, manager);
    auto domains = manager.get_domains();
    auto table   = tsl::truth_table(make_vector(expr, domains), domains);




    for (auto varIndex = 0; varIndex < manager.get_var_count(); ++varIndex)
    {
        auto const varDomain = manager.get_domains()[as_uindex(varIndex)];
        for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom)
        {
            for (auto varTo = varFrom + 1; varTo < varDomain; ++varTo)
            {
                auto const varChange = tsl::var_change {.index = varIndex, .from = varFrom, .to = varTo};
                auto const varOppositeChange = tsl::var_change {.index = varIndex, .from = varTo, .to = varFrom};
                // Basic DPLD
                for (auto fFrom = 0; fFrom < Fixture::stateCount_ - 1; ++fFrom)
                {
                    for (auto fTo = fFrom; fTo < Fixture::stateCount_; ++fTo)
                    {
                        BOOST_TEST_MESSAGE(fmt::format(
                            "Basic dpld f({} -> {}) / x({} -> {})",
                            fFrom,
                            fTo,
                            varChange.from,
                            varChange.to
                        ));

                        auto tableDpld
                            = tsl::dpld(table, varChange, tsl::dpld_basic(fFrom, fTo));
                        auto diagramDpld = manager.dpld(
                            {varChange.from, varChange.to},
                            {fFrom, fTo},
                            diagram,
                            varChange.index
                        );
                        BOOST_TEST_MESSAGE(
                            fmt::format("One count = {}", tsl::satisfy_count(tableDpld, 1))
                        );
                        BOOST_REQUIRE_MESSAGE(
                            compareDpld(tableDpld, diagramDpld),
                            "Diagram and table produced the same derivative"
                        );
                    }
                }
            }
        }
    }


// old:
    auto const varIndex = std::uniform_int_distribution<int32>(
        0, manager.get_var_count() - 1
    )(Fixture::rng_);
    auto const varDomain = manager.get_domains()[as_uindex(varIndex)];
    auto const varFrom
        = std::uniform_int_distribution<int32>(0, varDomain - 2)(Fixture::rng_);
    auto const varTo = std::uniform_int_distribution<int32>(
        varFrom + 1, manager.get_domains()[as_uindex(varIndex)] - 1
    )(Fixture::rng_);

    auto const varChange
        = tsl::var_change {.index = varIndex, .from = varFrom, .to = varTo};

    auto const varChangeR
        = tsl::var_change {.index = varIndex, .from = varTo, .to = varFrom};

    // Basic DPLD
    {
        auto const ffrom = std::uniform_int_distribution<int32>(
            0, table.get_max_val() - 1
        )(Fixture::rng_);
        auto const fto = std::uniform_int_distribution<int32>(
            ffrom + 1, table.get_max_val()
        )(Fixture::rng_);

        BOOST_TEST_MESSAGE(fmt::format(
            "Basic dpld f({} -> {}) / x({} -> {})",
            ffrom,
            fto,
            varChange.from,
            varChange.to
        ));

        auto tabledpld
            = tsl::dpld(table, varChange, tsl::dpld_basic(ffrom, fto));
        auto diagramdpld = manager.dpld(
            {varChange.from, varChange.to},
            {ffrom, fto},
            diagram,
            varChange.index
        );
        // BOOST_TEST_MESSAGE(
        //     fmt::format("One count = {}", tsl::satisfy_count(tabledpld, 1))
        // );
        BOOST_REQUIRE_MESSAGE(
            compareDpld(tabledpld, diagramdpld),
            "Diagram and table produced the same derivative"
        );
    }

    // Integrated DPLD type I decrease
    {
        auto const j = std::uniform_int_distribution<int32>(
            1, table.get_max_val()
        )(Fixture::rng_);

        BOOST_TEST_MESSAGE(fmt::format(
            "idpld_type_1_decrease f({} -> <{}) / x({} -> {})",
            j,
            j,
            varChangeR.from,
            varChangeR.to
        ));

        auto tabledpld
            = tsl::dpld(table, varChangeR, tsl::dpld_i_1_decrease(j));
        auto diagramdpld = manager.idpld_type_1_decrease(
            {varChangeR.from, varChangeR.to}, j, diagram, varChangeR.index
        );
        BOOST_TEST_MESSAGE(
            fmt::format("One count = {}", tsl::satisfy_count(tabledpld, 1u))
        );
        BOOST_REQUIRE_MESSAGE(
            compareDpld(tabledpld, diagramdpld),
            "Diagram and table produced the same derivative"
        );
    }

    // Integrated DPLD type I increase
    {
        auto fvaldist
            = std::uniform_int_distribution<int32>(0, table.get_max_val() - 1);
        auto const j = fvaldist(Fixture::rng_);

        BOOST_TEST_MESSAGE(fmt::format(
            "idpld_type_1_increase f({} -> >{}) / x({} -> {})",
            j,
            j,
            varChange.from,
            varChange.to
        ));

        auto tabledpld = tsl::dpld(table, varChange, tsl::dpld_i_1_increase(j));
        auto diagramdpld = manager.idpld_type_1_increase(
            {varChange.from, varChange.to}, j, diagram, varChange.index
        );
        BOOST_TEST_MESSAGE(
            fmt::format("One count = {}", tsl::satisfy_count(tabledpld, 1u))
        );
        BOOST_REQUIRE_MESSAGE(
            compareDpld(tabledpld, diagramdpld),
            "Diagram and table produced the same derivative"
        );
    }

    // Integrated DPLD type II decrease
    {
        BOOST_TEST_MESSAGE(fmt::format(
            "idpld_type_2_decrease f( < ) / x({} -> {})",
            varChangeR.from,
            varChangeR.to
        ));

        auto tabledpld = tsl::dpld(table, varChangeR, tsl::dpld_i_2_decrease());
        auto diagramdpld = manager.idpld_type_2_decrease(
            {varChangeR.from, varChangeR.to}, diagram, varChangeR.index
        );
        BOOST_TEST_MESSAGE(
            fmt::format("One count = {}", tsl::satisfy_count(tabledpld, 1u))
        );
        BOOST_REQUIRE_MESSAGE(
            compareDpld(tabledpld, diagramdpld),
            "Diagram and table produced the same derivative"
        );
    }

    // Integrated DPLD type II increase
    {
        BOOST_TEST_MESSAGE(fmt::format(
            "idpld_type_2_increase f( > ) / x({} -> {})",
            varChange.from,
            varChange.to
        ));

        auto tabledpld = tsl::dpld(table, varChange, tsl::dpld_i_2_increase());
        auto diagramdpld = manager.idpld_type_2_increase(
            {varChange.from, varChange.to}, diagram, varChange.index
        );
        BOOST_TEST_MESSAGE(
            fmt::format("One count = {}", tsl::satisfy_count(tabledpld, 1u))
        );
        BOOST_REQUIRE_MESSAGE(
            compareDpld(tabledpld, diagramdpld),
            "Diagram and table produced the same derivative"
        );
    }

    // Integrated DPLD type III decrease
    {
        auto const j = std::uniform_int_distribution<int32>(
            1u, table.get_max_val()
        )(Fixture::rng_);

        BOOST_TEST_MESSAGE(fmt::format(
            "idpld_type_3_decrease f(>={} -> <{}) / x({} -> {})",
            j,
            j,
            varChangeR.from,
            varChangeR.to
        ));

        auto tabledpld
            = tsl::dpld(table, varChangeR, tsl::dpld_i_3_decrease(j));
        auto diagramdpld = manager.idpld_type_3_decrease(
            {varChangeR.from, varChangeR.to}, j, diagram, varChangeR.index
        );
        BOOST_TEST_MESSAGE(
            fmt::format("One count = {}", tsl::satisfy_count(tabledpld, 1u))
        );
        BOOST_REQUIRE_MESSAGE(
            compareDpld(tabledpld, diagramdpld),
            "Diagram and table produced the same derivative"
        );
    }

    // Integrated DPLD type III increase
    {
        auto fvaldist
            = std::uniform_int_distribution<int32>(1u, table.get_max_val());
        auto const j = fvaldist(Fixture::rng_);

        BOOST_TEST_MESSAGE(fmt::format(
            "idpld_type_3_increase f(<{} -> >={}) / x({} -> {})",
            j,
            j,
            varChange.from,
            varChange.to
        ));

        auto tabledpld = tsl::dpld(table, varChange, tsl::dpld_i_3_increase(j));
        auto diagramdpld = manager.idpld_type_3_increase(
            {varChange.from, varChange.to}, j, diagram, varChange.index
        );
        BOOST_TEST_MESSAGE(
            fmt::format("One count = {}", tsl::satisfy_count(tabledpld, 1u))
        );
        BOOST_REQUIRE_MESSAGE(
            compareDpld(tabledpld, diagramdpld),
            "Diagram and table produced the same derivative"
        );
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(
    structural_importances, Fixture, Fixtures, Fixture
)
{
    auto const expr
        = make_expression(Fixture::expressionSettings_, Fixture::rng_);
    auto manager       = make_manager(Fixture::managerSettings_, Fixture::rng_);
    auto diagram       = make_diagram(expr, manager);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);
    auto const maxDomain = std::ranges::max(manager.get_domains());

    for (auto j = 1; j < maxDomain; ++j)
    {
        for (auto i = 0; i < manager.get_var_count(); ++i)
        {
            for (auto s = 1; s < manager.get_domains()[as_uindex(i)]; ++s)
            {
                auto const td = tsl::dpld(
                    table, {i, s, s - 1}, tsl::dpld_i_3_decrease(j)
                );
                auto const dd
                    = manager.idpld_type_3_decrease({s, s - 1}, j, diagram, i);
                auto const expected = tsl::structural_importance(td, i);
                auto const actual   = manager.structural_importance(dd);
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
    auto diagram       = make_diagram(expr, manager);
    auto ps            = make_probabilities(manager, Fixture::rng_);
    auto const domains = manager.get_domains();
    auto const table   = tsl::truth_table(make_vector(expr, domains), domains);
    auto const maxDomain = std::ranges::max(manager.get_domains());

    for (auto j = 1; j < maxDomain; ++j)
    {
        for (auto i = 0; i < manager.get_var_count(); ++i)
        {
            for (auto s = 1; s < manager.get_domains()[as_uindex(i)]; ++s)
            {
                auto const td = tsl::dpld_e(
                    table, {i, s, s - 1}, tsl::dpld_i_3_decrease(j)
                );
                auto const dd
                    = manager.idpld_type_3_decrease({s, s - 1}, j, diagram, i);
                auto const expected = tsl::birnbaum_importance(td, ps);
                auto const actual
                    = manager.birnbaum_importance(ps, {s, s - 1}, dd, i);
                BOOST_TEST(
                    expected == actual,
                    boost::test_tools::tolerance(FloatingTolerance)
                );
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace teddy::tests