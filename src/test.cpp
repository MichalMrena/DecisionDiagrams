#define TEDDY_VERBOSE
#undef TEDDY_VERBOSE

#include "teddy/teddy.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <ranges>
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

    auto constexpr CodeReset  = "\x1B[0m";

    auto out_green (std::string_view const s)
    {
        auto constexpr CodeGreen = "\x1B[92m";
        std::cout << CodeGreen << s << CodeReset;
    }

    auto out_red (std::string_view const s)
    {
        auto constexpr CodeRed = "\x1B[91m";
        std::cout << CodeRed << s << CodeReset;
    }

    auto outl_green (std::string_view const s)
    {
        out_green(s);
        std::cout << '\n';
    }

    auto outl_red (std::string_view const s)
    {
        out_red(s);
        std::cout << '\n';
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
                out_red("!");
                break;
            }
            ++iterator;
        }

        if (not iterator.has_more())
        {
            out_green("✓");
        }
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
            out_green("✓");
        }
        else
        {
            out_red("!");
        }
    }

    /**
     *  Tests if garbage collection collects all nodes except nodes
     *  that are part of @p diagram .
7     */
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
            out_green("✓");
        }
        else
        {
            out_red("!");
        }
    }

    /**
     *  Tests the satisfy_count algorithm.
     */
    template<std::size_t Max, class Dat, class Deg, class Dom>
    auto test_satisfy_count
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg>&              diagram
        , expr_var const&                 expr )
    {
        auto const expectedCounts = [&manager, &expr]()
        {
            auto cs = std::array<std::size_t, Max>{};
            auto domains = manager.get_domains();
            if (domains.empty())
            {
                ++cs[evaluate_expression(expr, {})];
            }
            else
            {
                auto iterator = domain_iterator(std::move(domains));
                while (iterator.has_more())
                {
                    ++cs[evaluate_expression(expr, *iterator)];
                    ++iterator;
                }
            }
            return cs;
        }();

        auto const realCounts = [&manager, &expr, &diagram]() mutable
        {
            auto cs = std::array<std::size_t, Max>{};
            for (auto v = 0u; v < Max; ++v)
            {
                cs[v] = manager.satisfy_count(v, diagram);
            }
            return cs;
        }();

        for (auto k = 0u; k < Max; ++k)
        {
            if (realCounts[k] != expectedCounts[k])
            {
                out_red("!");
                return;
            }
        }

        out_green("✓");
    }

    /**
     *  Test the satisfy_all algorithm;
     */
    template<std::size_t Max, class Dat, class Deg, class Dom>
    auto test_satisfy_all
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg>&              diagram
        , expr_var const&                 expr )
    {
        auto const domains = manager.get_domains();
        auto const expectedCounts = [&manager, &expr, &domains]()
        {
            auto vals = std::array<std::size_t, Max> {};
            auto it = domain_iterator(manager.get_domains());
            if (domains.empty())
            {
                ++vals[evaluate_expression(expr, {})];
            }
            else
            {
                auto iterator = domain_iterator(std::move(domains));
                while (iterator.has_more())
                {
                    ++vals[evaluate_expression(expr, *iterator)];
                    ++iterator;
                }
            }
            return vals;
        }();

        auto const realCounts = [&manager, &diagram, &domains]()
        {
            namespace rs = std::ranges;
            auto const max = domains.empty()
                ? 2
                : rs::max(manager.get_domains());
            auto vals = std::array<std::size_t, Max> {};
            for (auto k = 0u; k < max; ++k)
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

        for (auto k = 0u; k < Max; ++k)
        {
            if (expectedCounts[k] != realCounts[k])
            {
                out_red("!");
                return;
            }
        }
        out_green("✓");
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
        auto constexpr CodeYellow = "\x1B[93m";
        auto const flushed_space = []()
        {
            std::cout << ' ' << std::flush;
        };
        auto const endl = []()
        {
            std::cout << '\n';
        };
        auto const out = [](auto const& s)
        {
            std::cout << s;
        };

        auto const testCount = managers.size();
        std::cout << CodeYellow << name << CodeReset << '\n';

        auto diagram1s = utils::fill_vector(testCount, [&](auto const k)
        {
            return create_diagram(exprs[k], managers[k], fold_e::Left);
        });

        auto diagram2s = utils::fill_vector(testCount, [&](auto const k)
        {
            return create_diagram(exprs[k], managers[k], fold_e::Tree);
        });

        out("  node counts: ");
        for (auto k = 0u; k < testCount; ++k)
        {
            std::cout << managers[k].node_count(diagram1s[k]) << ' ';
        }
        endl();
        endl();

        out("  evaluate:      ");
        for (auto k = 0u; k < testCount; ++k)
        {
            test_evaluate(managers[k], diagram1s[k], exprs[k]);
            flushed_space();
        }
        endl();

        out("  fold:          ");
        for (auto k = 0u; k < testCount; ++k)
        {
            test_fold(diagram1s[k], diagram2s[k]);
            flushed_space();
        }
        endl();

        out("  gc:            ");
        for (auto k = 0u; k < testCount; ++k)
        {
            test_gc(managers[k], diagram1s[k]);
            flushed_space();
        }
        endl();

        out("  satisfy-count: ");
        for (auto k = 0u; k < testCount; ++k)
        {
            test_satisfy_count<3>(managers[k], diagram1s[k], exprs[k]);
            flushed_space();
        }
        endl();

        out("  satisfy-all:   ");
        for (auto k = 0u; k < testCount; ++k)
        {
            test_satisfy_all<3>(managers[k], diagram1s[k], exprs[k]);
            flushed_space();
        }
        endl();

        endl();
    }

    auto random_domains (std::size_t const n, rng_t& rng)
    {
        auto domainDst = int_dist<teddy::uint_t>(2, 3);
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

    auto const varCount  = 17;
    auto const termCount = 20;
    auto const termSize  = 5;
    auto const nodeCount = 1000;
    auto const testCount = 10;
    auto const initSeed  = std::random_device()();
    // auto const initSeed  = 144;

    // One rng for one test should be enough for the purpose of this tests.
    auto seeder = ts::rng_t(initSeed);
    auto rngs = us::fill_vector(testCount - 2, [&seeder](auto const)
    {
        return ts::rng_t(seeder());
    });
    auto const exprs = [=, &rngs]()
    {
        auto res = us::fmap(rngs, [=, &rngs](auto& indexRng)
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
        return teddy::mdd_manager<3>(varCount, nodeCount);
    });
    mddManagers.emplace_back(0, 2);
    mddManagers.emplace_back(0, 2);
    auto domains = us::fmap(rngs, [&](auto& rng)
    {
        return ts::random_domains(varCount, rng);
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
        return teddy::ifmdd_manager<3>(varCount, nodeCount, domains[k]);
    });
    ifmddManagers.emplace_back(0, 2, std::vector<teddy::uint_t>());
    ifmddManagers.emplace_back(0, 2, std::vector<teddy::uint_t>());

    std::cout << "Seed is " << initSeed << '.' << '\n';
    ts::test_all("BDD manager",   bddManagers,   exprs, rngs);
    ts::test_all("MDD manager",   mddManagers,   exprs, rngs);
    ts::test_all("iMDD manager",  imddManagers,  exprs, rngs);
    ts::test_all("ifMDD manager", ifmddManagers, exprs, rngs);

    // auto m    = teddy::imdd_manager(0, 10, {});
    // auto zero = m.constant(0);
    // auto one  = m.constant(1);
    // std::cout << m.satisfy_count(0, zero) << '\n';
    // std::cout << m.satisfy_count(1, zero) << '\n';
    // std::cout << m.satisfy_count(0, one)  << '\n';
    // std::cout << m.satisfy_count(1, one)  << '\n';

    std::cout << '\n' << "End of main." << '\n';

    return 0;
}