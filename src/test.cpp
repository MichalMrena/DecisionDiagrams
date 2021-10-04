#define VERBOSE 1

#include "teddy/teddy.hpp"
#include <random>
#include <cstdio>
#include <iostream>
#include <ranges>
#include <vector>
#include <algorithm>

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

    template<class Int>
    using int_dist = std::uniform_int_distribution<Int>;

    auto generate_expression
        ( std::default_random_engine& seeder
        , std::size_t const           varCount
        , std::size_t const           termCount
        , std::size_t const           termSize )
    {
        assert(varCount > 0);
        static auto indexFrom = index_t {0};
        static auto indexTo   = static_cast<index_t>(varCount - 1u);
        static auto indexRng  = std::default_random_engine(seeder());
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
                outl_red("!!!");
                break;
            }
            ++iterator;
        }

        if (not iterator.has_more())
        {
            outl_green("OK");
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
            outl_green("OK");
        }
        else
        {
            outl_red("!!!");
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
            outl_green("OK");
        }
        else
        {
            out_red("!!!");
            std::cout << " expected " << diagramNodeCount
                      << " got "      << totalNodeCount;
        }
    }

    /**
     *  Runs all test. Creates diagram represeting @p expr using @p manager .
     */
    template<class Dat, class Deg, class Dom>
    auto test_all
        ( std::string_view                name
        , diagram_manager<Dat, Deg, Dom>& manager
        , expression const&               expr
        , std::default_random_engine&      )
    {
        std::cout << CodeYellow << name << CodeReset << '\n';

        auto const diagram1 = create_diagram(expr, manager, fold_e::Left);
        auto       diagram2 = create_diagram(expr, manager, fold_e::Tree);
        std::cout << "  node count: " << manager.node_count(diagram1) << "\n\n";

        std::cout << "  evaluate: ";
        test_evaluate(manager, diagram1, expr);

        std::cout << "  fold:     ";
        test_fold(diagram1, diagram2);

        std::cout << "  gc:       ";
        test_gc(manager, diagram1);

        std::cout << '\n';
    }
}

auto main () -> int
{
    auto const seed      = std::random_device()();
    auto const varCount  = 15;
    auto const termCount = 20;
    auto const termSize  = 5;
    // auto const nodeCount = 10'000;
    auto const nodeCount = 200;

    auto seeder     = std::default_random_engine(seed);
    auto expr       = teddy::test::generate_expression( seeder, varCount
                                                      , termCount, termSize );
    auto bddManager = teddy::bdd_manager(varCount, nodeCount);
    auto mddManager = teddy::mdd_manager<3>(varCount, nodeCount);

    std::cout << "Seed is " << seed << '.' << '\n';
    test_all("BDD manager", bddManager, expr, seeder);
    test_all("MDD manager", mddManager, expr, seeder);

    std::puts("\nEnd of main.");

    return 0;
}