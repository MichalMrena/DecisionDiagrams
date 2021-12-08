#define TEDDY_VERBOSE
#undef TEDDY_VERBOSE

#include "teddy/teddy.hpp"
#include <algorithm>
#include <array>
#include <fstream>
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
    namespace rs = std::ranges;
        auto ofs = std::ofstream("sift_data.txt");

    struct minmax_expr
    {
        std::vector<std::vector<uint_t>> terms;
    };

    struct constant_expr
    {
        uint_t val;
    };

    using expr_var = std::variant<minmax_expr, constant_expr>;

    using rng_t = std::mt19937_64;

    template<class Int>
    using int_dist_t = std::uniform_int_distribution<Int>;

    /**
     *  Generates random minmax expression.
     */
    auto generate_expression
        ( rng_t&            indexRng
        , std::size_t const varCount
        , std::size_t const termCount
        , std::size_t const termSize )
    {
        assert(varCount > 0);
        static auto indexFrom = index_t {0};
        static auto indexTo   = static_cast<index_t>(varCount - 1u);
        static auto indexDst  = int_dist_t<index_t>(indexFrom, indexTo);

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

    /**
     *  Evaluates @p expr using values of variables in @p vs .
     */
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

    struct domain_iterator_sentinel {};

    /**
     *  Iterates domain of a function.
     */
    class domain_iterator
    {
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::vector<uint_t>;
        using pointer           = value_type*;
        using reference         = value_type&;
        using iterator_category = std::input_iterator_tag;

    public:
        /**
         *  Initializes this as end iterator.
         */
        domain_iterator () :
            domains_ ({}),
            indices_ ({}),
            varVals_ ({})
        {
        }

        /**
         *  Uses implicit order where x0 is the
         *  least significant (changes most often).
         *  @p domains of individual variables
         */
        domain_iterator
            (std::vector<uint_t> domains) :
            domain_iterator
                ( std::move(domains)
                , utils::fill_vector(domains.size(), utils::identity)
                , {} )
        {
        }

        /**
         *  Uses order of variables defined in @p order . Variable with
         *  index @c order[0] changes most often, then variable with
         *  index @c order[1] and so on...
         *  @p domains of individual variables
         *  @p order   order in which variables are incremented
         */
        domain_iterator
            (std::vector<uint_t> domains, std::vector<index_t> order) :
            domain_iterator (std::move(domains), std::move(order), {})
        {
        }

        /**
         *  Uses order of variables defined in @p order . Variable with
         *  index @c order[0] changes most often, then variable with
         *  index @c order[1] and so on... while skipping variables
         *  defined as fixed by @p fixed .
         *  @p domains of individual variables
         *  @p order   order in which variables are incremented
         *  @p fixed   defines variables with fixed value
         */
        domain_iterator
            ( std::vector<uint_t>                     domains
            , std::vector<index_t>                    order
            , std::vector<std::pair<index_t, uint_t>> fixed ) :
            domains_ (std::move(domains)),
            indices_ ([&order, &fixed]()
            {
                auto is = std::vector<index_t>();
                rs::copy_if(order, std::back_inserter(is),
                    [&fixed](auto const i)
                {
                    return rs::end(fixed) == rs::find_if(fixed, [i](auto&& p)
                    {
                        return p.first == i;
                    });
                });
                return is;
            }()),
            varVals_ ([this, &fixed, &domains]()
            {
                auto vs = std::vector<uint_t>(domains_.size());
                for (auto const& [i, v] : fixed)
                {
                    varVals_[i] = v;
                }
                return vs;
            }())
        {
        }

        auto operator* () const -> std::vector<uint_t> const&
        {
            return varVals_;
        }

        auto operator++ () -> domain_iterator&
        {
            auto overflow = false;

            for (auto const i : indices_)
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
                domains_.clear();
                indices_.clear();
                varVals_.clear();
            }

            return *this;
        }

        auto operator++ (int) -> domain_iterator
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        auto operator== (domain_iterator const& rhs) const -> bool
        {
            return rs::equal(varVals_, rhs.varVals_)
               and rs::equal(indices_, rhs.indices_)
               and rs::equal(domains_, rhs.domains_);
        }

        auto operator!= (domain_iterator const& rhs) const -> bool
        {
            return !(rhs == *this);
        }

        auto operator== (domain_iterator_sentinel) const -> bool
        {
            return varVals_.empty();
        }

        auto operator!= (domain_iterator_sentinel) const -> bool
        {
            return not varVals_.empty();
        }

    protected:
        std::vector<uint_t>  domains_;
        std::vector<index_t> indices_;
        std::vector<uint_t>  varVals_;
    };

    /**
     *  Evaluates @p expr for each element of its domain gived by @p iterator .
     */
    class evaluating_iterator
    {
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = uint_t;
        using pointer           = value_type*;
        using reference         = value_type&;
        using iterator_category = std::input_iterator_tag;

    public:
        evaluating_iterator () :
            iterator_ (),
            expr_     (nullptr)
        {
        }

        evaluating_iterator (domain_iterator iterator, expr_var const& expr) :
            iterator_ (std::move(iterator)),
            expr_     (&expr)
        {
        }

        auto operator* () const -> uint_t
        {
            return evaluate_expression(*expr_, *iterator_);
        }

        auto operator++ () -> evaluating_iterator&
        {
            ++iterator_;
            return *this;
        }

        auto operator++ (int) -> evaluating_iterator
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        auto operator== (domain_iterator_sentinel const s) const -> bool
        {
            return iterator_ == s;
        }

        auto operator!= (domain_iterator_sentinel const s) const -> bool
        {
            return iterator_ != s;
        }

        auto var_vals () const -> std::vector<uint_t> const&
        {
            return *iterator_;
        }

    private:
        domain_iterator iterator_;
        expr_var const* expr_;
    };

    /**
     *  Proxy output iterator that feeds outputed values into @p f .
     */
    template<class F>
    class forwarding_iterator
    {
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = forwarding_iterator&;
        using pointer           = value_type;
        using reference         = value_type;
        using iterator_category = std::output_iterator_tag;

    public:
        forwarding_iterator ()               { }
        forwarding_iterator (F& f) : f_ (&f) { }

        auto operator++ () -> forwarding_iterator&
        {
            return *this;
        }

        auto operator++ (int) -> forwarding_iterator&
        {
            return *this;
        }

        auto operator* () -> forwarding_iterator&
        {
            return *this;
        }

        auto operator= (auto&& arg) -> forwarding_iterator&
        {
            (*f_)(std::forward<decltype(arg)>(arg));
            return *this;
        }

        auto operator= (auto&& arg) const -> forwarding_iterator const&
        {
            (*f_)(std::forward<decltype(arg)>(arg));
            return *this;
        }

    private:
        F* f_ {nullptr};
    };

    enum class fold_e
    {
        Left, Tree
    };

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

        auto get_status()
        {
            return status_;
        }

        auto get_message ()
        {
            return std::string_view(msg_);
        }

    private:
        bool        status_;
        std::string msg_;
    };

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
    template<class Dat, class Deg>
    auto test_fold
        ( diagram<Dat, Deg> const& diagram1
        , diagram<Dat, Deg> const& diagram2 )
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
        manager.gc();
        auto const totalNodeCount   = manager.node_count();
        auto const diagramNodeCount = manager.node_count(diagram);
        if (totalNodeCount == diagramNodeCount)
        {
            return test_result(true);
        }
        else
        {
            return test_result(false, "Node count missmatch.");
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
                auto outF = [&vals, k](auto&&)
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
        auto const bd   = manager.booleanize(diagram, utils::not_zero);
        auto const rd   = manager.reduce(diagram);

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

        if (not manager.template apply<MULTIPLIES<2>>(rd, zero).equals(zero))
        {
            return test_result(false, "MULTIPLIES absorbing failed.");
        }

        if (not manager.template apply<MULTIPLIES<4>>(rd, one).equals(rd))
        {
            return test_result(false, "MULTIPLIES neutral failed.");
        }

        if (not manager.template apply<PLUS<4>>(rd, zero).equals(rd))
        {
            return test_result(false, "PLUS neutral failed.");
        }

        if (not manager.template apply<EQUAL_TO>(rd, rd).equals(one))
        {
            return test_result(false, "EQUAL_TO annihilate failed.");
        }

        if (not manager.template apply<NOT_EQUAL_TO>(rd, rd).equals(zero))
        {
            return test_result(false, "NOT_EQUAL_TO annihilate failed.");
        }

        if (not manager.template apply<LESS>(rd, rd).equals(zero))
        {
            return test_result(false, "LESS annihilate failed.");
        }

        if (not manager.template apply<GREATER>(rd, rd).equals(zero))
        {
            return test_result(false, "GREATER annihilate failed.");
        }

        if (not manager.template apply<LESS_EQUAL>(rd, rd).equals(one))
        {
            return test_result(false, "LESS_EQUAL annihilate failed.");
        }

        if (not manager.template apply<GREATER_EQUAL>(rd, rd).equals(one))
        {
            return test_result(false, "GREATER_EQUAL annihilate failed.");
        }

        if (not manager.template apply<MIN>(rd, zero).equals(zero))
        {
            return test_result(false, "MIN absorbing failed.");
        }

        if (not manager.template apply<MIN>(rd, sup).equals(rd))
        {
            return test_result(false, "MIN neutral failed.");
        }

        if (not manager.template apply<MAX>(rd, sup).equals(sup))
        {
            return test_result(false, "MAX absoring failed.");
        }

        if (not manager.template apply<MAX>(rd, zero).equals(rd))
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
                rs::reverse(order);
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
     * 
     */
    template<class Dat, class Deg, class Dom>
    auto test_var_sift
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg>&              diagram
        , expr_var const&                 expr )
    {
        manager.gc();
        ofs << manager.node_count() << " ";
        manager.sift();
        ofs << manager.node_count() << "\n";
        return test_evaluate(manager, diagram, expr);
    }

    /**
     *  Runs all test. Creates diagram represeting @p expr using @p manager .
     */
    template<class Manager>
    auto test_all
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

        auto diagram2s = utils::fill_vector(testCount, [&](auto const k)
        {
            return create_diagram(exprs[k], managers[k], fold_e::Tree);
        });

        using namespace std::string_view_literals;
        using result_opt = std::optional<test_result>;
        auto const tests = { "evaluate"sv
                           , "fold"sv
                           , "gc"sv
                           , "satisfy_count"sv
                           , "satisfy_all"sv
                           , "operators"sv
                           , "cofactors"sv
                           , "from_vector"sv
                           , "var_sift"sv };
        auto results = std::unordered_map< std::string_view
                                         , std::vector<result_opt> >();
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
                // Erase one line from console.
                std::cout << "\033[A";
            }
            output_results();
            std::cout << std::flush;
        };

        std::cout << wrap_yellow(name) << '\n';
        std::cout << "  node counts: ";
        for (auto k = 0u; k < testCount; ++k)
        {
            std::cout << managers[k].node_count(diagram1s[k]) << ' ';
        }
        std::cout << "\n\n";

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
            results.at("var_sift")[k]
                = test_var_sift(managers[k], diagram1s[k], exprs[k]);

            refresh_results();
        }

        std::cout << '\n';
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

auto main () -> int
{
    namespace us = teddy::utils;
    namespace ts = teddy::test;

    auto constexpr M     = 4;
    auto const varCount  = 13;
    auto const termCount = 20;
    auto const termSize  = 5;
    auto const nodeCount = 100;
    auto const testCount = std::thread::hardware_concurrency() + 2;
    auto       seedSrc   = std::random_device();
    // auto const seedSrc   = std::integral_constant<long, 2928425735>();
    auto const initSeed  = seedSrc();
    auto constexpr IsFixedSeed = not std::same_as< std::random_device
                                                 , decltype(seedSrc) >;

    auto seeder = ts::rng_t(initSeed);
    auto rngs = us::fill_vector(testCount, [&seeder](auto const)
    {
        // One rng to rule them all.
        // Not technically correct but
        // it should be good enough for the purpose of these tests.
        return ts::rng_t(seeder());
    });

    auto const exprs = [=, &rngs]()
    {
        auto res = us::fill_vector(testCount - 2, [=, &rngs](auto const k)
        {
            return ts::generate_expression( rngs[k], varCount
                                          , termCount, termSize );
        });
        res.emplace_back(std::in_place_type_t<ts::constant_expr>(), 0);
        res.emplace_back(std::in_place_type_t<ts::constant_expr>(), 1);
        return res;
    }();

    auto orders = us::fmap(rngs, [testCount](auto& rng)
    {
        return ts::random_order(varCount, rng);
    });

    auto domains = us::fmap(rngs, [&](auto& rng)
    {
        return ts::random_domains<M>(varCount, rng);
    });

    auto bddManagers = us::fill_vector(testCount - 2, [&](auto const k)
    {
        return teddy::bdd_manager(varCount, nodeCount, orders[k]);
    });

    auto mddManagers = us::fill_vector(testCount - 2, [&](auto const k)
    {
        return teddy::mdd_manager<M>(varCount, nodeCount, orders[k]);
    });

    auto imddManagers = us::fill_vector(testCount - 2, [&]
        (auto const k) mutable
    {
        return teddy::imdd_manager(varCount, nodeCount, domains[k], orders[k]);
    });

    auto ifmddManagers = us::fill_vector(testCount - 2, [&]
        (auto const k) mutable
    {
        return teddy::ifmdd_manager<M>( varCount, nodeCount
                                      , domains[k], orders[k] );
    });

    // Add constant functions.
    bddManagers.emplace_back(0, 2);
    bddManagers.emplace_back(0, 2);
    mddManagers.emplace_back(0, 2);
    mddManagers.emplace_back(0, 2);
    imddManagers.emplace_back(0, 2, std::vector<teddy::uint_t>());
    imddManagers.emplace_back(0, 2, std::vector<teddy::uint_t>());
    ifmddManagers.emplace_back(0, 2, std::vector<teddy::uint_t>());
    ifmddManagers.emplace_back(0, 2, std::vector<teddy::uint_t>());

    if (not teddy::test::ofs.is_open())
    {
        std::cout << "not opened" << '\n';
        return 1;
    }

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