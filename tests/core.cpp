#define LIBTEDDY_VERBOSE
#undef LIBTEDDY_VERBOSE
// #define NDEBUG

#include <libteddy/teddy.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <omp.h>
#include <random>
#include <ranges>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include "expressions.hpp"

namespace teddy::test
{
    enum class fold_e
    {
        Left, Tree
    };

    auto wrap_green(std::string_view const s)
    {
        return std::string("\x1B[92m") + std::string(s) + "\x1B[0m";
    }

    auto wrap_red(std::string_view const s)
    {
        return std::string("\x1B[91m") + std::string(s) + "\x1B[0m";
    }

    auto wrap_yellow(std::string_view const s)
    {
        return std::string("\x1B[93m") + std::string(s) + "\x1B[0m";
    }

    auto constexpr char_ok()
    {
        return "✓";
    }

    auto constexpr char_err()
    {
        return "x";
    }

    /**
     *  Describes result of a test.
     */
    class test_result
    {
    public:
        test_result(bool status) :
            status_ (status)
        {
        }

        test_result(bool status, std::string msg) :
            status_ (status),
            msg_    (std::move(msg))
        {
        }

        constexpr operator bool () const
        {
            return status_;
        }

        auto get_status() const
        {
            return status_;
        }

        auto get_message () const
        {
            return std::string_view(msg_);
        }

    private:
        bool        status_;
        std::string msg_;
    };

    auto operator<< (std::ostream& ost, test_result const& t) -> std::ostream&
    {
        using namespace std::string_view_literals;
        if (t)
        {
            ost << wrap_green(char_ok());
        }
        else
        {
            ost << wrap_red(char_err()) << " " << t.get_message();
        }
        return ost;
    }

    /**
     *  Creates diagram representing the same functions as @p expr does.
     */
    template<class Dat, degree Deg, domain Dom>
    auto create_diagram
        ( expr_var const&                 expr
        , diagram_manager<Dat, Deg, Dom>& manager
        , fold_e const                    foldType )
    {
        if (std::holds_alternative<constant_expr>(expr))
        {
            return manager.constant(std::get<constant_expr>(expr).val);
        }

        auto const min_fold = [&manager, foldType](auto& xs)
        {
            return foldType == fold_e::Left
                ? manager.template left_fold<ops::MIN>(xs)
                : manager.template tree_fold<ops::MIN>(xs);
        };

        auto const max_fold = [&manager, foldType](auto& xs)
        {
            return foldType == fold_e::Left
                ? manager.template left_fold<ops::MAX>(xs)
                : manager.template tree_fold<ops::MAX>(xs);
        };

        using diagram_t = typename diagram_manager<Dat, Deg, Dom>::diagram_t;
        auto termDs = std::vector<diagram_t>();
        auto const& ts = std::get<minmax_expr>(expr).terms;
        for (auto const& eTerm : ts)
        {
            auto vars = manager.variables(eTerm);
            termDs.emplace_back(min_fold(vars));
        }

        return max_fold(termDs);
    }

    /**
     *  Tests if @p diagram evaluates to the same value as @p expr .
     */
    template<class Dat, class Deg, class Dom>
    auto test_evaluate
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg> const&        diagram
        , expr_var const&                 expr
        , domain_iterator                 domainIt )
    {
        auto const end = domain_iterator_sentinel();
        auto evalIt    = evaluating_iterator(domainIt, expr);
        while (evalIt != end)
        {
            auto const expectedVal = *evalIt;
            auto const diagramVal  = manager.evaluate( diagram
                                                     , evalIt.var_vals() );
            if (expectedVal != diagramVal)
            {
                return test_result(false, "Value missmatch.");
            }
            ++evalIt;
        }

        if (evalIt == end)
        {
            return test_result(true);
        }

        return test_result(false, "This should not have happened.");
    }

    /**
     *  Tests if @p diagram evaluates to the same value as @p expr .
     */
    template<class Dat, class Deg, class Dom>
    auto test_evaluate
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg> const&        diagram
        , expr_var const&                 expr )
    {
        return test_evaluate( manager
                            , diagram
                            , expr
                            , domain_iterator(manager.get_domains()) );
    }

    /**
     *  Tests if different folds create the same node.
     */
    template<class Dat, class Deg, class Dom>
    auto test_fold
        ( diagram_manager<Dat, Deg, Dom>&
        , diagram<Dat, Deg> const&        diagram1
        , diagram<Dat, Deg> const&        diagram2 )
    {
        if (diagram1.equals(diagram2))
        {
            return test_result(true);
        }
        else
        {
            return test_result(false, "Diagrams are different.");
        }
    }

    /**
     *  Tests if garbage collection collects all nodes except nodes
     *  that are part of @p diagram .
     */
    template<class Dat, class Deg, class Dom>
    auto test_gc
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg> const&        diagram )
    {
        manager.force_gc();
        auto const totalNodeCount   = manager.node_count();
        auto const diagramNodeCount = manager.node_count(diagram);
        if (totalNodeCount == diagramNodeCount)
        {
            return test_result(true);
        }
        else
        {
            return test_result( false, "Node count missmatch. Expected "
                              + std::to_string(diagramNodeCount)
                              + " got "
                              + std::to_string(totalNodeCount) + ".");
        }
    }

    /**
     *  Calculates frequency table for each possible value of @p expr .
     */
    template<class Dat, class Deg, class Dom>
    auto expected_counts
        ( diagram_manager<Dat, Deg, Dom>& manager
        , expr_var const&                 expr )
    {
        auto counts = std::vector<std::size_t>();
        auto const domains = manager.get_domains();
        auto const inc     = [](auto& cs, auto const v)
        {
            if (v >= cs.size())
            {
                cs.resize(v + 1, 0);
            }
            ++cs[v];
        };
        if (domains.empty())
        {
            inc(counts, evaluate_expression(expr, {}));
        }
        else
        {
            auto domainIt  = domain_iterator(domains);
            auto evalIt    = evaluating_iterator(domainIt, expr);
            auto const end = domain_iterator_sentinel();
            while (evalIt != end)
            {
                inc(counts, *evalIt);
                ++evalIt;
            }
        }
        return counts;
    }

    /**
     *  Tests the satisfy_count algorithm.
     */
    template<class Dat, class Deg, class Dom>
    auto test_satisfy_count
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg>&              diagram
        , expr_var const&                 expr )
    {
        auto const domains        = manager.get_domains();
        auto const expectedCounts = expected_counts(manager, expr);
        auto const realCounts     = [&]()
        {
            auto cs = std::vector<std::size_t>(expectedCounts.size(), 0);
            for (auto v = 0u; v < cs.size(); ++v)
            {
                cs[v] = manager.satisfy_count(v, diagram);
            }
            return cs;
        }();

        for (auto k = 0u; k < realCounts.size(); ++k)
        {
            if (realCounts[k] != expectedCounts[k])
            {
                return test_result(false, "Count missmatch.");
            }
        }

        return test_result(true);
    }

    /**
     *  Test the satisfy_all algorithm;
     */
    template<class Dat, class Deg, class Dom>
    auto test_satisfy_all
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg>&              diagram
        , expr_var const&                 expr )
    {
        auto const domains        = manager.get_domains();
        auto const expectedCounts = expected_counts(manager, expr);
        auto const realCounts     = [&]()
        {
            auto vals = std::vector<std::size_t>(expectedCounts.size(), 0);
            for (auto k = 0u; k < expectedCounts.size(); ++k)
            {
                using out_var_vals = std::array<uint_t, 100>;
                auto outF = [&vals, k](auto const&)
                {
                    ++vals[k];
                };
                auto out = forwarding_iterator<decltype(outF)>(outF);
                manager.template satisfy_all_g<out_var_vals>(k, diagram, out);
            }
            return vals;
        }();

        for (auto k = 0u; k < expectedCounts.size(); ++k)
        {
            if (expectedCounts[k] != realCounts[k])
            {
                return test_result(false, "Count missmatch.");
            }
        }
        return test_result(true);
    }

    /**
     *  Tests neutral and absorbing elements of different operators.
     */
    template<class Dat, class Deg, class Dom>
    auto test_operators
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg>&              diagram
        , expr_var const&                 expr )
    {
        using namespace teddy::ops;

        auto const max  = [&expr, &manager]()
        {
            auto domains = manager.get_domains();

            if (domains.empty())
            {
                return evaluate_expression(expr, {});
            }

            auto m = uint_t {0};
            auto domainIt  = domain_iterator(std::move(domains));
            auto evalIt    = evaluating_iterator(domainIt, expr);
            auto const end = domain_iterator_sentinel();
            while (evalIt != end)
            {
                m = std::max(m, *evalIt);
                ++evalIt;
            }
            return m;
        }();
        auto const cs   = expected_counts(manager, expr);
        auto const zero = manager.constant(0);
        auto const one  = manager.constant(1);
        auto const sup  = manager.constant(max);
        auto const bd   = manager.transform(diagram, utils::not_zero);
        // auto const rd   = manager.reduce(diagram);

        if (not manager.template apply<AND>(bd, zero).equals(zero))
        {
            return test_result(false, "AND absorbing failed.");
        }

        if (not manager.template apply<AND>(bd, one).equals(bd))
        {
            return test_result(false, "AND neutral failed.");
        }

        if (not manager.template apply<OR>(bd, one).equals(one))
        {
            return test_result(false, "OR absorbing failed.");
        }

        if (not manager.template apply<OR>(bd, zero).equals(bd))
        {
            return test_result(false, "OR neutral failed.");
        }

        if (not manager.template apply<XOR>(bd, bd).equals(zero))
        {
            return test_result(false, "XOR annihilate failed.");
        }

        if (not manager.template apply<MULTIPLIES<2>>(bd, zero).equals(zero))
        {
            return test_result(false, "MULTIPLIES absorbing failed.");
        }

        if (not manager.template apply<MULTIPLIES<4>>(bd, one).equals(bd))
        {
            return test_result(false, "MULTIPLIES neutral failed.");
        }

        if (not manager.template apply<PLUS<4>>(bd, zero).equals(bd))
        {
            return test_result(false, "PLUS neutral failed.");
        }

        if (not manager.template apply<EQUAL_TO>(bd, bd).equals(one))
        {
            return test_result(false, "EQUAL_TO annihilate failed.");
        }

        if (not manager.template apply<NOT_EQUAL_TO>(bd, bd).equals(zero))
        {
            return test_result(false, "NOT_EQUAL_TO annihilate failed.");
        }

        if (not manager.template apply<LESS>(bd, bd).equals(zero))
        {
            return test_result(false, "LESS annihilate failed.");
        }

        if (not manager.template apply<GREATER>(bd, bd).equals(zero))
        {
            return test_result(false, "GREATER annihilate failed.");
        }

        if (not manager.template apply<LESS_EQUAL>(bd, bd).equals(one))
        {
            return test_result(false, "LESS_EQUAL annihilate failed.");
        }

        if (not manager.template apply<GREATER_EQUAL>(bd, bd).equals(one))
        {
            return test_result(false, "GREATER_EQUAL annihilate failed.");
        }

        if (not manager.template apply<MIN>(bd, zero).equals(zero))
        {
            return test_result(false, "MIN absorbing failed.");
        }

        if (not manager.template apply<MIN>(bd, sup).equals(bd))
        {
            return test_result(false, "MIN neutral failed.");
        }

        if (not manager.template apply<MAX>(bd, sup).equals(sup))
        {
            return test_result(false, "MAX absoring failed.");
        }

        if (not manager.template apply<MAX>(bd, zero).equals(bd))
        {
            return test_result(false, "MAX neutral failed.");
        }

        return test_result(true);
    }

    /**
     *  Tests cofactor algorithm.
     */
    template<class Dat, class Deg, class Dom>
    auto test_cofactor
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg>&              diagram
        , expr_var const&                 expr
        , rng_t&                          rng )
    {
        if (std::holds_alternative<constant_expr>(expr))
        {
            auto const dTmp = manager.cofactor(diagram, 0, 1);
            auto const d    = manager.cofactor(dTmp, 1, 0);
            return test_evaluate(manager, d, expr);
        }
        else
        {
            auto const maxI = static_cast<index_t>(manager.get_var_count() - 1);
            auto indexDist  = int_dist_t<index_t>(0u, maxI);
            auto const i1   = indexDist(rng);
            auto const i2   = [&indexDist, &rng, i1]()
            {
                for (;;)
                {
                    // Potentially dangerous but should be ok...
                    auto const i = indexDist(rng);
                    if (i != i1)
                    {
                        return i;
                    }
                }
            }();
            auto const v1   = uint_t {0};
            auto const v2   = uint_t {1};
            auto const dTmp = manager.cofactor(diagram, i1, v1);
            auto const d    = manager.cofactor(dTmp, i2, v2);

            auto it = domain_iterator
                ( manager.get_domains()
                , manager.get_order()
                , {std::make_pair(i1, v1), std::make_pair(i2, v2)} );
            return test_evaluate(manager, d, expr, std::move(it));
        }
    }

    /**
     *  Tests from vector algorithm.
     */
    template<class Dat, class Deg, class Dom>
    auto test_from_vector
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg>&              diagram
        , expr_var const&                 expr )
    {
        auto const vectorDiagram = [&manager, &expr]()
        {
            if (std::holds_alternative<constant_expr>(expr))
            {
                auto const val = evaluate_expression(expr, {});
                auto const vec = std::vector<uint_t> {val};
                return manager.from_vector(vec);
            }
            else
            {
                auto order    = manager.get_order();
                auto domains  = manager.get_domains();
                std::ranges::reverse(order);
                auto domainIt = domain_iterator( std::move(domains)
                                               , std::move(order) );
                auto evalIt   = evaluating_iterator(domainIt, expr);
                auto end      = domain_iterator_sentinel();
                return manager.from_vector(evalIt, end);
            }
        }();

        if (vectorDiagram.equals(diagram))
        {
            return test_result(true);
        }
        else
        {
            return test_result(false, "From vector created different diagram.");
        }
    }

    /**
     *  Tests variable sifting algorithm.
     */
    template<class Dat, class Deg, class Dom>
    auto test_var_sift
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg>&              diagram
        , expr_var const&                 expr )
    {
        manager.force_gc();
        manager.sift();
        manager.force_gc();
        auto const actualCount = manager.node_count();
        auto const expectedCount = manager.node_count(diagram);

        if (actualCount != expectedCount)
        {
            return test_result(false, "Expected "
                                    + std::to_string(expectedCount)
                                    + " nodes, got "
                                    + std::to_string(actualCount)
                                    + ".");
        }

        return test_evaluate(manager, diagram, expr);
    }

    /**
     *  Tests transformation from diagram to truth vector.
     */
    template<class Dat, class Deg, class Dom>
    auto test_to_vector
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg>&              diagram )
    {
        auto const vector = manager.to_vector(diagram);
        auto const diagram2 = manager.from_vector(vector);
        return diagram.equals(diagram2)
            ? test_result(true)
            : test_result(false, "Diagram created from vector is different.");
    }

    /**
     *  Tests creation of diagra from expression tree.
     */
    template<class Dat, class Deg, class Dom>
    auto test_from_expression
        ( diagram_manager<Dat, Deg, Dom>& manager
        , rng_t&                          rng )
    {
        auto const exprTree = generate_expression_tree(
            manager.get_var_count(),
            rng,
            rng
        );
        auto const diagram = manager.from_expression_tree(*exprTree);
            // manager.to_dot_graph(std::cout, diagram);
        auto it = domain_iterator(manager.get_domains());
        auto const end = domain_iterator_sentinel();
        while (it != end)
        {
            auto const expected = evaluate_expression_tree(*exprTree, *it);
            auto const actual = manager.evaluate(diagram, *it);
            if (expected != actual)
            {
                return test_result( false, "Value missmatch. Expected "
                                         + std::to_string(expected)
                                         + " got "
                                         + std::to_string(actual)
                                         + "." );
            }
            ++it;
        }

        if (it == end)
        {
            return test_result(true);
        }

        return test_result(false, "This should not have happened.");
    }

    /**
     *  Runs all test. Creates diagram represeting @p expr using @p manager .
     */
    template<class Manager>
    auto test_many
        ( std::string_view             name
        , std::vector<Manager>&        managers
        , std::vector<expr_var> const& exprs
        , std::vector<rng_t>&          rngs )
    {
        auto const testCount = managers.size();

        auto diagram1s = utils::fill_vector(testCount, [&](auto const k)
        {
            return create_diagram(exprs[k], managers[k], fold_e::Left);
        });

        auto sizesNotSifted = teddy::utils::fill_vector(diagram1s.size(),
            [&managers, &diagram1s](auto const k)
        {
            return managers[k].node_count(diagram1s[k]);
        });

        for (auto& manager : managers)
        {
            manager.set_auto_reorder(true);
        }

        auto diagram2s = utils::fill_vector(testCount, [&](auto const k)
        {
            return create_diagram(exprs[k], managers[k], fold_e::Tree);
        });

        for (auto k = 0u; k < managers.size(); ++k)
        {
            diagram1s[k] = managers[k].reduce(diagram1s[k]);
            diagram2s[k] = managers[k].reduce(diagram2s[k]);
        }

        auto sizesSifted = teddy::utils::fill_vector(diagram2s.size(),
            [&managers, &diagram2s](auto const k)
        {
            return managers[k].node_count(diagram2s[k]);
        });

        using namespace std::string_view_literals;
        auto const tests = { "evaluate"sv
                           , "fold"sv
                           , "gc"sv
                           , "satisfy_count"sv
                           , "satisfy_all"sv
                           , "operators"sv
                           , "cofactors"sv
                           , "from_vector"sv
                           , "to_vector"sv
                           , "from_expression"sv };
        auto results = std::unordered_map
            <std::string_view, std::vector<std::optional<test_result>>>();

        // Inserts vector of nullopts for each test name.
        for (auto const test : tests)
        {
            results.emplace( std::piecewise_construct_t()
                           , std::make_tuple(test)
                           , std::make_tuple(testCount) );
        }

        auto output_results = [&results, &tests]()
        {
            for (auto const k : tests)
            {
                auto const& rs = results.at(k);
                std::cout << "  " << k << std::string(16 - k.size(), ' ');
                for (auto const& r : rs)
                {
                    if (r)
                    {
                        std::cout << " " << (*r ? wrap_green(char_ok())
                                                : wrap_red(char_err()));
                    }
                    else
                    {
                        // Not evaluated yet.
                        std::cout << "  ";
                    }
                }
                std::cout << '\n';
            }
        };

        auto outputMutex = std::mutex();
        auto const refresh_results = [&]()
        {
            auto lock = std::scoped_lock<std::mutex>(outputMutex);
            for (auto i = 0u; i < results.size(); ++i)
            {
                // Erase one line from the console.
                std::cout << "\033[A";
            }
            output_results();
            std::cout << std::flush;
        };

        std::cout << wrap_yellow(name) << '\n';
        std::cout << "  node counts default: ";
        for (auto const s : sizesNotSifted)
        {
            std::cout << s << ' ';
        }
        std::cout << "\n";
        std::cout << "  node counts sifted:  ";
        for (auto const s : sizesSifted)
        {
            std::cout << s << ' ';
        }
        std::cout << "\n\n";

        output_results();
        #pragma omp parallel for schedule(dynamic)
        for (auto k = 0u; k < testCount; ++k)
        {
            results.at("evaluate")[k]
                = test_evaluate(managers[k], diagram1s[k], exprs[k]);
            results.at("fold")[k]
                = test_fold(managers[k], diagram1s[k], diagram2s[k]);
            results.at("gc")[k]
                = test_gc(managers[k], diagram1s[k]);
            results.at("satisfy_count")[k]
                = test_satisfy_count(managers[k], diagram1s[k], exprs[k]);
            results.at("satisfy_all")[k]
                = test_satisfy_all(managers[k], diagram1s[k], exprs[k]);
            results.at("operators")[k]
                = test_operators(managers[k], diagram1s[k], exprs[k]);
            results.at("cofactors")[k]
                = test_cofactor(managers[k], diagram1s[k], exprs[k], rngs[k]);
            results.at("from_vector")[k]
                = test_from_vector(managers[k], diagram1s[k], exprs[k]);
            results.at("to_vector")[k]
                = test_to_vector(managers[k], diagram1s[k]);
            results.at("from_expression")[k]
                = test_from_expression(managers[k], rngs[k]);

            refresh_results();
        }

        std::cout << '\n';
    }

    template<class Manager>
    auto test_one
        ( std::string_view name
        , Manager&         manager
        , expr_var const&  expr
        , rng_t&           rng )
    {
        auto diagram1 = create_diagram(expr, manager, fold_e::Left);
        auto diagram2 = create_diagram(expr, manager, fold_e::Tree);

        diagram1 = manager.reduce(diagram1);
        diagram2 = manager.reduce(diagram2);

        std::cout << '\n' << wrap_yellow(name)             << '\n';
        std::cout << "Node count      " << manager.node_count(diagram1)
                                                           << '\n';
        std::cout << "Evaluate        "
            << test_evaluate(manager, diagram1, expr)      << '\n';
        std::cout << "Fold            "
            << test_fold(manager, diagram1, diagram2)      << '\n';
        std::cout << "GC              "
            << test_gc(manager, diagram1)                  << '\n';
        std::cout << "Satisfy-count   "
            << test_satisfy_count(manager, diagram1, expr) << '\n';
        std::cout << "Satisfy-all     "
            << test_satisfy_all(manager, diagram1, expr)   << '\n';
        std::cout << "Operators       "
            << test_operators(manager, diagram1, expr)     << '\n';
        std::cout << "Cofactor        "
            << test_cofactor(manager, diagram1, expr, rng) << '\n';
        std::cout << "From-vector     "
            << test_from_vector(manager, diagram1, expr)   << '\n';
        std::cout << "To-vector       "
            << test_to_vector(manager, diagram1)           << '\n';
        std::cout << "From-expression "
            << test_from_expression(manager, rng)          << '\n';
    }

    template<std::size_t M>
    auto random_domains (std::size_t const n, rng_t& rng)
    {
        auto domainDst = int_dist_t<teddy::uint_t>(2, M);
        return utils::fill_vector(n, [&](auto const)
        {
            return domainDst(rng);
        });
    }

    auto random_order (std::size_t const n, rng_t& rng)
    {
        auto is = utils::fill_vector(n, utils::identity);
        std::shuffle(std::begin(is), std::end(is), rng);
        return is;
    }
}

auto run_test_many ()
{
    namespace us = teddy::utils;
    namespace ts = teddy::test;

    auto constexpr M     = 3;
    auto const varCount  = 15;
    auto const termCount = 20;
    auto const termSize  = 5;
    auto const nodeCount = 1'000;
    auto const testCount = std::thread::hardware_concurrency();
    auto       seedSrc   = std::random_device();
    // auto const seedSrc   = std::integral_constant<long, 1021306696>();
    auto const initSeed  = seedSrc();
    auto constexpr IsFixedSeed = not std::same_as< std::random_device
                                                 , decltype(seedSrc) >;

    auto seeder = ts::rng_t(initSeed);
    auto rngs = us::fill_vector(testCount, [&seeder](auto const)
    {
        return ts::rng_t(seeder());
    });

    auto const exprs = us::fill_vector(testCount, [=, &rngs](auto const k)
    {
        return ts::generate_expression(rngs[k], varCount, termCount, termSize);
    });

    auto orders = us::fmap(rngs, [testCount](auto& rng)
    {
        return ts::random_order(varCount, rng);
    });

    auto domains = us::fmap(rngs, [&](auto& rng)
    {
        return ts::random_domains<M>(varCount, rng);
    });

    auto bddManagers = us::fill_vector(testCount, [&](auto const k)
    {
        return teddy::bdd_manager(varCount, nodeCount, orders[k]);
    });

    auto mddManagers = us::fill_vector(testCount, [&](auto const k)
    {
        return teddy::mdd_manager<M>(varCount, nodeCount, orders[k]);
    });

    auto imddManagers = us::fill_vector(testCount, [&]
        (auto const k) mutable
    {
        return teddy::imdd_manager(varCount, nodeCount, domains[k], orders[k]);
    });

    auto ifmddManagers = us::fill_vector(testCount, [&]
        (auto const k) mutable
    {
        return teddy::ifmdd_manager<M>( varCount, nodeCount
                                      , domains[k], orders[k] );
    });

    auto const seedStr = IsFixedSeed
        ? ts::wrap_red(std::to_string(initSeed))
        : std::to_string(initSeed);
    std::cout << "Seed is " << seedStr << '.' << '\n';
    ts::test_many("BDD manager",   bddManagers,   exprs, rngs);
    ts::test_many("MDD manager",   mddManagers,   exprs, rngs);
    ts::test_many("iMDD manager",  imddManagers,  exprs, rngs);
    ts::test_many("ifMDD manager", ifmddManagers, exprs, rngs);
}

auto run_test_one()
{
    namespace us = teddy::utils;
    namespace ts = teddy::test;

    auto       seedSrc   = std::random_device();
    // auto const seedSrc   = std::integral_constant<long, 31564>();
    auto const initSeed  = seedSrc();
    auto constexpr IsFixedSeed = not std::same_as< std::random_device
                                                 , decltype(seedSrc) >;
    auto seeder     = ts::rng_t(initSeed);
    auto rngDomains = teddy::test::rng_t(seeder());
    auto rngOrder   = teddy::test::rng_t(seeder());
    auto rngExpr    = teddy::test::rng_t(seeder());
    auto rngsTest   = teddy::utils::fill_vector(4, [&](auto const)
    {
        return teddy::test::rng_t(seeder());
    });

    auto constexpr M     = 3;
    auto const varCount  = 15;
    auto const nodeCount = 200;
    auto const domains   = teddy::test::random_domains<M>(varCount, rngDomains);
    auto const order     = teddy::test::random_order(varCount, rngOrder);
    auto const termCount = 20;
    auto const termSize  = 5;

    auto bddM   = teddy::bdd_manager(varCount, nodeCount, nodeCount);
    // bddM.set_auto_reorder(true);
    auto mddM   = teddy::mdd_manager<M>(varCount, nodeCount);
    // mddM.set_auto_reorder(true);
    auto imddM  = teddy::imdd_manager(varCount, nodeCount, domains, order);
    auto ifmddM = teddy::ifmdd_manager<M>(varCount, nodeCount, domains, order);

    auto const expr   = ts::generate_expression( rngExpr, varCount
                                               , termCount, termSize );

    auto const seedStr = IsFixedSeed
        ? ts::wrap_red(std::to_string(initSeed))
        : std::to_string(initSeed);
    std::cout << "Seed is " << seedStr << '.' << '\n';
    teddy::test::test_one("BDD manager",   bddM,   expr, rngsTest[0]);
    teddy::test::test_one("MDD manager",   mddM,   expr, rngsTest[1]);
    teddy::test::test_one("iMDD manager",  imddM,  expr, rngsTest[2]);
    teddy::test::test_one("ifMDD manager", ifmddM, expr, rngsTest[3]);
}

auto run_speed_benchmark()
{
    using namespace std::string_literals;
    auto const plaDir = "/home/michal/Downloads/pla/"s;
    auto const plas   = { "02-adder_col.pla"s, "03-adder_col.pla"s
                        , "04-adder_col.pla"s, "05-adder_col.pla"s
                        , "05-adder_col.pla"s, "06-adder_col.pla"s
                        , "07-adder_col.pla"s, "08-adder_col.pla"s
                        , "09-adder_col.pla"s, "10-adder_col.pla"s
                        , "11-adder_col.pla"s, "12-adder_col.pla"s
                        , "13-adder_col.pla"s, "14-adder_col.pla"s
                        , "15-adder_col.pla"s, "16-adder_col.pla"s };
    for (auto const& pla : plas)
    {
        namespace td = teddy;
        namespace ch = std::chrono;
        namespace rs = std::ranges;
        namespace vs = std::ranges::views;
        auto const path    = plaDir + pla;
        auto const fileOpt = td::pla_file::load_file(path);
        if (fileOpt)
        {
            auto m = td::bdd_manager(fileOpt->variable_count(), 2'000'000);
            auto const timeBefore = ch::high_resolution_clock::now();
            auto const ds         = m.from_pla(*fileOpt, td::fold_type::Tree);
            auto const timeAfter  = ch::high_resolution_clock::now();
            auto const timeMs     = ch::duration_cast<ch::milliseconds>
                                        (timeAfter - timeBefore).count();
            auto const nodeCounts = ds | vs::transform([&m](auto const& d)
            {
                return m.node_count(d);
            });
            auto const nodeCount  = std::reduce( rs::begin(nodeCounts)
                                               , rs::end(nodeCounts) );
            m.force_gc();
            std::cout << pla << " [" << nodeCount << " nodes] ("
                                     << timeMs    <<" ms)" << '\n';
        }
        else
        {
            std::cout << "Failed to load " << path << '\n';
        }
    }
}

auto main () -> int
{
    run_test_many();
    // run_test_one();
    // run_speed_benchmark();

    std::cout << '\n' << "End of main." << '\n';
    return 0;
}