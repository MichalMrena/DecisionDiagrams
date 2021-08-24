#include "teddy/teddy.hpp"
#include <random>

namespace teddy::test
{
    using bin_f = uint_t(*)(uint_t, uint_t);

    enum class node_type
    {
        var, op
    };

    struct expr_node
    {
        struct var_node
        {
            index_t index;
        };

        struct op_node
        {
            expr_node* lhs;
            expr_node* rhs;
            bin_f      op;
        };

        node_type type;
        union
        {
            var_node var;
            op_node op;
        };

        expr_node (index_t const i) :
            type {node_type::var},
            var  {i}
        {
        }

        expr_node (expr_node* const l, expr_node* const r, bin_f const o) :
            type {node_type::op},
            op   {l, r, o}
        {
        }
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
            switch (root->type)
            {
                case node_type::var: break;
                case node_type::op:
                    delete root->op.lhs;
                    delete root->op.rhs;
                    break;
                default: break;
            }
        }
    };

    auto evaluate (expr_node const& n, std::vector<uint_t> const& vs) -> uint_t
    {
        auto const go = [&vs](auto&& go_, auto const& n)
        {
            switch (n.type)
            {
                case node_type::var:
                {
                    return vs[n.var.index];
                }

                case node_type::op:
                {
                    auto const l = go_(go_, *n.op.lhs);
                    auto const r = go_(go_, *n.op.rhs);
                    return n.op.op(l, r);
                }
            }
        };
        return go(go, n);
    }

    template<class Dat, degree Deg, domain Dom>
    auto generate_diagram
        (expr_node const& n, diagram_manager<Dat, Deg, Dom>& manager)
    {
        // TODO
    }

    template<class Int>
    using int_dist = std::uniform_int_distribution<Int>;

    auto constexpr max_op = [](uint_t const l, uint_t const r) -> uint_t
    {
        return l < r ? r : l;
    };

    auto constexpr min_op = [](uint_t const l, uint_t const r) -> uint_t
    {
        return l < r ? l : r;
    };

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

        auto const gen = [&]
            (auto&& gen_, auto const s) -> expr_node*
        {
            auto const stepsFrom = uint_t {0};
            auto const stepsTo   = static_cast<uint_t>(steps - 1u);
            auto stepsDst        = int_dist<uint_t>(stepsFrom, stepsTo);

            if (0 == s)
            {
                return new expr_node(indexDst(indexRng));
            }
            else
            {
                auto op = opDst(opRng) < 0.5 ? +max_op : +min_op;
                auto l  = gen_(gen_, steps - 1);
                auto r  = gen_(gen_, steps - 1);
                return new expr_node(l, r, op);
            }
        };

        return expr_tree(gen(gen, steps));
    }
}

auto main () -> int
{
    // auto const seed = std::random_device()();
    auto const seed = 144;
    auto seeder = std::default_random_engine(seed);
    auto exp    = teddy::test::generate_expr_tree(seeder, 5, 3);

    return 0;
}