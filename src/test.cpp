#define TEDDY_VERBOSE
#undef TEDDY_VERBOSE

#include "teddy/teddy.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <ranges>
#include <vector>

namespace teddy::test
{
    struct expression
    {
        std::vector<std::vector<uint_t>> terms;
    };

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

    enum class fold_e
    {
        Left, Tree
    };

    template<class Dat, degree Deg, domain Dom>
    auto create_diagram
        ( expression const&               expr
        , diagram_manager<Dat, Deg, Dom>& manager
        , fold_e const                    foldType )
    {
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
        for (auto const& eTerm : expr.terms)
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

        return expression {std::move(terms)};
    }

    auto evaluate_expression
        ( expression const&          expr
        , std::vector<uint_t> const& vs )
    {
        namespace rs = std::ranges;
        auto const term_val = [&vs](auto const& is)
        {
            return vs[rs::min(is, {}, [&vs](auto const i)
            {
                return vs[i];
            })];
        };
        return rs::max(rs::transform_view(expr.terms, term_val));
    }

    auto constexpr CodeRed    = "\x1B[91m";
    auto constexpr CodeGreen  = "\x1B[92m";
    auto constexpr CodeYellow = "\x1B[93m";
    auto constexpr CodeReset  = "\x1B[0m";

    auto out_green (std::string_view const s)
    {
        std::cout << CodeGreen << s << CodeReset;
    }

    auto out_red (std::string_view const s)
    {
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
        , expression const&               expr )
    {
        auto iterator = domain_iterator(manager.get_domains());
        while (iterator.has_more())
        {
            auto const expectedVal = evaluate_expression(expr, *iterator);
            auto const diagramVal  = manager.evaluate(diagram, *iterator);
            if (expectedVal != diagramVal)
            {
                out_red("!!");
                break;
            }
            ++iterator;
        }

        if (not iterator.has_more())
        {
            out_green("OK");
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
            out_green("OK");
        }
        else
        {
            out_red("!!");
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
            out_green("OK");
        }
        else
        {
            out_red("!!");
            // std::cout << " expected " << diagramNodeCount
            //           << " got "      << totalNodeCount;
        }
    }

    /**
     *  Runs all test. Creates diagram represeting @p expr using @p manager .
     */
    template<class Manager>
    auto test_all
        ( std::string_view               name
        , std::vector<Manager>&          managers
        , std::vector<expression> const& exprs
        , std::vector<rng_t>&             )
    {
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

        out("  evaluate: ");
        for (auto k = 0u; k < testCount; ++k)
        {
            test_evaluate(managers[k], diagram1s[k], exprs[k]);
            flushed_space();
        }
        endl();

        out("  fold:     ");
        for (auto k = 0u; k < testCount; ++k)
        {
            test_fold(diagram1s[k], diagram2s[k]);
            flushed_space();
        }
        endl();

        out("  gc:       ");
        for (auto k = 0u; k < testCount; ++k)
        {
            test_gc(managers[k], diagram1s[k]);
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

    auto const varCount  = 15;
    auto const termCount = 20;
    auto const termSize  = 5;
    auto const nodeCount = 1000;
    auto const testCount = 5;
    // auto const initSeed  = std::random_device()();
    auto const initSeed  = 144;

    auto initSeeder = ts::rng_t(initSeed);
    auto seeders    = us::fill_vector(testCount, [&initSeeder](auto const)
    {
        return ts::rng_t(initSeeder());
    });
    auto indexRngs = us::fmap(seeders, [](auto& seeder)
    {
        return ts::rng_t(seeder());
    });
    auto domainRngs = us::fmap(seeders, [](auto& seeder)
    {
        return ts::rng_t(seeder());
    });

    auto const exprs = us::fmap(indexRngs, [=](auto& indexRng)
    {
        return ts::generate_expression(indexRng, varCount, termCount, termSize);
    });
    auto bddManagers = us::fill_vector(testCount, [=](auto const)
    {
        return teddy::bdd_manager(varCount, nodeCount);
    });
    auto mddManagers = us::fill_vector(testCount, [=](auto const)
    {
        return teddy::mdd_manager<3>(varCount, nodeCount);
    });
    auto imddManagers = us::fill_vector(testCount, [=, &seeders]
        (auto const k) mutable
    {
        auto domains = ts::random_domains(varCount, domainRngs[k]);
        return teddy::imdd_manager(varCount, nodeCount, std::move(domains));
    });
    auto ifmddManagers = us::fill_vector(testCount, [=, &seeders]
        (auto const k) mutable
    {
        auto domains = ts::random_domains(varCount, domainRngs[k]);
        return teddy::ifmdd_manager<3>(varCount, nodeCount, std::move(domains));
    });

    if (imddManagers.front().get_domains() == ifmddManagers.front().get_domains())
    {
        std::cout << "rovanke\n";
    }

    if (ts::random_domains(10, domainRngs[0]) == ts::random_domains(10, domainRngs[0]))
    {
        std::cout << "stale rovnake\n";
    }

    std::cout << "Seed is " << initSeed << '.' << '\n';
    // test_all("BDD manager",   bddManagers,  exprs, seeders);
    // test_all("MDD manager",   mddManagers,  exprs, seeders);
    test_all("iMDD manager",  imddManagers,  exprs, seeders);
    test_all("ifMDD manager", ifmddManagers, exprs, seeders);

    std::cout << '\n' << "End of main." << '\n';

    return 0;
}