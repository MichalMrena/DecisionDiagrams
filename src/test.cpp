#include "teddy/teddy.hpp"
#include <random>
#include <cstdio>
#include <iostream>

namespace teddy::test
{
    enum class node_type
    {
        Var, Op
    };

    enum class op_type
    {
        Min, Max
    };

    class expr_node
    {
        struct var_node
        {
            index_t index;
        };

        struct op_node
        {
            expr_node* lhs;
            expr_node* rhs;
            op_type    op;
        };

    public:
        expr_node (index_t const i) :
            type_ {node_type::Var},
            var_  {i}
        {
        }

        expr_node (expr_node* const l, expr_node* const r, op_type const o) :
            type_ {node_type::Op},
            op_   {l, r, o}
        {
        }

        auto type   () const { return type_; }
        auto is_op  () const { return type_ == node_type::Op; }
        auto is_var () const { return type_ == node_type::Var; }
        auto lhs    () const { assert(is_op());  return op_.lhs; }
        auto rhs    () const { assert(is_op());  return op_.rhs; }
        auto op     () const { assert(is_op());  return op_.op; }
        auto index  () const { assert(is_var()); return var_.index; }

    private:
        node_type type_;
        union
        {
            var_node var_;
            op_node op_;
        };
    };

    struct expr_tree
    {
        expr_node* root;

        expr_tree (expr_node* const r) :
            root {r}
        {
        }

        ~expr_tree()
        {
            switch (root->type())
            {
                case node_type::Var: break;
                case node_type::Op:
                    delete root->lhs();
                    delete root->rhs();
                    break;
                default: break;
            }
        }
    };

    auto evaluate (expr_node const& n, std::vector<uint_t> const& vs) -> uint_t
    {
        auto const eval_op = [](auto const o, auto const l, auto const r)
        {
            switch (o)
            {
                case op_type::Min:
                    return l < r ? l : r;

                case op_type::Max:
                    return l < r ? r : l;
            }
        };

        auto const go = [&vs, eval_op](auto&& go_, auto const& n)
        {
            switch (n.type())
            {
                case node_type::Var:
                {
                    return vs[n.index()];
                }

                case node_type::Op:
                {
                    auto const l = go_(go_, *n.lhs());
                    auto const r = go_(go_, *n.rhs());
                    return eval_op(n.op(), l, r);
                }
            }
        };
        return go(go, n);
    }

    template<class Dat, degree Deg, domain Dom>
    auto create_diagram
        (expr_node const& n, diagram_manager<Dat, Deg, Dom>& manager)
    {
        auto const apply_op =
            [&manager](auto const op, auto const& l, auto const& r)
        {
            switch (op)
            {
                case op_type::Min:
                    return manager.template apply<MIN>(l, r);

                case op_type::Max:
                    return manager.template apply<MAX>(l, r);
            }
        };

        auto const go =
            [&manager, &apply_op](auto&& go_, auto const& node)
        {
            switch (node.type())
            {
                case node_type::Var:
                {
                    return manager.variable(node.index());
                }

                case node_type::Op:
                {
                    auto const l = go_(go_, *node.lhs());
                    auto const r = go_(go_, *node.rhs());
                    return apply_op(node.op(), l, r);
                }
            }
        };

        return go(go, n);
    }

    template<class Int>
    using int_dist = std::uniform_int_distribution<Int>;

    auto generate_expr_tree
        ( std::default_random_engine& seeder
        , std::size_t const varCount
        , std::size_t const steps ) -> expr_tree
    {
        assert(varCount > 0);
        static auto indexFrom = index_t {0};
        static auto indexTo   = static_cast<index_t>(varCount - 1u);
        static auto indexRng  = std::default_random_engine(seeder());
        static auto indexDst  = int_dist<index_t>(indexFrom, indexTo);
        static auto stepsRng  = std::default_random_engine(seeder());
        static auto opRng     = std::default_random_engine(seeder());
        static auto opDst     = std::uniform_real_distribution(0.0, 1.0);

        auto const go = []
            (auto&& go_, auto const s) -> expr_node*
        {
            if (0 == s)
            {
                return new expr_node(indexDst(indexRng));
            }
            else
            {
                auto const stepsFrom = uint_t {0};
                auto const stepsTo   = static_cast<uint_t>(s - 1u);
                auto stepsDst        = int_dist<uint_t>(stepsFrom, stepsTo);
                auto op = opDst(opRng) < 0.5 ? op_type::Max : op_type::Min;
                auto l  = go_(go_, stepsDst(stepsRng));
                auto r  = go_(go_, stepsDst(stepsRng));
                return new expr_node(l, r, op);
            }
        };

        return expr_tree(go(go, steps));
    }
}

auto main () -> int
{
    // auto const seed = std::random_device()();
    auto const seed = 144;
    auto seeder  = std::default_random_engine(seed);
    auto expr    = teddy::test::generate_expr_tree(seeder, 5, 3);
    auto manager = teddy::bdd_manager(5, 10'000);
    auto diagram = teddy::test::create_diagram(*expr.root, manager);
    manager.to_dot_graph(std::cout);
    // manager.to_dot_graph(std::cout, diagram);

    std::puts("End of main().");
    return 0;
}