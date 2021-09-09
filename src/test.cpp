#include "teddy/teddy.hpp"
#include <random>
#include <cstdio>
#include <iostream>
#include <ranges>

namespace teddy::test
{
    // enum class node_type
    // {
    //     Var, Op
    // };

    // enum class op_type
    // {
    //     Min, Max
    // };

    // /**
    //  *  Node of an expression tree.
    //  *  It can either represent a variable or a binary operation.
    //  */
    // class expr_node
    // {
    //     struct var_node
    //     {
    //         index_t index;
    //     };

    //     struct op_node
    //     {
    //         expr_node* lhs;
    //         expr_node* rhs;
    //         op_type    op;
    //     };

    // public:
    //     expr_node (index_t const i) :
    //         type_ {node_type::Var},
    //         var_  {i}
    //     {
    //     }

    //     expr_node (expr_node* const l, expr_node* const r, op_type const o) :
    //         type_ {node_type::Op},
    //         op_   {l, r, o}
    //     {
    //     }

    //     auto type   () const { return type_; }
    //     auto is_op  () const { return type_ == node_type::Op; }
    //     auto is_var () const { return type_ == node_type::Var; }
    //     auto lhs    () const { assert(is_op());  return op_.lhs; }
    //     auto rhs    () const { assert(is_op());  return op_.rhs; }
    //     auto op     () const { assert(is_op());  return op_.op; }
    //     auto index  () const { assert(is_var()); return var_.index; }

    // private:
    //     node_type type_;
    //     union
    //     {
    //         var_node var_;
    //         op_node op_;
    //     };
    // };

    // /**
    //  *  Wrapper for the root of an expression tree.
    //  *  Ensures correct deallocation.
    //  */
    // class expr_tree
    // {
    // public:
    //     expr_tree (expr_node* const r) :
    //         root_ {r}
    //     {
    //     }

    //     ~expr_tree()
    //     {
    //         auto const del = [](auto&& del_, auto const node) -> void
    //         {
    //             switch (node->type())
    //             {
    //                 case node_type::Op:
    //                     del_(del_, node->lhs());
    //                     del_(del_, node->rhs());
    //                     break;

    //                 default:
    //                     break;
    //             }
    //             delete node;
    //         };
    //         del(del, root_);
    //     }

    //     auto root () const -> expr_node*
    //     {
    //         return root_;
    //     }

    // private:
    //     expr_node* root_;
    // };

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

    // /**
    //  *  Evaluates expression tree @p expr using variable values in @p vs .
    //  */
    // auto evaluate_expr
    //     ( expr_tree const&           expr
    //     , std::vector<uint_t> const& vs ) -> uint_t
    // {
    //     auto const eval_op = [](auto const o, auto const l, auto const r)
    //     {
    //         switch (o)
    //         {
    //             case op_type::Min:
    //                 return l < r ? l : r;

    //             case op_type::Max:
    //                 return l < r ? r : l;
    //         }
    //     };

    //     auto const go = [&vs, &eval_op](auto&& go_, auto const& node)
    //     {
    //         switch (node.type())
    //         {
    //             case node_type::Var:
    //             {
    //                 return vs[node.index()];
    //             }

    //             case node_type::Op:
    //             {
    //                 auto const l = go_(go_, *node.lhs());
    //                 auto const r = go_(go_, *node.rhs());
    //                 return eval_op(node.op(), l, r);
    //             }
    //         }
    //     };
    //     return go(go, *expr.root());
    // }

    template<class Dat, degree Deg, domain Dom>
    auto create_diagram
        ( expression const&               expr
        , diagram_manager<Dat, Deg, Dom>& manager )
    {
        using diagram_t = typename diagram_manager<Dat, Deg, Dom>::diagram_t
        auto termsDs = std::vector<diagram_t>();
        for (auto const& eTerm : expr.terms)
        {
            termDs.emplace_back(manager.left_fold(manager.variables(eTerm)));
        }
        return manager.left_fold(termDs);
    }

    // /**
    //  *  Creates diagram from the expression tree @p expr using @p manager .
    //  */
    // template<class Dat, degree Deg, domain Dom>
    // auto create_diagram
    //     ( expr_tree const&                expr
    //     , diagram_manager<Dat, Deg, Dom>& manager )
    // {
    //     auto const apply_op =
    //         [&manager](auto const op, auto const& l, auto const& r)
    //     {
    //         switch (op)
    //         {
    //             case op_type::Min:
    //                 return manager.template apply<MIN>(l, r);

    //             case op_type::Max:
    //                 return manager.template apply<MAX>(l, r);
    //         }
    //     };

    //     auto const go =
    //         [&manager, &apply_op](auto&& go_, auto const& node)
    //     {
    //         switch (node.type())
    //         {
    //             case node_type::Var:
    //             {
    //                 return manager.variable(node.index());
    //             }

    //             case node_type::Op:
    //             {
    //                 auto const l = go_(go_, *node.lhs());
    //                 auto const r = go_(go_, *node.rhs());
    //                 return apply_op(node.op(), l, r);
    //             }
    //         }
    //     };

    //     return go(go, *expr.root());
    // }

    template<class Int>
    using int_dist = std::uniform_int_distribution<Int>;

    // /**
    //  * Generates random expression tree.
    //  */
    // auto generate_expr_tree
    //     ( std::default_random_engine& seeder
    //     , std::size_t const           varCount
    //     , std::size_t const           depth ) -> expr_tree
    // {
    //     assert(varCount > 0);
    //     static auto indexFrom = index_t {0};
    //     static auto indexTo   = static_cast<index_t>(varCount - 1u);
    //     static auto indexRng  = std::default_random_engine(seeder());
    //     static auto indexDst  = int_dist<index_t>(indexFrom, indexTo);
    //     static auto depthRng  = std::default_random_engine(seeder());
    //     static auto opRng     = std::default_random_engine(seeder());
    //     static auto opDst     = std::uniform_real_distribution(0.0, 1.0);

    //     auto const go = []
    //         (auto&& go_, auto const dep) -> expr_node*
    //     {
    //         if (0 == dep)
    //         {
    //             return new expr_node(indexDst(indexRng));
    //         }
    //         else
    //         {
    //             auto const depFrom   = uint_t {0};
    //             auto const depTo     = static_cast<uint_t>(dep - 1u);
    //             auto depDst          = int_dist<uint_t>(depFrom, depTo);
    //             auto op = opDst(opRng) < 0.5 ? op_type::Max : op_type::Min;
    //             auto l  = go_(go_, depDst(depthRng));
    //             auto r  = go_(go_, depDst(depthRng));
    //             return new expr_node(l, r, op);
    //         }
    //     };

    //     return expr_tree(go(go, depth));
    // }

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
                terms[k].emplace_back(indexDst(indexRng));
            }
        }

        return expression {std::move(terms)};
    }

    auto evaluate_expression
        ( expression const&          expr
        , std::vector<uint_t> const& vs )
    {
        auto const var_val = [&vs](auto)
        {

        };

        for (auto const& term : expr.terms)
        {
            std::ranges::transform_view(terms)
        }
    }

    auto constexpr CodeRed    = "\x1B[91m";
    auto constexpr CodeGreen  = "\x1B[92m";
    auto constexpr CodeYellow = "\x1B[93m";
    auto constexpr CodeReset  = "\x1B[0m";

    /**
     *  Tests whether @p diagram evaluates to the same 
     */
    template<class Dat, class Deg, class Dom>
    auto test_evaluate
        ( diagram_manager<Dat, Deg, Dom>& manager
        , diagram<Dat, Deg> const&        diagram
        , expr_tree const&                expr )
    {
        auto iterator = domain_iterator(manager.get_domains());
        while (iterator.has_more())
        {
            auto const expectedVal = evaluate_exprression(expr, *iterator);
            auto const diagramVal  = manager.evaluate(diagram, *iterator);
            if (expectedVal != diagramVal)
            {
                std::cout << CodeRed << "!!!" << CodeReset << '\n';
            }
            ++iterator;
        }

        if (not iterator.has_more())
        {
            std::cout << CodeGreen << "OK" << CodeReset << '\n';
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
        , std::default_random_engine&     seeder )
    {
        std::cout << CodeYellow << name << CodeReset << '\n';

        auto diagram = create_diagram(expr, manager);
        std::cout << "  node count: " << manager.node_count(diagram) << "\n\n";

        std::cout << "  evaluate: ";
        test_evaluate(manager, diagram, expr);
    }
}

auto main () -> int
{
    // auto const seed      = std::random_device()();
    auto const seed      = 144;
    auto const varCount  = 20;
    auto const termCount = 20;
    auto const termSize  = 5;
    auto const nodeCount = 10'000;

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