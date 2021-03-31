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
    using manager_t = mdd_manager<void, void, P>;
    template<std::size_t P>
    using mdd_t     = mdd<void, void, P>;

    inline auto constexpr MddVariableCount  = 15;
    inline auto constexpr MddProductCount   = 25;
    inline auto constexpr MddMaxProductSize = 4;

    struct mvl_function
    {
        std::size_t varCount;
        uint_v      domains;
        var_vv      products;
    };

    enum class order_e
    {
        Default,
        Random
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
        domain_iterator (uint_v domains) :
            domains_ (std::move(domains)),
            varVals_ (domains_.size())
        {
        };

        auto has_more () const -> bool
        {
            return not varVals_.empty();
        }

        auto operator* () const -> log_v const&
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

    auto constexpr UIntMax = std::numeric_limits<unsigned int>::max();

    inline auto get_order ( order_e const     o
                          , std::mt19937&     rngOrder
                          , std::size_t const varCount )
    {
        auto const get_default_order = [=]()
        {
            auto is = std::vector<index_t>(varCount);
            std::iota(std::begin(is), std::end(is), 0);
            return is;
        };

        auto const get_random_order = [&]()
        {
            auto is = get_default_order();
            std::shuffle(std::begin(is), std::end(is), rngOrder);
            return is;
        };

        switch (o)
        {
            case order_e::Default: return get_default_order();
            case order_e::Random:  return get_random_order();
            default: throw "not good";
        }
    }

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
            result += product;
        }
        return static_cast<unsigned int>(result % P);
    }

    template<std::size_t P>
    auto test_collect_garbage ( manager_t<P>&   m
                              , mdd_t<P> const& d )
    {
        m.collect_garbage();
        auto const depSet        = m.dependency_set(d);
        auto const rootIndex     = depSet.front();
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
        auto enumerator = domain_iterator<P>(f.domains);

        while (enumerator.has_more())
        {
            auto const realVal    = eval_function<P>(f, *enumerator);
            auto const diagramVal = m.evaluate(d, *enumerator);
            if (realVal != diagramVal)
            {
                return utils::concat( "Failed. ", "Got ", diagramVal, ", expected ", realVal, "" );
            }
            ++enumerator;
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
            auto vs  = domain_iterator<P>(f.domains);
            auto scs = std::array<unsigned, P> {};

            while (vs.has_more())
            {
                ++scs[eval_function<P>(f, *vs)];
                ++vs;
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
            auto vs  = domain_iterator<P>(f.domains);
            auto scs = std::array<unsigned, P> {};

            while (vs.has_more())
            {
                auto const val = eval_function<P>(f, *vs);
                if (!is_nodomain<P>(val))
                {
                    ++scs[val];
                }
                ++vs;
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
                m.template satisfy_all_g<var_vals_t>(i, d, out);
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
        auto const d_ = m.cofactor(m.cofactor(d, i1, v1), i2, v2);

        return test_evaluate(m, f, d);
    }

    template<std::size_t P>
    auto test_operators ( manager_t<P>&   m
                        , mdd_t<P> const& d )
    {
        auto const zero = m.constant(0);
        auto const one  = m.constant(1);
        auto const sup  = m.constant(P - 1);
        auto const bd   = m.booleanize(d);
        auto const rd   = m.reduce(d);

        if (not m.template apply<AND>(bd, zero).equals(zero))
        {
            return std::string("AND Absorbing element failed.");
        }

        if (not m.template apply<AND>(bd, one).equals(bd))
        {
            return std::string("AND Neutral element failed.");
        }

        if (not m.template apply<OR>(bd, one).equals(one))
        {
            return std::string("OR Absorbing element failed.");
        }

        if (not m.template apply<OR>(bd, zero).equals(bd))
        {
            return std::string("OR Neutral element failed.");
        }

        if (not m.template apply<XOR>(bd, bd).equals(zero))
        {
            return std::string("XOR Annihilate failed.");
        }

        if (not m.template apply<MULTIPLIES>(rd, zero).equals(zero))
        {
            return std::string("(*) Absorbing element failed.");
        }

        if (not m.template apply<MULTIPLIES>(rd, one).equals(rd))
        {
            return std::string("(*) Neutral element failed.");
        }

        if (not m.template apply<PLUS>(rd, zero).equals(rd))
        {
            return std::string("(+) Neutral element failed.");
        }

        if (not m.template apply<EQUAL_TO>(rd, rd).equals(one))
        {
            return std::string("(==) Annihilate failed.");
        }

        if (not m.template apply<NOT_EQUAL_TO>(rd, rd).equals(zero))
        {
            return std::string("(!=) Annihilate failed.");
        }

        if (not m.template apply<LESS>(rd, rd).equals(zero))
        {
            return std::string("(<) Annihilate failed.");
        }

        if (not m.template apply<GREATER>(rd, rd).equals(zero))
        {
            return std::string("(>) Annihilate failed.");
        }

        if (not m.template apply<LESS_EQUAL>(rd, rd).equals(one))
        {
            return std::string("(<=) Annihilate failed.");
        }

        if (not m.template apply<GREATER_EQUAL>(rd, rd).equals(one))
        {
            return std::string("(>=) Annihilate failed.");
        }

        if (not m.template apply<MIN>(rd, zero).equals(zero))
        {
            return std::string("MIN Absorbing element failed.");
        }

        if (not m.template apply<MIN>(rd, sup).equals(rd))
        {
            return std::string("MIN Neutral element failed.");
        }

        if (not m.template apply<MAX>(rd, sup).equals(sup))
        {
            return std::string("MAX Absorbing element failed.");
        }

        if (not m.template apply<MAX>(rd, zero).equals(rd))
        {
            return std::string("MAX Neutral element failed.");
        }

        return std::string("OK");
    }

    template<std::size_t P>
    auto test_var_swap ( manager_t<P>&       m
                       , mvl_function const& f
                       , mdd_t<P> const&     d
                       , int_rng<level_t>&   rngVarLevel )
    {
        auto constexpr N  = 5;
        auto const& order = m.get_order();

        for (auto i = 0; i < N; ++i)
        {
            auto const index = order[rngVarLevel.next_int()];
            m.swap_vars(index);
        }

        return test_evaluate(m, f, d);
    }

    template<std::size_t P>
    auto test_var_sift ( manager_t<P>&       m
                       , mvl_function const& f
                       , mdd_t<P> const&     d )
    {
        m.collect_garbage();
        m.sift_variables();
        auto const result   = test_evaluate(m, f, d);
        auto const newCount = std::to_string(m.vertex_count(d));

        return result + std::string(" New vertex count ") + newCount;
    }

    template<std::size_t P>
    auto test_mdd_random ( std::size_t const n
                         , order_e const     order  = order_e::Default
                         , domain_e const    domain = domain_e::Homogenous
                         , seed_t const      seed   = 0u )
    {
        auto initSeed        = 0ul == seed ? std::random_device () () : seed;
        auto seeder          = int_rng<seed_t>(0u, UIntMax, initSeed);
        auto rngProductSize  = int_rng<std::size_t>(1, MddMaxProductSize, seeder.next_int());
        auto rngVarIndex     = int_rng<index_t>(0, MddVariableCount - 1, seeder.next_int());
        auto rngVarLevel     = int_rng<level_t>(0, MddVariableCount - 2, seeder.next_int());
        auto rngRestVarIndex = int_rng<index_t>(0, MddVariableCount - 1, seeder.next_int());
        auto rngOrderShuffle = std::mt19937(seeder.next_int());
        auto rngDomain       = int_rng<unsigned>(2, P, seeder.next_int());

        std::cout << "Running " << n << " tests. Init seed was " << initSeed << '.' << '\n';

        for (auto i = 0u; i < n; ++i)
        {
            auto const varorder = get_order(order, rngOrderShuffle, MddVariableCount);
            auto const domains  = get_domains<P>(domain, MddVariableCount, rngDomain);
            auto manager        = manager_t<P>(MddVariableCount, 5'000);
            manager.set_order(varorder);
            manager.set_domains(domains);
            manager.set_cache_ratio(2);
            manager.set_pool_ratio(3);

            auto const mulLeftFold  = [&manager](auto&& ds){ return manager.template left_fold<MULTIPLIES>(ds); };
            auto const plusLeftFold = [&manager](auto&& ds){ return manager.template left_fold<PLUS>(ds);       };
            auto const mulTreeFold  = [&manager](auto&& ds){ return manager.template tree_fold<MULTIPLIES>(ds); };
            auto const plusTreeFold = [&manager](auto&& ds){ return manager.template tree_fold<PLUS>(ds);       };

            auto const function     = generate_function(MddVariableCount, MddProductCount, domains, rngProductSize, rngVarIndex);
            auto       diagram      = make_diagram<P>(manager, function, mulLeftFold, plusLeftFold);
            auto       diagram2     = make_diagram<P>(manager, function, mulTreeFold, plusTreeFold);

            std::cout << '#'                        << i                                                                 << '\n';
            std::cout << "    Diagram"                                                                                   << '\n';
            std::cout << "        Vertex count    " << manager.vertex_count(diagram)                                     << '\n';
            std::cout << "        Initial order   " << utils::concat_range(varorder, " > ")                              << '\n';
            std::cout << "        Domains         " << utils::concat_range(function.domains, " > ")                      << '\n';
            std::cout << "    Tests"                                                                                     << '\n';
            std::cout << "        Fold            " << (diagram.equals(diagram2) ? "OK" : "Failed.")                     << '\n';
            std::cout << "        Collect garbage " << test_collect_garbage<P>(manager, diagram)                         << '\n';

            manager.sift_variables();
            std::cout << "        Var sift        " << test_var_sift<P>(manager, function, diagram)                      << '\n';
            // std::cout << "        Swap var        " << test_var_swap<P>(manager, function, diagram, rngVarLevel)         << '\n';
            std::cout << "        Evaluate        " << test_evaluate<P>(manager, function, diagram)                      << '\n';
            std::cout << "        Satisfy count   " << test_satisfy_count<P>(manager, function, diagram)                 << '\n';
            std::cout << "        Satisfy all     " << test_satisfy_all<P>(manager, function, diagram)                   << '\n';
            std::cout << "        Cofactor        " << test_restrict_var<P>(manager, function, diagram, rngRestVarIndex) << '\n';
            std::cout << "        Operators       " << test_operators<P>(manager, diagram)                               << '\n';
            std::cout << "\n";
        }
    }

    template<std::size_t P>
    auto test_mdd_vector_eval( uint_v const& vector
                             , uint_v const& domains
                             , manager_t<P>& manager  )
    {
        auto const d       = manager.from_vector(vector);
        auto enumerator    = domain_iterator<P>(domains);
        auto const offsets = [&]()
        {
            auto os = std::vector<uint>(domains.size());
            os[0] = 1;
            for (auto i = 1u; i < domains.size(); ++i)
            {
                os[i] = os[i - 1] * domains[domains.size() - i];
            }
            std::reverse(std::begin(os), std::end(os));
            return os;
        }();

        while (enumerator.has_more())
        {
            auto const vals  = *enumerator;
            auto const index = [&]()
            {
                auto idx = 0u;
                for (auto i = 0u; i < vals.size(); ++i)
                {
                    idx += vals[i] * offsets[i];
                }
                return idx;
            }();
            auto const realVal = vector[index];
            auto const diagramVal = manager.evaluate(d, *enumerator);

            if (realVal != diagramVal)
            {
                return utils::concat("Failed. ", "Got ", diagramVal, ", expected ", realVal, "." );
            }

            ++enumerator;
        }

        return std::string("OK");
    }

    auto test_mdd_vector( std::size_t const n
                        , seed_t const seed = 0u )
    {
        auto constexpr VarCount = 10;
        auto constexpr P        = 4;
        using log_t    = typename log_val_traits<4>::type;
        auto initSeed  = 0ul == seed ? std::random_device () () : seed;
        auto seeder    = int_rng<seed_t>(0u, UIntMax, initSeed);
        auto rngDomain = int_rng<log_t>(2, P, seeder.next_int());
        auto rngValue  = int_rng<log_t>(0, P - 1, seeder.next_int());
        auto manager   = manager_t<P>(VarCount);

        std::cout << "Testing from_vector. Init seed was " << initSeed << '.' << '\n';

        for (auto i = 0u; i < n; ++i)
        {
            auto const domains    = utils::fill_vector(VarCount, [&](auto const){ return rngDomain.next_int(); });
            auto const domainProd = std::reduce(std::begin(domains), std::end(domains), 1u, std::multiplies<>());
            auto const vector     = utils::fill_vector(domainProd, [&](auto const){ return rngValue.next_int(); });
            manager.set_domains(domains);
            std::cout << '#' << i << ' ' << test_mdd_vector_eval<P>(vector, domains, manager) << '\n';

            manager.clear();
        }
        {
            auto m = manager_t<4>(3);
            auto const domains = {2u, 2u, 4u};
            auto const vector  = {0u, 0u, 0u, 0u, 0u, 1u, 1u, 2u, 0u, 1u, 1u, 2u, 0u, 2u, 3u, 3u};
            m.set_domains(domains);
            std::cout << "#_ " << test_mdd_vector_eval<4>(vector, domains, m) << '\n';
        }
        {
            auto m = manager_t<3>(4);
            auto const domains = {2u, 3u, 2u, 3u};
            auto const vector  = {0u, 1u, 1u, 1u, 1u, 1u, 0u, 1u, 1u, 1u, 1u, 1u, 0u, 1u, 1u, 1u, 1u, 1u, 0u, 1u, 1u, 1u, 1u, 1u, 1u, 2u, 2u, 2u, 2u, 2u, 1u, 2u, 2u, 2u, 2u, 2u};
            m.set_domains(domains);
            std::cout << "#_ " << test_mdd_vector_eval<3>(vector, domains, m) << '\n';
        }
        {
            auto m = manager_t<3>(4);
            auto const domains = {3u, 2u, 2u, 3u};
            auto const vector  = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 0u, 0u, 0u, 0u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u};
            m.set_domains(domains);
            std::cout << "#_ " << test_mdd_vector_eval<3>(vector, domains, m) << '\n';
        }
        std::cout << '\n';
    }
}

#endif