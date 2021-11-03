#define TEDDY_VERBOSE
#undef TEDDY_VERBOSE

#include "teddy/teddy.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <mutex>
#include <omp.h>
#include <random>
#include <ranges>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace teddy::test
{
    struct minmax_expr
    {
        std::vector<std::vector<uint_t>> terms;
    };

    struct constant_expr
    {
        uint_t val;
    };

    using expr_var = std::variant<minmax_expr, constant_expr>;

    /**
     *  Iterates domain of a function. @p domains contains
     *  domains of individual variables.
     */
    class domain_iterator
    {
    public:
        domain_iterator
            (std::vector<uint_t> domains) :
            domains_ (std::move(domains)),
            varVals_ (domains_.size())
        {
        };

        auto has_more () const -> bool
        {
            return not varVals_.empty();
        }

        auto operator* () const -> std::vector<uint_t> const&
        {
            return varVals_;
        }

        auto operator++ () -> void
        {
            auto const varCount = varVals_.size();
            auto overflow       = false;

            for (auto i = 0u; i < varCount; ++i)
            {
                ++varVals_[i];
                overflow = varVals_[i] == domains_[i];
                if (overflow)
                {
                    varVals_[i] = 0;
                }

                if (not overflow)
                {
                    break;
                }
            }

            if (overflow)
            {
                varVals_.clear();
            }
        }

    private:
        std::vector<uint_t> domains_;
        std::vector<uint_t> varVals_;
    };

    template<class F>
    struct dummy_output
    {
        using difference_type   = std::ptrdiff_t;
        using value_type        = dummy_output&;
        using pointer           = value_type;
        using reference         = value_type;
        using iterator_category = std::output_iterator_tag;

        F* f_ {nullptr};
        dummy_output ()               { }
        dummy_output (F& f) : f_ (&f) { }

        auto operator++ () -> dummy_output&
        {
            return *this;
        }

        auto operator++ (int) -> dummy_output&
        {
            return *this;
        }

        auto operator* () -> dummy_output&
        {
            return *this;
        }

        auto operator= (auto&& arg) -> dummy_output&
        {
            (*f_)(std::forward<decltype(arg)>(arg));
            return (*this);
        }

        auto operator= (auto&& arg) const -> dummy_output const&
        {
            (*f_)(std::forward<decltype(arg)>(arg));
            return (*this);
        }
    };

    enum class fold_e
    {
        Left, Tree
    };

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
                ? manager.template left_fold<MIN>(xs)
                : manager.template tree_fold<MIN>(xs);
        };

        auto const max_fold = [&manager, foldType](auto& xs)
        {
            return foldType == fold_e::Left
                ? manager.template left_fold<MAX>(xs)
                : manager.template tree_fold<MAX>(xs);
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

    using rng_t = std::mt19937_64;

    template<class Int>
    using int_dist = std::uniform_int_distribution<Int>;

    auto generate_expression
        ( rng_t&            indexRng
        , std::size_t const varCount
        , std::size_t const termCount
        , std::size_t const termSize )
    {
        assert(varCount > 0);
        static auto indexFrom = index_t {0};
        static auto indexTo   = static_cast<index_t>(varCount - 1u);
        static auto indexDst  = int_dist<index_t>(indexFrom, indexTo);

        auto terms = std::vector<std::vector<uint_t>>(termCount);
        for (auto t = 0u; t < termCount; ++t)
        {
            for (auto k = 0u; k < termSize; ++k)
            {
                terms[t].emplace_back(indexDst(indexRng));
            }
        }

        return expr_var {std::in_place_type_t<minmax_expr>(), std::move(terms)};
    }

    auto evaluate_expression
        ( expr_var const&            expr
        , std::vector<uint_t> const& vs )
    {
        if (std::holds_alternative<constant_expr>(expr))
        {
            return std::get<constant_expr>(expr).val;
        }
        else
        {
            namespace rs = std::ranges;
            auto const term_val = [&vs](auto const& is)
            {
                return vs[rs::min(is, {}, [&vs](auto const i)
                {
                    return vs[i];
                })];
            };
            auto const& ts = std::get<minmax_expr>(expr).terms;
            return rs::max(rs::transform_view(ts, term_val));
        }
    }

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
        return "!";
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
        auto iterator = domain_iterator(manager.get_domains());
        while (iterator.has_more())
        {
            auto const expectedVal = evaluate_expression(expr, *iterator);
            auto const diagramVal  = manager.evaluate(diagram, *iterator);
            if (expectedVal != diagramVal)
            {
                return char_err();
            }
            ++iterator;
        }

        if (not iterator.has_more())
        {
            return char_ok();
        }

        return char_err();
    }

    /**
     *  Tests if different fold creates the same node.
     */
    template<class Dat, class Deg>
    auto test_fold
        ( diagram<Dat, Deg> const& diagram1
        , diagram<Dat, Deg> const& diagram2 )
    {
        if (diagram1.equals(diagram2))
        {
            return char_ok();
        }
        else
        {
            return char_err();
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
        manager.gc();
        auto const totalNodeCount   = manager.node_count();
        auto const diagramNodeCount = manager.node_count(diagram);
        if (totalNodeCount == diagramNodeCount)
        {
            return char_ok();
        }
        else
        {
            return char_err();
        }
    }

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
            auto iterator = domain_iterator(domains);
            while (iterator.has_more())
            {
                inc(counts, evaluate_expression(expr, *iterator));
                ++iterator;
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
                return char_err();
            }
        }

        return char_ok();
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
                auto outF = [&vals, k](auto&&)
                {
                    ++vals[k];
                };
                auto out = dummy_output<decltype(outF)>(outF);
                manager.template satisfy_all_g<out_var_vals>(k, diagram, out);
            }
            return vals;
        }();

        for (auto k = 0u; k < expectedCounts.size(); ++k)
        {
            if (expectedCounts[k] != realCounts[k])
            {
                return char_err();
            }
        }
        return char_ok();
    }

    template<class Dat, class Deg, class Dom>
    auto test_operators
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg>&              diagram
        , expr_var const&                 expr )
    {
        namespace rs = std::ranges;
        auto const max  = [&expr](auto domains)
        {
            if (domains.empty())
            {
                return evaluate_expression(expr, {});
            }

            auto m = uint_t {0};
            auto iterator = domain_iterator(std::move(domains));
            while (iterator.has_more())
            {
                m = std::max(m, evaluate_expression(expr, *iterator));
                ++iterator;
            }
            return m;
        }(manager.get_domains());
        auto const cs   = expected_counts(manager, expr);
        auto const zero = manager.constant(0);
        auto const one  = manager.constant(1);
        auto const sup  = manager.constant(max);
        auto const bd   = manager.booleanize(diagram, utils::not_zero);
        auto const rd   = manager.reduce(diagram);

        if (not manager.template apply<AND>(bd, zero).equals(zero))
        {
            return char_err();
        }

        if (not manager.template apply<AND>(bd, one).equals(bd))
        {
            return char_err();
        }

        if (not manager.template apply<OR>(bd, one).equals(one))
        {
            return char_err();
        }

        if (not manager.template apply<OR>(bd, zero).equals(bd))
        {
            return char_err();
        }

        if (not manager.template apply<XOR>(bd, bd).equals(zero))
        {
            return char_err();
        }

        if (not manager.template apply<MULTIPLIES<2>>(rd, zero).equals(zero))
        {
            return char_err();
        }

        if (not manager.template apply<MULTIPLIES<4>>(rd, one).equals(rd))
        {
            return char_err();
        }

        if (not manager.template apply<PLUS<4>>(rd, zero).equals(rd))
        {
            return char_err();
        }

        if (not manager.template apply<EQUAL_TO>(rd, rd).equals(one))
        {
            return char_err();
        }

        if (not manager.template apply<NOT_EQUAL_TO>(rd, rd).equals(zero))
        {
            return char_err();
        }

        if (not manager.template apply<LESS>(rd, rd).equals(zero))
        {
            return char_err();
        }

        if (not manager.template apply<GREATER>(rd, rd).equals(zero))
        {
            return char_err();
        }

        if (not manager.template apply<LESS_EQUAL>(rd, rd).equals(one))
        {
            return char_err();
        }

        if (not manager.template apply<GREATER_EQUAL>(rd, rd).equals(one))
        {
            return char_err();
        }

        if (not manager.template apply<MIN>(rd, zero).equals(zero))
        {
            return char_err();
        }

        if (not manager.template apply<MIN>(rd, sup).equals(rd))
        {
            return char_err();
        }

        if (not manager.template apply<MAX>(rd, sup).equals(sup))
        {
            return char_err();
        }

        if (not manager.template apply<MAX>(rd, zero).equals(rd))
        {
            return char_err();
        }

        return char_ok();
    }

    /**
     *  Runs all test. Creates diagram represeting @p expr using @p manager .
     */
    template<class Manager>
    auto test_all
        ( std::string_view             name
        , std::vector<Manager>&        managers
        , std::vector<expr_var> const& exprs
        , std::vector<rng_t>&             )
    {
        auto const testCount = managers.size();
        std::cout << wrap_yellow(name) << '\n';

        auto diagram1s = utils::fill_vector(testCount, [&](auto const k)
        {
            return create_diagram(exprs[k], managers[k], fold_e::Left);
        });

        auto diagram2s = utils::fill_vector(testCount, [&](auto const k)
        {
            return create_diagram(exprs[k], managers[k], fold_e::Tree);
        });

        std::cout << "  node counts: ";
        for (auto k = 0u; k < testCount; ++k)
        {
            std::cout << managers[k].node_count(diagram1s[k]) << ' ';
        }
        std::cout << "\n\n";

        using namespace std::string_view_literals;
        auto const tests = { "evaluate"sv
                           , "fold"sv
                           , "gc"sv
                           , "satisfy-count"sv
                           , "satisfy-all"sv
                           , "operators"sv };
        auto results = std::unordered_map< std::string_view
                                         , std::vector<std::string> >();
        for (auto const test : tests)
        {
            results.emplace( std::piecewise_construct_t()
                           , std::make_tuple(test)
                           , std::make_tuple(testCount, " ") );
        }

        auto output_results = [&results, &tests]()
        {
            for (auto const k : tests)
            {
                auto const& rs = results.at(k);
                std::cout << "  " << k << std::string(16 - k.size(), ' ');
                for (auto const& r : rs)
                {
                    std::cout << " " << (r == "✓" ? wrap_green(r) :
                                        (r == "!" ? wrap_red(r)   : " "sv));
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
                std::cout << "\033[A";
            }
            output_results();
            std::cout << std::flush;
        };

        output_results();
        #pragma omp parallel for schedule(dynamic)
        for (auto k = 0u; k < testCount; ++k)
        {
            results.at("evaluate")[k]
                = test_evaluate(managers[k], diagram1s[k], exprs[k]);
            results.at("fold")[k]
                = test_fold(diagram1s[k], diagram2s[k]);
            results.at("gc")[k]
                = test_gc(managers[k], diagram1s[k]);
            results.at("satisfy-count")[k]
                = test_satisfy_count(managers[k], diagram1s[k], exprs[k]);
            results.at("satisfy-all")[k]
                = test_satisfy_all(managers[k], diagram1s[k], exprs[k]);
            results.at("operators")[k]
                = test_operators(managers[k], diagram1s[k], exprs[k]);

            refresh_results();
        }

        std::cout << '\n';
    }

    template<std::size_t M>
    auto random_domains (std::size_t const n, rng_t& rng)
    {
        auto domainDst = int_dist<teddy::uint_t>(2, M);
        return utils::fill_vector(n, [&](auto const)
        {
            return domainDst(rng);
        });
    }
}

auto main () -> int
{
    namespace us = teddy::utils;
    namespace ts = teddy::test;

    auto constexpr M     = 4;
    auto const varCount  = 13;
    auto const termCount = 20;
    auto const termSize  = 5;
    auto const nodeCount = 1000;
    auto const testCount = std::thread::hardware_concurrency() + 2;
    auto       seedSrc   = std::random_device();
    // auto const seedSrc   = std::integral_constant<int, 144>();
    auto const initSeed  = seedSrc();
    auto constexpr IsFixedSeed = not std::same_as< std::random_device
                                                 , decltype(seedSrc) >;

    auto seeder = ts::rng_t(initSeed);
    auto rngs = us::fill_vector(testCount - 2, [&seeder](auto const)
    {
        // One rng to rule them all.
        // Not technically correct but
        // it should be good enough for the purpose of these tests.
        return ts::rng_t(seeder());
    });
    auto const exprs = [=, &rngs]()
    {
        auto res = us::fmap(rngs, [=](auto& indexRng)
        {
            return ts::generate_expression( indexRng, varCount
                                          , termCount, termSize );
        });
        res.emplace_back(std::in_place_type_t<ts::constant_expr>(), 0);
        res.emplace_back(std::in_place_type_t<ts::constant_expr>(), 1);
        return res;
    }();
    auto bddManagers = us::fill_vector(testCount - 2, [=](auto const)
    {
        return teddy::bdd_manager(varCount, nodeCount);
    });
    bddManagers.emplace_back(0, 2);
    bddManagers.emplace_back(0, 2);
    auto mddManagers = us::fill_vector(testCount - 2, [=](auto const)
    {
        return teddy::mdd_manager<M>(varCount, nodeCount);
    });
    mddManagers.emplace_back(0, 2);
    mddManagers.emplace_back(0, 2);
    auto domains = us::fmap(rngs, [&](auto& rng)
    {
        return ts::random_domains<M>(varCount, rng);
    });
    auto imddManagers = us::fill_vector(testCount - 2, [&]
        (auto const k) mutable
    {
        return teddy::imdd_manager(varCount, nodeCount, domains[k]);
    });
    imddManagers.emplace_back(0, 2, std::vector<teddy::uint_t>());
    imddManagers.emplace_back(0, 2, std::vector<teddy::uint_t>());
    auto ifmddManagers = us::fill_vector(testCount - 2, [&]
        (auto const k) mutable
    {
        return teddy::ifmdd_manager<M>(varCount, nodeCount, domains[k]);
    });
    ifmddManagers.emplace_back(0, 2, std::vector<teddy::uint_t>());
    ifmddManagers.emplace_back(0, 2, std::vector<teddy::uint_t>());

    auto const seedStr = IsFixedSeed
        ? ts::wrap_red(std::to_string(initSeed))
        : std::to_string(initSeed);
    std::cout << "Seed is " << seedStr << '.' << '\n';
    ts::test_all("BDD manager",   bddManagers,   exprs, rngs);
    ts::test_all("MDD manager",   mddManagers,   exprs, rngs);
    ts::test_all("iMDD manager",  imddManagers,  exprs, rngs);
    ts::test_all("ifMDD manager", ifmddManagers, exprs, rngs);

    std::cout << '\n' << "End of main." << '\n';

    return 0;
}