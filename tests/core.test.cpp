#define LIBTEDDY_VERBOSE
#undef LIBTEDDY_VERBOSE
// #define NDEBUG

#include <libteddy/core.hpp>

#include <libtsl/expressions.hpp>
#include <libtsl/iterators.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <ranges>
#include <span>
#include <vector>

#include <fmt/format.h>
#include <librog/rog.hpp>

#ifdef LIBTEDDY_TESTS_USE_OMP
#    include <mutex>
#    include <omp.h>
#    include <thread>
#endif

#include "setup.hpp"

namespace teddy::tests
{
/**
 *  \brief Calculates frequency table for each possible value of @p expr .
 */
template<class Dat, class Deg, class Dom>
auto expected_counts (
    diagram_manager<Dat, Deg, Dom>& manager, tsl::minmax_expr const& expr
)
{
    auto counts   = std::vector<int64>();
    auto domainit = tsl::domain_iterator(manager.get_domains());
    auto evalit   = tsl::evaluating_iterator(domainit, expr);
    auto evalend  = tsl::evaluating_iterator_sentinel();
    while (evalit != evalend)
    {
        auto const value = *evalit;
        if (value >= ssize(counts))
        {
            counts.resize(as_usize(value + 1), 0);
        }
        ++counts[as_uindex(value)];
        ++evalit;
    }
    return counts;
}

/**
 *  \brief Implements brute-force evaluation of the entire domain.
 */
template<class Settings>
class evaluating_test : public test_base<Settings>
{
public:
    evaluating_test(std::string name, Settings settings) :
        test_base<Settings>(std::move(name), std::move(settings))
    {
    }

protected:
    auto compare_eval (auto evalit, auto& manager, auto& diagram) -> void
    {
        auto evalend = tsl::evaluating_iterator_sentinel();
        while (evalit != evalend)
        {
            auto const expectedval = *evalit;
            auto const diagramval
                = manager.evaluate(diagram, evalit.get_var_vals());
            if (expectedval != diagramval)
            {
                this->fail(
                    fmt::format("Exptected {} got {}", expectedval, diagramval)
                );
                break;
            }
            ++evalit;
        }

        if (evalit == evalend)
        {
            this->pass("OK, expected == actual.");
        }
    }
};

/**
 *  \brief Tests the evaluate algorithm.
 */
template<class Settings>
class test_evaluate : public evaluating_test<Settings>
{
public:
    test_evaluate(Settings settings) :
        evaluating_test<Settings>("evaluate", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr    = make_expression(this->settings(), this->rng());
        auto manager = make_manager(this->settings(), this->rng());
        auto diagram = make_diagram(expr, manager);
        this->info(fmt::format("Node count {}", manager.node_count(diagram)));
        auto domainit = make_domain_iterator(manager);
        auto evalit   = tsl::evaluating_iterator(domainit, expr);
        this->compare_eval(evalit, manager, diagram);
    }
};

/**
 *  \brief Tests tree fold and left fold algorithms.
 */
template<class Settings>
class test_fold : public test_base<Settings>
{
public:
    test_fold(Settings settings) :
        test_base<Settings>("fold", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr     = make_expression(this->settings(), this->rng());
        auto manager  = make_manager(this->settings(), this->rng());
        auto diagram1 = make_diagram(expr, manager, fold_type::Left);
        auto diagram2 = make_diagram(expr, manager, fold_type::Tree);
        this->info(fmt::format("Node count {}", manager.node_count(diagram1)));
        this->assert_true(diagram1.equals(diagram2), "Diagrams are equal");
    }
};

/**
 *  \brief Tests tree fold and left fold.
 */
template<class Settings>
class test_gc : public test_base<Settings>
{
public:
    test_gc(Settings settings) : test_base<Settings>("gc", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr     = make_expression(this->settings(), this->rng());
        auto manager  = make_manager(this->settings(), this->rng());
        auto diagram1 = make_diagram(expr, manager, fold_type::Left);
        auto diagram2 = make_diagram(expr, manager, fold_type::Tree);
        this->info(fmt::format("Node count {}", manager.node_count(diagram1)));
        manager.force_gc();
        auto const expected = manager.node_count(diagram1);
        auto const actual   = manager.node_count();
        this->assert_equals(expected, actual);
    }
};

/**
 *  \brief Tests satisfy-count algorithm.
 */
template<class Settings>
class test_satisfy_count : public test_base<Settings>
{
public:
    test_satisfy_count(Settings settings) :
        test_base<Settings>("satisfy-count", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr    = make_expression(this->settings(), this->rng());
        auto manager = make_manager(this->settings(), this->rng());
        auto diagram = make_diagram(expr, manager);
        this->info(fmt::format("Node count {}", manager.node_count(diagram)));
        auto expected = expected_counts(manager, expr);
        auto actual   = std::vector<int64>(expected.size(), 0);

        for (auto j = 0; j < ssize(actual); ++j)
        {
            actual[as_uindex(j)] = manager.satisfy_count(j, diagram);
        }

        for (auto k = 0; k < ssize(actual); ++k)
        {
            this->assert_equals(actual[as_uindex(k)], expected[as_uindex(k)]);
        }
    }
};

/**
 *  \brief Tests satisfy-all algorithm.
 */
template<class Settings>
class test_satisfy_all : public test_base<Settings>
{
public:
    test_satisfy_all(Settings settings) :
        test_base<Settings>("satisfy-all", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr    = make_expression(this->settings(), this->rng());
        auto manager = make_manager(this->settings(), this->rng());
        auto diagram = make_diagram(expr, manager);
        this->info(fmt::format("Node count {}", manager.node_count(diagram)));
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
            manager.template satisfy_all_g<out_var_vals>(k, diagram, out);
        }

        for (auto k = 0; k < ssize(actual); ++k)
        {
            this->assert_equals(actual[as_uindex(k)], expected[as_uindex(k)]);
        }
    }
};

/**
 *  \brief Tests neutral and absorbing elements of different operators.
 */
template<class Settings>
class test_operators : public test_base<Settings>
{
public:
    test_operators(Settings settings) :
        test_base<Settings>("operators", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        using namespace teddy::ops;
        auto expr    = make_expression(this->settings(), this->rng());
        auto manager = make_manager(this->settings(), this->rng());
        auto diagram = make_diagram(expr, manager);
        this->info(fmt::format("Node count {}", manager.node_count(diagram)));
        auto const zero = manager.constant(0);
        auto const one  = manager.constant(1);
        auto const sup
            = manager.constant(std::ranges::max(manager.get_domains()));
        auto const boolValDiagram = manager.transform(diagram, utils::not_zero);

        this->assert_true(
            manager.template apply<AND>(boolValDiagram, zero).equals(zero), "AND absorbing"
        );

        this->assert_true(
            manager.template apply<AND>(boolValDiagram, one).equals(boolValDiagram), "AND neutral"
        );

        this->assert_true(
            manager.template apply<OR>(boolValDiagram, one).equals(one), "OR absorbing"
        );

        this->assert_true(
            manager.template apply<OR>(boolValDiagram, zero).equals(boolValDiagram), "OR neutral"
        );

        this->assert_true(
            manager.template apply<XOR>(boolValDiagram, boolValDiagram).equals(zero), "XOR annihilate"
        );

        this->assert_true(
            manager.template apply<MULTIPLIES<2>>(boolValDiagram, zero).equals(zero),
            "MULTIPLIES absorbing"
        );

        this->assert_true(
            manager.template apply<MULTIPLIES<4>>(boolValDiagram, one).equals(boolValDiagram),
            "MULTIPLIES neutral"
        );

        this->assert_true(
            manager.template apply<PLUS<4>>(boolValDiagram, zero).equals(boolValDiagram), "PLUS neutral"
        );

        this->assert_true(
            manager.template apply<EQUAL_TO>(boolValDiagram, boolValDiagram).equals(one),
            "EQUAL_TO annihilate"
        );

        this->assert_true(
            manager.template apply<NOT_EQUAL_TO>(boolValDiagram, boolValDiagram).equals(zero),
            "NOT_EQUAL_TO annihilate"
        );

        this->assert_true(
            manager.template apply<LESS>(boolValDiagram, boolValDiagram).equals(zero), "LESS annihilate"
        );

        this->assert_true(
            manager.template apply<GREATER>(boolValDiagram, boolValDiagram).equals(zero),
            "GREATER annihilate"
        );

        this->assert_true(
            manager.template apply<LESS_EQUAL>(boolValDiagram, boolValDiagram).equals(one),
            "LESS_EQUAL annihilate"
        );

        this->assert_true(
            manager.template apply<GREATER_EQUAL>(boolValDiagram, boolValDiagram).equals(one),
            "GREATER_EQUAL annihilate"
        );

        this->assert_true(
            manager.template apply<MIN>(boolValDiagram, zero).equals(zero), "MIN absorbing"
        );

        this->assert_true(
            manager.template apply<MIN>(boolValDiagram, sup).equals(boolValDiagram), "MIN neutral"
        );

        this->assert_true(
            manager.template apply<MAX>(boolValDiagram, sup).equals(sup), "MAX absoring"
        );

        this->assert_true(
            manager.template apply<MAX>(boolValDiagram, zero).equals(boolValDiagram), "MAX neutral"
        );
    }
};

/**
 *  \brief Tests cofactor algorithm.
 */
template<class Settings>
class test_cofactor : public evaluating_test<Settings>
{
public:
    test_cofactor(Settings settings) :
        evaluating_test<Settings>("cofactor", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr    = make_expression(this->settings(), this->rng());
        auto manager = make_manager(this->settings(), this->rng());
        auto diagram = make_diagram(expr, manager);
        this->info(fmt::format("Node count {}", manager.node_count(diagram)));
        auto const maxIndex = manager.get_var_count() - 1;
        auto indexDist = std::uniform_int_distribution<int32>(0, maxIndex);
        auto const index1 = indexDist(this->rng());
        auto const index2 = [this, &indexDist, index1] ()
        {
            for (;;)
            {
                // I know ... but should be ok...
                auto const randomIndex = indexDist(this->rng());
                if (randomIndex != index1)
                {
                    return randomIndex;
                }
            }
        }();
        auto const value1 = int32 {0};
        auto const value2 = int32 {1};
        auto const intermediateDiagram = manager.cofactor(diagram, index1, value1);
        auto const cofactoredDiagram = manager.cofactor(intermediateDiagram, index2, value2);

        auto domainIt = tsl::domain_iterator(
            manager.get_domains(),
            manager.get_order(),
            {std::make_pair(index1, value1), std::make_pair(index2, value2)}
        );
        auto evalIt = tsl::evaluating_iterator(domainIt, expr);
        this->compare_eval(evalIt, manager, diagram);
    }
};

/**
 *  \brief Tests one time var-sift algorithm.
 */
template<class Settings>
class test_one_var_sift : public evaluating_test<Settings>
{
public:
    test_one_var_sift(Settings settings) :
        evaluating_test<Settings>("one-var-sift", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr    = make_expression(this->settings(), this->rng());
        auto manager = make_manager(this->settings(), this->rng());
        auto diagram = make_diagram(expr, manager);
        this->info(
            fmt::format("Node count before {}", manager.node_count(diagram))
        );
        manager.force_gc();
        manager.force_reorder();
        manager.force_gc();
        auto const actual   = manager.node_count();
        auto const expected = manager.node_count(diagram);
        this->info(fmt::format("Node count after {}", actual));
        this->assert_equals(expected, actual);

        auto domainit = make_domain_iterator(manager);
        auto evalit   = tsl::evaluating_iterator(domainit, expr);
        this->compare_eval(evalit, manager, diagram);
    }
};

/**
 *  \brief Tests automatic var-sift algorithm.
 */
template<class Settings>
class test_auto_var_sift : public evaluating_test<Settings>
{
public:
    test_auto_var_sift(Settings settings) :
        evaluating_test<Settings>("auto-var-sift", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr    = make_expression(this->settings(), this->rng());
        auto manager = make_manager(this->settings(), this->rng());
        manager.set_auto_reorder(true);
        auto diagram = make_diagram(expr, manager);
        manager.force_gc();
        auto const actual   = manager.node_count();
        auto const expected = manager.node_count(diagram);

        this->assert_equals(expected, actual);

        auto domainit = make_domain_iterator(manager);
        auto evalit   = tsl::evaluating_iterator(domainit, expr);
        this->compare_eval(evalit, manager, diagram);
    }
};

/**
 *  \brief Tests from-vector algorithm.
 */
template<class Settings>
class test_from_vector : public test_base<Settings>
{
public:
    test_from_vector(Settings settings) :
        test_base<Settings>("from-vector", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr    = make_expression(this->settings(), this->rng());
        auto manager = make_manager(this->settings(), this->rng());
        auto diagram = make_diagram(expr, manager);
        this->info(fmt::format("Node count {}", manager.node_count(diagram)));
        auto domainit = make_domain_iterator(manager);
        auto evalit   = tsl::evaluating_iterator(domainit, expr);
        auto evalend  = tsl::evaluating_iterator_sentinel();
        auto vectord  = manager.from_vector(evalit, evalend);
        this->assert_true(
            diagram.equals(vectord), "From-vector created the same diagram"
        );
    }
};

/**
 *  \brief Tests to-vector algorithm.
 */
template<class Settings>
class test_to_vector : public test_base<Settings>
{
public:
    test_to_vector(Settings settings) :
        test_base<Settings>("to-vector", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto expr    = make_expression(this->settings(), this->rng());
        auto manager = make_manager(this->settings(), this->rng());
        auto diagram = make_diagram(expr, manager);
        this->info(fmt::format("Node count {}", manager.node_count(diagram)));
        auto vector  = manager.to_vector(diagram);
        auto vectord = manager.from_vector(vector);
        this->assert_true(
            diagram.equals(vectord),
            "From-vector from to-vectored vector created the same diagram"
        );
    }
};

/**
 *  \brief Tests from-expression algorithm.
 */
template<class Settings>
class test_from_expression : public evaluating_test<Settings>
{
public:
    test_from_expression(Settings settings) :
        evaluating_test<Settings>("from-expression", std::move(settings))
    {
    }

protected:
    auto test () -> void override
    {
        auto manager  = make_manager(this->settings(), this->rng());
        auto exprtree = tsl::make_expression_tree(
            manager.get_var_count(), this->rng(), this->rng()
        );
        auto diagram  = manager.from_expression_tree(*exprtree);
        auto domainit = tsl::domain_iterator(manager.get_domains());
        auto evalit   = teddy::tsl::evaluating_iterator(domainit, *exprtree);
        this->compare_eval(evalit, manager, diagram);
    }
};

template<class ManagerSettings, class ExpressionSettings>
class test_manager : public rog::CompositeTest
{
public:
    test_manager(
        std::size_t const seed,
        ManagerSettings manager,
        ExpressionSettings expr,
        std::string name
    ) :
        rog::CompositeTest(std::move(name))
    {
        auto seeder      = std::mt19937_64(seed);
        using settings_t = test_settings<ManagerSettings, ExpressionSettings>;

        this->add_test(std::make_unique<test_evaluate<settings_t>>(settings_t {
            seeder(), manager, expr}));

        this->add_test(std::make_unique<test_fold<settings_t>>(settings_t {
            seeder(), manager, expr}));

        this->add_test(std::make_unique<test_gc<settings_t>>(settings_t {
            seeder(), manager, expr}));

        this->add_test(std::make_unique<test_satisfy_count<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_satisfy_all<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_operators<settings_t>>(settings_t {
            seeder(), manager, expr}));

        this->add_test(std::make_unique<test_cofactor<settings_t>>(settings_t {
            seeder(), manager, expr}));

        this->add_test(std::make_unique<test_one_var_sift<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_auto_var_sift<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_from_vector<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));

        this->add_test(std::make_unique<test_to_vector<settings_t>>(settings_t {
            seeder(), manager, expr}));

        this->add_test(std::make_unique<test_from_expression<settings_t>>(
            settings_t {seeder(), manager, expr}
        ));
    }
};

/**
 *  \brief Tests bdd manager.
 */
class test_bdd_manager :
    public test_manager<bdd_manager_settings, minmax_expression_settings>
{
private:
    inline static auto constexpr VarCount = 15;
    inline static auto constexpr NodeCount = 5'000;
    inline static auto constexpr TermCount = 20;
    inline static auto constexpr TermSize = 5;

public:
    test_bdd_manager(std::size_t const seed) :
        test_manager<bdd_manager_settings, minmax_expression_settings>(
            seed,
            bdd_manager_settings {VarCount, NodeCount, random_order_tag()},
            minmax_expression_settings {TermCount, TermSize},
            "bdd_manager"
        )
    {
    }
};

/**
 *  \brief Tests mdd manager.
 */
class test_mdd_manager :
    public test_manager<mdd_manager_settings<3>, minmax_expression_settings>
{
private:
    inline static auto constexpr VarCount = 15;
    inline static auto constexpr NodeCount = 5'000;
    inline static auto constexpr TermCount = 20;
    inline static auto constexpr TermSize = 5;

public:
    test_mdd_manager(std::size_t const seed) :
        test_manager<mdd_manager_settings<3>, minmax_expression_settings>(
            seed,
            mdd_manager_settings<3> {VarCount, NodeCount, random_order_tag()},
            minmax_expression_settings {TermCount, TermSize},
            "mdd_manager"
        )
    {
    }
};

/**
 *  \brief Tests imdd manager.
 */
class test_imdd_manager :
    public test_manager<imdd_manager_settings<3>, minmax_expression_settings>
{
private:
    inline static auto constexpr VarCount = 15;
    inline static auto constexpr NodeCount = 5'000;
    inline static auto constexpr TermCount = 20;
    inline static auto constexpr TermSize = 5;

public:
    test_imdd_manager(std::size_t const seed) :
        test_manager<imdd_manager_settings<3>, minmax_expression_settings>(
            seed,
            imdd_manager_settings<3> {
                {{VarCount, NodeCount, random_order_tag()}, random_domains_tag()}
    },
            minmax_expression_settings {TermCount, TermSize},
            "imdd_manager"
        )
    {
    }
};

/**
 *  \brief Tests ifmdd manager.
 */
class test_ifmdd_manager :
    public test_manager<ifmdd_manager_settings<3>, minmax_expression_settings>
{
private:
    inline static auto constexpr VarCount = 15;
    inline static auto constexpr NodeCount = 5'000;
    inline static auto constexpr TermCount = 20;
    inline static auto constexpr TermSize = 5;

public:
    test_ifmdd_manager(std::size_t const seed) :
        test_manager<ifmdd_manager_settings<3>, minmax_expression_settings>(
            seed,
            ifmdd_manager_settings<3> {
                {{VarCount, NodeCount, random_order_tag()}, random_domains_tag()}
    },
            minmax_expression_settings {TermCount, TermSize},
            "ifmdd_manager"
        )
    {
    }
};
} // namespace teddy::tests

auto run_test_one (std::size_t const seed)
{
    auto bddmt = teddy::tests::test_bdd_manager(seed);
    bddmt.run();
    rog::console_print_results(bddmt);

    auto mddmt = teddy::tests::test_mdd_manager(seed);
    mddmt.run();
    rog::console_print_results(mddmt);

    auto imddmt = teddy::tests::test_imdd_manager(seed);
    imddmt.run();
    rog::console_print_results(imddmt);

    auto ifmddmt = teddy::tests::test_ifmdd_manager(seed);
    ifmddmt.run();
    rog::console_print_results(ifmddmt);
}

auto run_test_many (std::size_t const seed)
{
    auto const result_to_str = [] (auto const result)
    {
        auto const* const Oks = "o";
        auto const* const Ers = "!";

        switch (result)
        {
        case rog::TestResult::Fail:
            return Ers;

        case rog::TestResult::Pass:
            return Oks;

        default:
            return " ";
        }
    };

    auto const print_results = [=] (auto const& tests)
    {
        auto const numtest = tests.front().subtests().size();
        auto const numrep  = tests.size();
        auto const numw    = 1
                        + std::ranges::max(
                              tests.front().subtests()
                              | std::ranges::views::transform(&rog::Test::name)
                              | std::ranges::views::transform(std::ranges::size)
                        );

        std::cout << result_to_str(rog::TestResult::Pass) << " - ok; "
                  << result_to_str(rog::TestResult::Fail) << " - not good"
                  << '\n';
        std::cout << tests.front().name() << '\n';

        for (auto k = 0U; k < numtest; ++k)
        {
            std::cout << " > " << std::left << std::setw(static_cast<int>(numw))
                      << tests.front().subtests()[k]->name() << ' ';
            for (auto rep = 0U; rep < numrep; ++rep)
            {
                std::cout << result_to_str(tests[rep].subtests()[k]->result())
                          << " ";
            }
            std::cout << '\n';
        }
        std::cout << '\n';
    };

#ifdef LIBTEDDY_TESTS_USE_OMP
    auto const numtest = omp_get_num_procs();
#else
    auto const numtest = 10;
#endif

    auto bddmts  = std::vector<teddy::tests::test_bdd_manager>();
    auto mddmts  = std::vector<teddy::tests::test_mdd_manager>();
    auto imddts  = std::vector<teddy::tests::test_imdd_manager>();
    auto ifmddts = std::vector<teddy::tests::test_ifmdd_manager>();
    auto seeder  = std::mt19937_64(seed);

    for (auto k = 0; k < numtest; ++k)
    {
        bddmts.emplace_back(seeder());
        mddmts.emplace_back(seeder());
        imddts.emplace_back(seeder());
        ifmddts.emplace_back(seeder());
    }

#ifdef LIBTEDDY_TESTS_USE_OMP
#    pragma omp parallel for
    for (auto k = 0; k < numtest; ++k)
    {
        bddmts[teddy::as_uindex(k)].run();
    }
    print_results(bddmts);

#    pragma omp parallel for
    for (auto k = 0; k < numtest; ++k)
    {
        mddmts[teddy::as_uindex(k)].run();
    }
    print_results(mddmts);

#    pragma omp parallel for
    for (auto k = 0; k < numtest; ++k)
    {
        imddts[teddy::as_uindex(k)].run();
    }
    print_results(imddts);

#    pragma omp parallel for
    for (auto k = 0; k < numtest; ++k)
    {
        ifmddts[teddy::as_uindex(k)].run();
    }
    print_results(ifmddts);
#else
    for (auto k = 0; k < numtest; ++k)
    {
        bddmts[teddy::as_uindex(k)].run();
    }
    print_results(bddmts);

    for (auto k = 0; k < numtest; ++k)
    {
        mddmts[teddy::as_uindex(k)].run();
    }
    print_results(mddmts);

    for (auto k = 0; k < numtest; ++k)
    {
        imddts[teddy::as_uindex(k)].run();
    }
    print_results(imddts);

    for (auto k = 0; k < numtest; ++k)
    {
        ifmddts[teddy::as_uindex(k)].run();
    }
    print_results(ifmddts);
#endif
}

auto main (int const argc, char** const argv) -> int
{
    auto const args = std::span<char*>(argv, static_cast<std::size_t>(argc));
    auto const defaultSeed = std::size_t {144};
    auto seed = defaultSeed;

    if (argc == 1)
    {
        std::cout << "./[bin] [one|many] {seed}" << '\n';
        return 1;
    }

    if (argc > 2)
    {
        auto const seedopt = teddy::utils::parse<std::size_t>(args[2]);
        seed               = seedopt ? *seedopt : seed;
    }

    std::cout << "Seed is " << seed << '\n';

    if (argc > 1 && std::string_view(args[1]) == "one")
    {
        run_test_one(seed);
    }
    else if (argc > 1 && std::string_view(args[1]) == "many")
    {
        run_test_many(seed);
    }
    else
    {
        run_test_one(seed);
        run_test_many(seed);
    }

    std::cout << '\n' << "End of main." << '\n';
    return 0;
}