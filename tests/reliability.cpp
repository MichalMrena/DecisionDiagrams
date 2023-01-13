#include <iostream>
#include <librog/rog.hpp>
#include <libteddy/teddy.hpp>

#include "common_test_setup.hpp"
#include "table_reliability.hpp"

namespace teddy
{

/**
 *  \brief Tests calculation of system state probabilities.
 */
template<class Settings>
class test_probability : public test_base<Settings>
{
public:
    test_probability(Settings settings)
        : test_base<Settings>("probability", std::move(settings))
    {
    }

protected:
    auto test() -> void override
    {
        auto expr     = make_expression(this->settings(), this->rng());
        auto manager  = make_manager(this->settings(), this->rng());
        auto diagram  = make_diagram(expr, manager);
        auto ps       = make_probabilities(manager, this->rng());
        auto domains  = manager.get_domains();
        auto table    = truth_table(make_vector(expr, domains), domains);
        auto const m  = std::ranges::max(manager.get_domains());
        auto expected = std::vector<double>(m);
        auto actual   = std::vector<double>(m);

        for (auto j = 0; j < m; ++j)
        {
            expected[j] = probability(table, ps, j);
        }

        manager.calculate_probabilities(ps, diagram);
        for (auto j = 0; j < m; ++j)
        {
            actual[j] = manager.get_probability(j);
        }

        for (auto j = 0; j < m; ++j)
        {
            this->assert_equals(actual[j], expected[j], 0.00000001);
        }

        for (auto j = 0; j < m; ++j)
        {
            actual[j] = manager.probability(j, ps, diagram);
        }

        for (auto j = 0; j < m; ++j)
        {
            this->assert_equals(actual[j], expected[j], 0.00000001);
        }
    }
};

/**
 *  \brief Tests calculation of availability.
 */
template<class Settings>
class test_availability : public test_base<Settings>
{
public:
    test_availability(Settings settings)
        : test_base<Settings>("availability", std::move(settings))
    {
    }

protected:
    auto test() -> void override
    {
        auto expr     = make_expression(this->settings(), this->rng());
        auto manager  = make_manager(this->settings(), this->rng());
        auto diagram  = make_diagram(expr, manager);
        auto ps       = make_probabilities(manager, this->rng());
        auto domains  = manager.get_domains();
        auto table    = truth_table(make_vector(expr, domains), domains);
        auto const m  = std::ranges::max(manager.get_domains());
        auto expected = std::vector<double>(m);
        auto actual   = std::vector<double>(m);

        for (auto j = 0; j < m; ++j)
        {
            expected[j] = availability(table, ps, j);
        }

        for (auto j = 0; j < m; ++j)
        {
            actual[j] = manager.availability(j, ps, diagram);
        }

        for (auto j = 0; j < m; ++j)
        {
            this->assert_equals(expected[j], actual[j], 0.00000001);
        }

        manager.calculate_probabilities(ps, diagram);
        for (auto j = 0; j < m; ++j)
        {
            actual[j] = manager.get_availability(j);
        }
    }
};

/**
 *  \brief Tests calculation of unavailability.
 */
template<class Settings>
class test_unavailability : public test_base<Settings>
{
public:
    test_unavailability(Settings settings)
        : test_base<Settings>("unavailability", std::move(settings))
    {
    }

protected:
    auto test() -> void override
    {
        auto expr     = make_expression(this->settings(), this->rng());
        auto manager  = make_manager(this->settings(), this->rng());
        auto diagram  = make_diagram(expr, manager);
        auto ps       = make_probabilities(manager, this->rng());
        auto domains  = manager.get_domains();
        auto table    = truth_table(make_vector(expr, domains), domains);
        auto const m  = std::ranges::max(manager.get_domains());
        auto expected = std::vector<double>(m);
        auto actual   = std::vector<double>(m);

        for (auto j = 0; j < m; ++j)
        {
            expected[j] = unavailability(table, ps, j);
        }

        for (auto j = 0; j < m; ++j)
        {
            actual[j] = manager.unavailability(j, ps, diagram);
        }

        for (auto j = 0; j < m; ++j)
        {
            this->assert_equals(expected[j], actual[j], 0.00000001);
        }

        manager.calculate_probabilities(ps, diagram);
        for (auto j = 0; j < m; ++j)
        {
            actual[j] = manager.get_unavailability(j);
        }
    }
};

/**
 *  \brief Tests calculation of system state frequency.
 */
template<class Settings>
class test_state_frequency : public test_base<Settings>
{
public:
    test_state_frequency(Settings settings)
        : test_base<Settings>("state-frequency", std::move(settings))
    {
    }

protected:
    auto test() -> void override
    {
        auto expr     = make_expression(this->settings(), this->rng());
        auto manager  = make_manager(this->settings(), this->rng());
        auto diagram  = make_diagram(expr, manager);
        auto ps       = make_probabilities(manager, this->rng());
        auto domains  = manager.get_domains();
        auto table    = truth_table(make_vector(expr, domains), domains);
        auto const m  = std::ranges::max(manager.get_domains());
        auto expected = std::vector<double>(m);
        auto actual   = std::vector<double>(m);

        for (auto j = 0; j < m; ++j)
        {
            expected[j] = state_frequency(table, j);
        }

        for (auto j = 0; j < m; ++j)
        {
            actual[j] = manager.state_frequency(diagram, j);
        }

        for (auto j = 0; j < m; ++j)
        {
            this->assert_equals(expected[j], actual[j], 0.00000001);
        }
    }
};

/**
 *  \brief Tests calculation of all types of DPLDs.
 */
template<class Settings>
class test_dpbd : public test_base<Settings>
{
public:
    test_dpbd(Settings settings)
        : test_base<Settings>("dpld", std::move(settings))
    {
    }

protected:
    auto test() -> void override
    {
        using namespace std::string_literals;
        auto expr         = make_expression(this->settings(), this->rng());
        auto manager      = make_manager(this->settings(), this->rng());
        auto diagram      = make_diagram(expr, manager);
        auto ps           = make_probabilities(manager, this->rng());
        auto domains      = manager.get_domains();
        auto table        = truth_table(make_vector(expr, domains), domains);

        auto comparedpbds = [&manager](auto const& tabledpld, auto diagramdpld)
        {
            auto result = true;
            domain_for_each(
                tabledpld,
                [&manager,
                 &result,
                 &diagramdpld](auto const v, auto const& elem)
                {
                    if (v != U)
                    {
                        if (manager.evaluate(diagramdpld, elem) != v)
                        {
                            result = false;
                            return;
                        }
                    }
                }
            );
            return result;
        };

        auto s = [](auto const x)
        {
            return std::to_string(x);
        };

        auto const varindex = std::uniform_int_distribution<int32>(
            0, static_cast<int32>(manager.get_var_count() - 1)
        )(this->rng());
        auto const vardomain = manager.get_domains()[varindex];
        auto const varfrom =
            std::uniform_int_distribution<int32>(0, vardomain - 2)(this->rng()
            );
        auto const varto = std::uniform_int_distribution<int32>(
            varfrom + 1, manager.get_domains()[varindex] - 1
        )(this->rng());

        auto const varchange =
            var_change {.index = varindex, .from = varfrom, .to = varto};

        auto const varchanger =
            var_change {.index = varindex, .from = varto, .to = varfrom};

        // Basic DPLD
        {
            auto const ffrom = std::uniform_int_distribution<int32>(
                0, table.get_max_val() - 1
            )(this->rng());
            auto const fto = std::uniform_int_distribution<int32>(
                ffrom + 1, table.get_max_val()
            )(this->rng());

            this->info(
                "Basic dpld f(" + s(ffrom) + " -> " + s(fto) + ") / " + "x(" +
                s(varchange.from) + " -> " + s(varchange.to) + ")"
            );

            auto tabledpld   = dpld(table, varchange, dpld_basic(ffrom, fto));
            auto diagramdpld = manager.dpld(
                {varchange.from, varchange.to},
                {ffrom, fto},
                diagram,
                varchange.index
            );
            this->info("One count = " + s(satisfy_count(tabledpld, 1u)));
            this->assert_true(
                comparedpbds(tabledpld, diagramdpld),
                "Diagram and table produced the same derivative"
            );
        }

        // Integrated DPLD type I decrease
        {
            auto const j = std::uniform_int_distribution<int32>(
                1u, table.get_max_val()
            )(this->rng());

            this->info(
                "idpld_type_1_decrease f(" + s(j) + " -> " + "<" + s(j) +
                ") / x(" + s(varchanger.from) + " -> " + s(varchanger.to) + ")"
            );

            auto tabledpld   = dpld(table, varchanger, dpld_i_1_decrease(j));
            auto diagramdpld = manager.idpld_type_1_decrease(
                {varchanger.from, varchanger.to}, j, diagram, varchanger.index
            );
            this->info("One count = " + s(satisfy_count(tabledpld, 1u)));
            this->assert_true(
                comparedpbds(tabledpld, diagramdpld),
                "Diagram and table produced the same derivative"
            );
        }

        // Integrated DPLD type I increase
        {
            auto fvaldist = std::uniform_int_distribution<int32>(
                0, table.get_max_val() - 1
            );
            auto const j = fvaldist(this->rng());

            this->info(
                "idpld_type_1_increase f(" + s(j) + " -> " + ">" + s(j) +
                ") / x(" + s(varchange.from) + " -> " + s(varchange.to) + ")"
            );

            auto tabledpld   = dpld(table, varchange, dpld_i_1_increase(j));
            auto diagramdpld = manager.idpld_type_1_increase(
                {varchange.from, varchange.to}, j, diagram, varchange.index
            );
            this->info("One count = " + s(satisfy_count(tabledpld, 1u)));
            this->assert_true(
                comparedpbds(tabledpld, diagramdpld),
                "Diagram and table produced the same derivative"
            );
        }

        // Integrated DPLD type II decrease
        {
            this->info(
                "idpld_type_2_decrease f( < ) / x(" + s(varchanger.from) +
                " -> " + s(varchanger.to) + ")"
            );

            auto tabledpld   = dpld(table, varchanger, dpld_i_2_decrease());
            auto diagramdpld = manager.idpld_type_2_decrease(
                {varchanger.from, varchanger.to}, diagram, varchanger.index
            );
            this->info("One count = " + s(satisfy_count(tabledpld, 1u)));
            this->assert_true(
                comparedpbds(tabledpld, diagramdpld),
                "Diagram and table produced the same derivative"
            );
        }

        // Integrated DPLD type II increase
        {
            this->info(
                "idpld_type_2_increase f( > ) / x(" + s(varchange.from) +
                " -> " + s(varchange.to) + ")"
            );

            auto tabledpld   = dpld(table, varchange, dpld_i_2_increase());
            auto diagramdpld = manager.idpld_type_2_increase(
                {varchange.from, varchange.to}, diagram, varchange.index
            );
            this->info("One count = " + s(satisfy_count(tabledpld, 1u)));
            this->assert_true(
                comparedpbds(tabledpld, diagramdpld),
                "Diagram and table produced the same derivative"
            );
        }

        // Integrated DPLD type III decrease
        {
            auto const j = std::uniform_int_distribution<int32>(
                1u, table.get_max_val()
            )(this->rng());

            this->info(
                "idpld_type_3_decrease f(>=" + s(j) + " -> " + "<" + s(j) +
                ") / x(" + s(varchanger.from) + " -> " + s(varchanger.to) + ")"
            );

            auto tabledpld   = dpld(table, varchanger, dpld_i_3_decrease(j));
            auto diagramdpld = manager.idpld_type_3_decrease(
                {varchanger.from, varchanger.to}, j, diagram, varchanger.index
            );
            this->info("One count = " + s(satisfy_count(tabledpld, 1u)));
            this->assert_true(
                comparedpbds(tabledpld, diagramdpld),
                "Diagram and table produced the same derivative"
            );
        }

        // Integrated DPLD type III increase
        {
            auto fvaldist =
                std::uniform_int_distribution<int32>(1u, table.get_max_val());
            auto const j = fvaldist(this->rng());

            this->info(
                "idpld_type_3_increase f(<" + s(j) + " -> " + ">=" + s(j) +
                ") / x(" + s(varchange.from) + " -> " + s(varchange.to) + ")"
            );

            auto tabledpld   = dpld(table, varchange, dpld_i_3_increase(j));
            auto diagramdpld = manager.idpld_type_3_increase(
                {varchange.from, varchange.to}, j, diagram, varchange.index
            );
            this->info("One count = " + s(satisfy_count(tabledpld, 1u)));
            this->assert_true(
                comparedpbds(tabledpld, diagramdpld),
                "Diagram and table produced the same derivative"
            );
        }
    }
};

template<class Settings>
class test_structural_importance : public test_base<Settings>
{
public:
    test_structural_importance(Settings settings)
        : test_base<Settings>("structural-imporatnce", std::move(settings))
    {
    }

protected:
    auto test() -> void override
    {
        auto expr     = make_expression(this->settings(), this->rng());
        auto manager  = make_manager(this->settings(), this->rng());
        auto diagram  = make_diagram(expr, manager);
        auto domains  = manager.get_domains();
        auto table    = truth_table(make_vector(expr, domains), domains);
        auto const m  = std::ranges::max(manager.get_domains());

        for (auto j = 1; j < m; ++j)
        {
            for (auto i = 0; i < manager.get_var_count(); ++i)
            {
                for (auto s = 1; s < manager.get_domains()[i]; ++s)
                {
                    auto const td = dpld(table, {i, s, s - 1}, dpld_i_3_decrease(j));
                    auto const dd = manager.idpld_type_3_decrease({s, s - 1}, j, diagram, i);
                    auto const expected = structural_importance(td, i);
                    auto const actual = manager.structural_importance(dd);
                    this->assert_equals(expected, actual, 0.00000001);
                }
            }
        }
    }
};

/**
 *  \brief Composite test for reliability manager.
 */
template<class ManagerSettings, class ExpressionSettings>
class test_reliability_manager : public rog::CompositeTest
{
public:
    test_reliability_manager(
        std::size_t const seed,
        ManagerSettings manager,
        ExpressionSettings expr,
        std::string name
    )
        : rog::CompositeTest(std::move(name))
    {
        auto seeder      = std::mt19937_64(seed);
        using settings_t = test_settings<ManagerSettings, ExpressionSettings>;

        this->add_test(std::make_unique<test_probability<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_availability<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_unavailability<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_state_frequency<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_dpbd<settings_t>>(settings_t {
            seeder(), manager, expr}));

        this->add_test(std::make_unique<test_structural_importance<settings_t>>(settings_t {
            seeder(), manager, expr}));
    }
};

/**
 *  \brief Tests bss_manager.
 */
class test_bss_manager : public test_reliability_manager<
                             bss_manager_settings,
                             expression_tree_settings>
{
public:
    test_bss_manager(std::size_t const seed)
        : test_reliability_manager<
              bss_manager_settings,
              expression_tree_settings>(
              seed,
              bss_manager_settings {21, 2'000, random_order_tag()},
              expression_tree_settings {},
              "bss_manager"
          )
    {
    }
};

/**
 *  \brief Tests mss_manager.
 */
template<unsigned int M>
class test_mss_manager : public test_reliability_manager<
                             mss_manager_settings<M>,
                             expression_tree_settings>
{
public:
    test_mss_manager(std::size_t const seed)
        : test_reliability_manager<
              mss_manager_settings<M>,
              expression_tree_settings>(
              seed,
              mss_manager_settings<M> {15, 5'000, random_order_tag()},
              expression_tree_settings {},
              "mss_manager"
          )
    {
    }
};

/**
 *  \brief Tests imss_manager.
 */
template<unsigned int M>
class test_imss_manager : public test_reliability_manager<
                              imss_manager_settings<M>,
                              expression_tree_settings>
{
public:
    test_imss_manager(std::size_t const seed)
        : test_reliability_manager<
              imss_manager_settings<M>,
              expression_tree_settings>(
              seed,
              imss_manager_settings<M> {
                  15, 5'000, random_order_tag(), random_domains()},
              expression_tree_settings {},
              "imss_manager"
          )
    {
    }
};

/**
 *  \brief Tests imss_manager.
 */
template<unsigned int M>
class test_ifmss_manager : public test_reliability_manager<
                               ifmss_manager_settings<M>,
                               expression_tree_settings>
{
public:
    test_ifmss_manager(std::size_t const seed)
        : test_reliability_manager<
              ifmss_manager_settings<M>,
              expression_tree_settings>(
              seed,
              ifmss_manager_settings<M> {
                  15, 5'000, random_order_tag(), random_domains()},
              expression_tree_settings {},
              "ifmss_manager"
          )
    {
    }
};

} // namespace teddy

auto run_test_one(std::size_t const seed)
{
    auto const M = 3;

    auto bssmt   = teddy::test_bss_manager(seed);
    bssmt.run();
    rog::console_print_results(bssmt, rog::ConsoleOutputType::NoLeaf);

    auto mssmt = teddy::test_mss_manager<M>(seed);
    mssmt.run();
    rog::console_print_results(mssmt, rog::ConsoleOutputType::NoLeaf);

    auto imssmt = teddy::test_imss_manager<M>(seed);
    imssmt.run();
    rog::console_print_results(imssmt, rog::ConsoleOutputType::NoLeaf);

    auto ifmssmt = teddy::test_ifmss_manager<M>(seed);
    ifmssmt.run();
    rog::console_print_results(ifmssmt, rog::ConsoleOutputType::NoLeaf);
}

auto main(int const argc, char** const argv) -> int
{
    auto seed = 144ull;

    if (argc == 1)
    {
        std::cout << "./[bin] [one|many] {seed}" << '\n';
        return 1;
    }

    if (argc > 2)
    {
        auto const seedopt = teddy::utils::parse<std::size_t>(argv[2]);
        seed               = seedopt ? *seedopt : seed;
    }

    std::cout << "Seed is " << seed << '\n';

    if (argc > 1 && std::string_view(argv[1]) == "one")
    {
        run_test_one(seed);
    }
    else if (argc > 1 && std::string_view(argv[1]) == "many")
    {
        // run_test_many(seed);
    }
    else
    {
        // run_test_one(seed);
        // run_test_many(seed);
    }

    std::cout << '\n' << "End of main." << '\n';
    return 0;
}