#ifndef MIX_DD_TEST_TEST_MDD_HPP
#define MIX_DD_TEST_TEST_MDD_HPP

#include "../lib/mdd_manager.hpp"
#include "../lib/utils/more_random.hpp"
#include "../lib/utils/string_utils.hpp"

#include <vector>

namespace mix::dd::test
{
    template<class T>
    using int_rng   = utils::random_uniform_int<T>;
    using seed_t    = unsigned int;
    using var_v     = std::vector<index_t>;
    using bool_v    = std::vector<bool>;
    using var_vv    = std::vector<var_v>;
    using uint_v    = std::vector<unsigned>;
    template<std::size_t P>
    using manager_t = mdd_manager<double, void, P>;
    template<std::size_t P>
    using mdd_t     = mdd<double, void, P>;

    inline auto constexpr MddVariableCount  = 15;
    inline auto constexpr MddProductCount   = 25;
    inline auto constexpr MddMaxProductSize = 4;

    struct mvl_function
    {
        std::size_t varCount;
        uint_v      domains;
        var_vv      products;
    };

    enum class domain_e
    {
        Homogenous,
        Nonhomogenous
    };

    template<std::size_t P>
    class domain_iterator
    {
    public:
        using log_t = typename log_val_traits<P>::type;
        using log_v = std::vector<log_t>;

    public:
        domain_iterator (std::size_t const varCount, uint_v domains) :
            domains_ (std::move(domains)),
            varVals_ (varCount)
        {
        };

        auto var_vals () const -> log_v const&
        {
            return varVals_;
        }

        auto has_more () const -> bool
        {
            return not varVals_.empty();
        }

        auto move_next () -> void
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
        uint_v domains_;
        log_v  varVals_;
    };

    template<class F>
    struct dummy_output
    {
        F f_;
        dummy_output    (F f) : f_ (f) { }
        auto operator++ (int)          { return *this; }
        auto operator*  ()             { return *this; }
        auto operator=  (auto&& arg)   { f_(std::forward<decltype(arg)>(arg)); }
    };

    template<std::size_t P>
    auto get_domains ( domain_e const     d
                     , std::size_t const  varCount
                     , int_rng<unsigned>& rngDomain )
    {
        auto const get_homogenous_domains = [=]()
        {
            auto ds = std::vector<unsigned int>(varCount);
            std::fill(std::begin(ds), std::end(ds), P);
            return ds;
        };

        auto const get_nonhomogenous_domains = [&]()
        {
            return utils::fill_vector(varCount, [&](auto const)
            {
                return rngDomain.next_int();
            });
        };

        switch (d)
        {
            case domain_e::Homogenous:    return get_homogenous_domains();
            case domain_e::Nonhomogenous: return get_nonhomogenous_domains();
            default: throw "not good";
        }
    }

    inline auto generate_function ( std::size_t const     varCount
                                  , std::size_t const     productCount
                                  , uint_v                domains
                                  , int_rng<std::size_t>& rngProductSize
                                  , int_rng<index_t>&     rngVarIndex )
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

        return mvl_function {varCount, std::move(domains), std::move(products)};
    }

    template<std::size_t P, class MulFold, class PlusFold>
    auto make_diagram ( manager_t<P>&       m
                      , mvl_function const& function
                      , MulFold             mulFold
                      , PlusFold            plusFold )
    {
        using mdd = typename manager_t<P>::mdd_t;

        auto productDiagrams = std::vector<mdd>();
        for (auto const& product : function.products)
        {
            auto varDiagrams = std::vector<mdd>(m.variables(product));
            productDiagrams.push_back(mulFold(varDiagrams));
        }

        return plusFold(productDiagrams);
    }

    template<std::size_t P>
    auto eval_function ( mvl_function const& function
                       , uint_v const&       varVals )
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
    auto test_collect_garbage ( manager_t<P>&   m
                              , mdd_t<P> const& d )
    {
        m.collect_garbage();
        auto const rootIndex     = m.dependency_set(d).front();
        auto const diagramVCount = m.vertex_count(d);
        auto const totalVCount   = m.vertex_count();
        auto const rootIVCount   = m.vertex_count(rootIndex);

        if (diagramVCount != totalVCount)
        {
            return utils::concat( "Failed. ", "Vertex count = ", totalVCount
                                , ", expected ", diagramVCount, "." );
        }

        if (1 != rootIVCount)
        {
            return utils::concat( "Failed. ", "Root vertex count = ", rootIVCount
                                , ", expected 1.");
        }

        return std::string("OK");
    }

    template<std::size_t P>
    auto test_evaluate ( manager_t<P>&       m
                       , mvl_function const& f
                       , mdd_t<P> const&     d )
    {
        auto enumerator = domain_iterator<P>(MddVariableCount, f.domains);

        while (enumerator.has_more())
        {
            auto const realVal    = eval_function<P>(f, enumerator.var_vals());
            auto const diagramVal = m.evaluate(d, enumerator.var_vals());
            if (realVal != diagramVal)
            {
                return utils::concat( "Failed. ", "Got ", diagramVal, ", expected ", realVal, "" );
            }
            enumerator.move_next();
        }

        return std::string("OK");
    }

    template<std::size_t P>
    auto test_satisfy_count ( manager_t<P>&       m
                            , mvl_function const& f
                            , mdd_t<P>&           d )
    {
        auto const expectedScs = [&]()
        {
            auto vs  = domain_iterator<P>(MddVariableCount, f.domains);
            auto scs = std::array<unsigned, P> {};

            while (vs.has_more())
            {
                ++scs[eval_function<P>(f, vs.var_vals())];
                vs.move_next();
            }

            return scs;
        }();

        auto const realScs = utils::fill_array<P>([&](auto const l)
        {
            return m.satisfy_count(l, d);
        });

        auto const areEqual = std::equal( std::begin(expectedScs), std::end(expectedScs)
                                        , std::begin(realScs) );

        if (!areEqual)
        {
            return utils::concat( "Failed. Expected {", utils::concat_range(expectedScs, " "), "}"
                                , " got {", utils::concat_range(realScs, " "), "}" );
        }

        return std::string("OK");
    }

    template<std::size_t P>
    auto test_satisfy_all ( manager_t<P>&       m
                          , mvl_function const& f
                          , mdd_t<P>&           d )
    {
        auto const expectedScs = [&]()
        {
            auto vs  = domain_iterator<P>(MddVariableCount, f.domains);
            auto scs = std::array<unsigned, P> {};

            while (vs.has_more())
            {
                auto const val = eval_function<P>(f, vs.var_vals());
                if (!is_nodomain<P>(val))
                {
                    ++scs[val];
                }
                vs.move_next();
            }

            return scs;
        }();

        auto const realScs = [&]()
        {
            using var_vals_t = std::array<unsigned, MddVariableCount>;
            auto sas = std::array<std::size_t, P> {};
            auto out = dummy_output([&](auto&& vals)
            {
                ++sas[m.evaluate(d, vals)];
            });
            for (auto i = 0u; i < P; ++i)
            {
                m.template satisfy_all<var_vals_t>(i, d, out);
            }
            return sas;
        }();

        auto const areEqual = std::equal( std::begin(expectedScs), std::end(expectedScs)
                                        , std::begin(realScs) );

        if (!areEqual)
        {
            return utils::concat( "Failed. Expected {", utils::concat_range(expectedScs, " "), "}"
                                , " got {", utils::concat_range(realScs, " "), "}" );
        }

        return std::string("OK");
    }

    template<std::size_t P>
    auto test_restrict_var ( manager_t<P>&       m
                           , mvl_function const& f
                           , mdd_t<P> const&     d
                           , int_rng<index_t>&   rngVarIndex )
    {
        auto const i1 = rngVarIndex.next_int();
        auto const i2 = [&]()
        {
            for (;;)
            {
                // Potentially dangerous but should be ok...
                auto const i = rngVarIndex.next_int();
                if (i != i1)
                {
                    return i;
                }
            }
        }();
        auto const v1 = 0;
        auto const v2 = 1;
        auto const d_ = m.restrict_var(m.restrict_var(d, i1, v1), i2, v2);

        auto enumerator = domain_iterator<P>(MddVariableCount, f.domains);

        while (enumerator.has_more())
        {
            auto const varVals    = [&]()
            {
                auto vs = enumerator.var_vals();
                vs[i1] = v1;
                vs[i2] = v2;
                return vs;
            }();
            auto const diagramVal = m.evaluate(d_, varVals);
            auto const realVal    = eval_function<P>(f, varVals);
            if (realVal != diagramVal)
            {
                return utils::concat( "Failed. ", "x", i1, "=", v1, ", x", i2, "=", v2, ". "
                                    , "Got ", diagramVal, ", expected ", realVal, "." );
            }
            enumerator.move_next();
        }

        return std::string("OK");
    }

    template<std::size_t P>
    auto test_mdd ( std::size_t const n
                  , order_e const     order  = order_e::Default
                  , domain_e const    domain = domain_e::Homogenous
                  , seed_t const      seed   = 0u )
    {
        auto initSeed        = 0ul == seed ? std::random_device () () : seed;
        auto seeder          = int_rng<seed_t>(0u, UIntMax, initSeed);
        auto rngProductSize  = int_rng<std::size_t>(1, MddMaxProductSize, seeder.next_int());
        auto rngVarIndex     = int_rng<index_t>(0, MddVariableCount - 1, seeder.next_int());
        auto rngRestVarIndex = int_rng<index_t>(0, MddVariableCount - 1, seeder.next_int());
        auto rngOrderShuffle = std::mt19937(seeder.next_int());
        auto rngDomain       = int_rng<unsigned>(2, P, seeder.next_int());

        std::cout << "Running " << n << " tests." << '\n';
        std::cout << "Seed is " << initSeed       << '\n';

        for (auto i = 0u; i < n; ++i)
        {
            auto const varorder = get_order(order, rngOrderShuffle, MddVariableCount);
            auto const domains  = get_domains<P>(domain, MddVariableCount, rngDomain);
            auto manager        = manager_t<P>(MddVariableCount);
            manager.set_order(varorder);
            manager.set_domains(domains);

            auto const mulLeftFold  = [&manager](auto&& ds){ return manager.template left_fold<MULTIPLIES>(ds); };
            auto const plusLeftFold = [&manager](auto&& ds){ return manager.template left_fold<PLUS>(ds);       };
            auto const mulTreeFold  = [&manager](auto&& ds){ return manager.template tree_fold<MULTIPLIES>(ds); };
            auto const plusTreeFold = [&manager](auto&& ds){ return manager.template tree_fold<PLUS>(ds);       };

            auto const function     = generate_function(MddVariableCount, MddProductCount, domains, rngProductSize, rngVarIndex);
            auto       diagram      = make_diagram<P>(manager, function, mulLeftFold, plusLeftFold);
            auto       diagram2     = make_diagram<P>(manager, function, mulTreeFold, plusTreeFold);

            std::cout << '#'                     << i                                                                    << '\n';
            std::cout << "    Diagram"                                                                                   << '\n';
            std::cout << "        Vertex count    " << manager.vertex_count(diagram)                                     << '\n';
            std::cout << "        Order           " << utils::concat_range(varorder, " > ")                              << '\n';
            std::cout << "        Domains         " << utils::concat_range(function.domains, " > ")                      << '\n';
            std::cout << "    Tests"                                                                                     << '\n';
            std::cout << "        Fold            " << (diagram.equals(diagram2) ? "OK" : "Failed.")                     << '\n';
            std::cout << "        Collect garbage " << test_collect_garbage<P>(manager, diagram)                         << '\n';
            std::cout << "        Evaluate        " << test_evaluate<P>(manager, function, diagram)                      << '\n';
            std::cout << "        Satisfy count   " << test_satisfy_count<P>(manager, function, diagram)                 << '\n';
            std::cout << "        Satisfy all     " << test_satisfy_all<P>(manager, function, diagram)                   << '\n';
            std::cout << "        Restrict var    " << test_restrict_var<P>(manager, function, diagram, rngRestVarIndex) << '\n';
            std::cout << "\n";
            // TODO test operators absorbing, neutral element, pri Booleovskych najprv transform na 0 1 cez EQ, LT, GT, ...
        }
    }
}

#endif