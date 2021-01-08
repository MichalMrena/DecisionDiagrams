#ifndef MIX_DD_TEST_TEST_MDD_HPP
#define MIX_DD_TEST_TEST_MDD_HPP

#include "../lib/mdd_manager.hpp"
#include "../lib/utils/more_random.hpp"

#include <vector>

namespace mix::dd::test
{
    template<class T>
    using int_rng     = utils::random_uniform_int<T>;
    using seed_t      = unsigned int;
    using var_v       = std::vector<index_t>;
    using var_vv      = std::vector<var_v>;

    inline auto constexpr MddVariableCount  = 6;
    inline auto constexpr MddProductCount   = 3;
    inline auto constexpr MddMaxProductSize = 3;

    struct mvl_function
    {
        var_vv products;
    };

    template<std::size_t P>
    class domain_iterator
    {
    public:
        using log_t = typename log_val_traits<P>::type;
        using log_v = std::vector<log_t>;

    public:
        domain_iterator (std::size_t const varCount);

        auto var_vals  () const -> log_v const&;
        auto has_more  () const -> bool;
        auto move_next ()       -> void;

    private:
        log_v varVals_;
    };

    inline auto generate_function ( std::size_t const     productCount
                                  , int_rng<std::size_t>& rngProductSize
                                  , int_rng<index_t>&     rngVarIndex ) -> mvl_function
    {
        auto products = var_vv(productCount);

        for (auto pi = 0u; pi < productCount; ++pi)
        {
            auto const productSize = rngProductSize.next_int();
            for (auto vi = 0u; vi <= productSize; ++vi)
            {
                products[pi].push_back(rngVarIndex.next_int());
            }
        }

        return mvl_function {std::move(products)};
    }

    template<std::size_t P, class Folder>
    inline auto make_diagram ( mdd_manager<void, void, P>& m
                             , mvl_function const&         function
                             , Folder                      fold )
    {
        using mdd = typename mdd_manager<void, void, P>::mdd_t;

        auto productDiagrams = std::vector<mdd>();
        for (auto const& product : function.products)
        {
            auto const varDiagrams = std::vector<mdd>(m.just_vars(product));
            productDiagrams.push_back(fold(varDiagrams, MULTIPLIES_MOD<P>()));
        }

        return fold(productDiagrams, PLUS_MOD<P>());
    }

    template<std::size_t P>
    auto eval_function ( mvl_function const& function
                       , auto const&         varVals )
    {
        auto const& pss = function.products;
        auto result = 0u;
        for (auto const& ps : pss)
        {
            auto product = 1u;
            for (auto const& i : ps)
            {
                product *= varVals[i];
            }
            result += product % P;
        }
        return static_cast<unsigned int>(result % P);
    }

    template<std::size_t P>
    auto test_mdd ( std::size_t const n
                  , order_e const     order = order_e::Default
                  , seed_t const      seed  = 0u )
    {
        auto initSeed          = 0ul == seed ? std::random_device () () : seed;
        auto seeder            = int_rng<seed_t>(0u, UIntMax, initSeed);
        auto rngProductSize    = int_rng<std::size_t>(1, MddMaxProductSize, seeder.next_int());
        auto rngVarIndex       = int_rng<index_t>(0, MddVariableCount - 1, seeder.next_int());
        auto rngOrderShuffle   = std::mt19937(seeder.next_int());

        for (auto i = 0u; i < n; ++i)
        {
            auto manager        = mdd_manager<void, void, P>(MddVariableCount);
            auto const os       = get_order(order, rngOrderShuffle, MddVariableCount);
            manager.set_order(os);
            auto const function = generate_function(MddProductCount, rngProductSize, rngVarIndex);
            auto const diagram  = make_diagram<P>(manager, function, [&manager](auto&& ds, auto&& f){ return manager.tree_fold(ds, f); });
            auto const diagram2 = make_diagram<P>(manager, function, [&manager](auto&& ds, auto&& f){ return manager.left_fold(ds, f); });
            manager.collect_garbage();
            auto const vertexCount = manager.vertex_count(diagram);

            assert(diagram == diagram2);
            assert(1 == manager.vertex_count(diagram.get_root()->get_index()));
            assert(vertexCount == manager.vertex_count());

            std::cout << '#' << i                            << '\n';
            std::cout << "    Vertex count: " << vertexCount << '\n';
            std::cout << "    Order:        " << utils::concat_range(os, " > ") << '\n';

            auto result     = true;
            auto enumerator = domain_iterator<P>(MddVariableCount);
            while (enumerator.has_more())
            {
                auto const realVal    = eval_function<P>(function, enumerator.var_vals());
                auto const diagramVal = manager.evaluate(diagram, enumerator.var_vals());
                if (realVal != diagramVal)
                {
                    std::cout << "    !!! Error output missmatch. " << '\n';
                    result = false;
                    break;
                }

                enumerator.move_next();
            }

            std::cout << (result ? "    Result:       OK"
                                 : "    Result:       Failed.") << "\n\n";
        }
    }

    template<std::size_t P>
    domain_iterator<P>::domain_iterator
        (std::size_t const varCount) :
        varVals_ (varCount)
    {
    }

    template<std::size_t P>
    auto domain_iterator<P>::var_vals
        () const -> log_v const&
    {
        return varVals_;
    }

    template<std::size_t P>
    auto domain_iterator<P>::has_more
        () const -> bool
    {
        return not varVals_.empty();
    }

    template<std::size_t P>
    auto domain_iterator<P>::move_next
        () -> void
    {
        auto const varCount = varVals_.size();
        auto overflow       = false;

        for (auto i = 0u; i < varCount; ++i)
        {
            ++varVals_[i];
            overflow = varVals_[i] == P;
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
}

#endif