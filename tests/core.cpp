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


namespace teddy::test
{
    namespace rs = std::ranges;

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

    struct expr_node_variable {};
    struct expr_node_constant {};
    struct expr_node_operation {};

    /**
     *  Node of an Abstract Syntax Tree.
     */
    class expression_node
    {
    public:
        using op_t = uint_t(*)(uint_t, uint_t);

    private:
        struct operation_t
        {
            operation_t ( op_t const                       o
                        , std::unique_ptr<expression_node> l
                        , std::unique_ptr<expression_node> r ) :
                op_ (o),
                l_  (std::move(l)),
                r_  (std::move(r))
            {
            }

            op_t op_;
            std::unique_ptr<expression_node> l_;
            std::unique_ptr<expression_node> r_;
        };

        struct variable_t
        {
            variable_t (index_t const i) : i_ (i) {};
            index_t i_;
        };

        struct constant_t
        {
            constant_t (uint_t const c) : c_ (c) {};
            uint_t c_;
        };

    public:
        expression_node (expr_node_variable, index_t const i) :
            data_ (std::in_place_type<variable_t>, i)
        {
        }

        expression_node (expr_node_constant, uint_t const c) :
            data_ (std::in_place_type<constant_t>, c)
        {
        }

        expression_node ( expr_node_operation
                        , op_t const o
                        , std::unique_ptr<expression_node> l
                        , std::unique_ptr<expression_node> r) :
            data_ ( std::in_place_type<operation_t>
                  , o
                  , std::move(l)
                  , std::move(r) )
        {
        }

        auto is_variable () const -> bool
        {
            return std::holds_alternative<variable_t>(data_);
        }

        auto is_constant () const -> bool
        {
            return std::holds_alternative<constant_t>(data_);
        }

        auto is_operation () const -> bool
        {
            return std::holds_alternative<operation_t>(data_);
        }

        auto get_index () const -> index_t
        {
            return std::get<variable_t>(data_).i_;
        }

        auto get_value () const -> uint_t
        {
            return std::get<constant_t>(data_).c_;
        }

        auto evaluate (uint_t const l, uint_t const r) const -> uint_t
        {
            return std::get<operation_t>(data_).op_(l, r);
        }

        auto get_left () const -> expression_node const&
        {
            return *std::get<operation_t>(data_).l_;
        }

        auto get_right () const -> expression_node const&
        {
            return *std::get<operation_t>(data_).r_;
        }

    private:
        std::variant<operation_t, variable_t, constant_t> data_;
    };

    namespace
    {
        auto op_max (uint_t const l, uint_t const r) -> uint_t
        {
            return l < r ? r : l;
        }

        auto op_min (uint_t const l, uint_t const r) -> uint_t
        {
            return l < r ? l : r;
        }
    }

    auto generate_expression_tree
        ( std::size_t const varcount
        , rng_t&            rngtype
        , rng_t&            rngbranch )
    {
        auto go = [&, i = 0u](auto& self, auto const n) mutable
        {
            if (n == 1)
            {
                return std::make_unique<expression_node>(
                    expr_node_variable(),
                    i++
                );
            }
            else
            {
                auto denomdist = std::uniform_int_distribution<uint_t>(2, 10);
                auto typedist  = std::uniform_real_distribution(0.0, 1.0);
                auto const denom   = denomdist(rngbranch);
                auto const lhssize = std::max(1ul, n / denom);
                auto const rhssize = n - lhssize;
                auto const p   = typedist(rngtype);
                auto const op  = p < 0.5 ? op_min : op_max;
                return std::make_unique<expression_node>(
                    expr_node_operation(),
                    op,
                    self(self, lhssize),
                    self(self, rhssize)
                );
            }
        };
        return go(go, varcount);
    }

    auto evaluate_expression_tree ( expression_node const&     root
                                  , std::vector<uint_t> const& vs )
    {
        auto const go = [&vs](auto self, auto const& node)
        {
            if (node.is_variable())
            {
                return vs[node.get_index()];
            }
            else if (node.is_constant())
            {
                return node.get_value();
            }
            else
            {
                assert(node.is_operation());
                auto const l = self(self, node.get_left());
                auto const r = self(self, node.get_right());
                return node.evaluate(l, r);
            }
        };
        return go(go, root);
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
                    return rs::end(fixed) == rs::find_if(fixed,
                        [i](auto const  p)
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
                && rs::equal(indices_, rhs.indices_)
                && rs::equal(domains_, rhs.domains_);
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