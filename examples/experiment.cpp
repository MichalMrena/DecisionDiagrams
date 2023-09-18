#include <string>
#include <sys/resource.h>

#include <algorithm>
#include <exception>
#include <libteddy/core.hpp>
#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <tuple>
#include <variant>
#include <vector>

using teddy::int32;
using teddy::int64;
using teddy::as_uindex;
using teddy::as_usize;

struct expr_node_variable {};

struct expr_node_operation {};

enum class operation_type
{
    Min,
    Max
};

class expr_node
{
private:
    struct operation_t
    {
        operation_t(
            operation_type o,
            std::vector<std::unique_ptr<expr_node>> args
        ) : op_(o), args_(std::move(args)) {}
        operation_type op_;
        std::vector<std::unique_ptr<expr_node>> args_;
    };

    struct variable_t
    {
        variable_t(int32 i) : i_(i) {}
        int32 i_;
    };

public:
    expr_node(expr_node_variable, int32 i) :
        data_(std::in_place_type<variable_t>, i)
    {
    }

    expr_node(
        expr_node_operation,
        operation_type o,
        std::vector<std::unique_ptr<expr_node>> args
    ) :
        data_(std::in_place_type<operation_t>, o, std::move(args))
    {
    }

    auto is_variable() const -> bool
    {
        return std::holds_alternative<variable_t>(data_);
    }

    auto is_operation() const -> bool
    {
        return std::holds_alternative<operation_t>(data_);
    }

    auto get_index() const -> int32
    {
        return std::get<variable_t>(data_).i_;
    }

    auto get_args() const -> std::vector<std::unique_ptr<expr_node>> const&
    {
        return std::get<operation_t>(data_).args_;
    }

    auto get_operation() const -> operation_type
    {
        return std::get<operation_t>(data_).op_;
    }

private:
    std::variant<operation_t, variable_t> data_;
};

auto make_expression_tree(
    int32 const varCount,
    std::mt19937_64& rngOperation,
    std::mt19937_64& rngArity
) -> std::unique_ptr<expr_node>
{
    int32 i = 0;
    std::uniform_int_distribution<int32> distArity(2, 5);
    auto go = [&](auto& self, int32 leafCount)
    {
        if (i >= varCount || leafCount < 2)
        {
            return std::make_unique<expr_node>(expr_node_variable(), i++);
        }

        int32 const arity = std::min(distArity(rngArity), leafCount);

        std::vector<std::unique_ptr<expr_node>> sons;
        for (int32 k = 0; k < arity - 1; ++k)
        {
            sons.push_back(self(self, leafCount / arity));
        }
        sons.push_back(self(self, leafCount / arity + leafCount % arity));

        std::uniform_real_distribution<double> distOp(0.0, 1.0);
        double const opProb = distOp(rngOperation);
        operation_type const op = opProb < 0.5
            ? operation_type::Min
            : operation_type::Max;

        return std::make_unique<expr_node>(
            expr_node_operation(),
            op,
            std::move(sons)
        );
    };
    return go(go, varCount);
}

auto tree_leaf_count (expr_node const& root) -> int64
{
    int64 count = 0;
    auto go = [&count](auto self, expr_node const& node) -> void
    {
        if (node.is_variable())
        {
            ++count;
        }
        else
        {
            for (auto const& son : node.get_args())
            {
                self(self, *son);
            }
        }
    };
    go(go, root);
    return count;
}

template<class F>
auto for_each_dfs(expr_node const& root, F f)
{
    auto nextId = 0;
    auto go = [&nextId, f](
        auto self,
        expr_node const& node,
        int64 const parentId
    ) -> void
    {
        auto const thisId = nextId;
        ++nextId;
        std::invoke(f, node, parentId, thisId);
        if (node.is_operation())
        {
            for (auto const& son : node.get_args())
            {
                self(self, *son, thisId);
            }
        }
    };
    go(go, root, -1);
}

auto dump_dot_impl (expr_node const& root) -> std::string
{
    auto out = std::string();
    out += "digraph Tree {\n";

    auto s = [](auto const x){ return std::to_string(x); };

    for_each_dfs(root, [&](auto const& node, int const, int const nodeId)
    {
        auto const label = node.is_variable()
            ? "x" + std::to_string(node.get_index())
            : (node.get_operation() == operation_type::Min ? "And" : "Or");
        out += "    " + s(nodeId) + " [label=\"" + label + "\"];\n";
    });
    out += "\n";

    for_each_dfs(root, [&out, s](auto&&, int const parentId, int const nodeId)
    {
        if (parentId != -1)
        {
            out += "    "
                + s(parentId)
                + " -> "
                + s(nodeId) + ";\n";
        }
    });
    out += "}\n";

    return out;
}

using duration_type = std::chrono::milliseconds;

template<int32 M>
auto make_diagram_bin(
    teddy::mdd_manager<M>& manager,
    expr_node const& exprRoot
)
{
    using diagram_t = typename teddy::mdd_manager<M>::diagram_t;

    auto go = [&manager](auto& self, expr_node const& node)
    {
        if (node.is_variable())
        {
            return manager.variable(node.get_index());
        }
        else
        {
            auto const& args = node.get_args();
            auto sons = std::vector<diagram_t>();
            for (auto const& arg : args)
            {
                sons.emplace_back(self(self, *arg));
            }

            switch (node.get_operation())
            {
            case operation_type::Min:
                return manager.template left_fold<teddy::ops::MIN>(sons);

            case operation_type::Max:
                return manager.template left_fold<teddy::ops::MAX>(sons);

            default:
                std::terminate();
            }
        }
    };

    auto t1 = std::chrono::high_resolution_clock::now();
    auto diagram = go(go, exprRoot);
    auto t2 = std::chrono::high_resolution_clock::now();

    return std::make_tuple(
        diagram,
        std::chrono::duration_cast<duration_type>(t2 - t1)
    );
}

template<int32 M>
auto make_diagram_nary(
    teddy::mdd_manager<M>& manager,
    expr_node const& exprRoot
)
{
    using diagram_t = typename teddy::mdd_manager<M>::diagram_t;

    auto app_2 = [&](
        auto const op,
        auto const& d1,
        auto const& d2
    )
    {
        switch (op)
        {
        case operation_type::Min:
            return manager.template apply_n<teddy::ops::MIN>(d1, d2);
        case operation_type::Max:
            return manager.template apply_n<teddy::ops::MAX>(d1, d2);
        }
        std::terminate();
        return diagram_t();
    };

    auto app_3 = [&](
        auto const op,
        auto const& d1,
        auto const& d2,
        auto const& d3
    )
    {
        switch (op)
        {
        case operation_type::Min:
            return manager.template apply_n<teddy::ops::MIN>(d1, d2, d3);
        case operation_type::Max:
            return manager.template apply_n<teddy::ops::MAX>(d1, d2, d3);
        }
        std::terminate();
        return diagram_t();
    };

    auto app_4 = [&](
        auto const op,
        auto const& d1,
        auto const& d2,
        auto const& d3,
        auto const& d4
    )
    {
        switch (op)
        {
        case operation_type::Min:
            return manager.template apply_n<teddy::ops::MIN>(d1, d2, d3, d4);
        case operation_type::Max:
            return manager.template apply_n<teddy::ops::MAX>(d1, d2, d3, d4);
        }
        std::terminate();
        return diagram_t();
    };

    auto app_5 = [&](
        auto const op,
        auto const& d1,
        auto const& d2,
        auto const& d3,
        auto const& d4,
        auto const& d5
    )
    {
        switch (op)
        {
        case operation_type::Min:
            return manager.template apply_n<teddy::ops::MIN>(d1,d2,d3,d4,d5);
        case operation_type::Max:
            return manager.template apply_n<teddy::ops::MAX>(d1,d2,d3,d4,d5);
        }
        std::terminate();
        return diagram_t();
    };

    auto go = [&](auto& self, expr_node const& node)
    {
        if (node.is_variable())
        {
            return manager.variable(node.get_index());
        }
        else
        {
            auto const& args = node.get_args();
            std::vector<diagram_t> sons;
            for (auto const& arg : args)
            {
                sons.emplace_back(self(self, *arg));
            }

            operation_type const op = node.get_operation();
            switch (args.size())
            {
            case 2:
                return app_2(op, sons[0], sons[1]);

            case 3:
                return app_3(op, sons[0], sons[1], sons[2]);

            case 4:
                return app_4(op, sons[0], sons[1], sons[2], sons[3]);

            case 5:
                return app_5(op, sons[0], sons[1], sons[2], sons[3], sons[4]);
            }
            std::terminate();
            return diagram_t();
        }
    };
    auto t1 = std::chrono::high_resolution_clock::now();
    diagram_t diagram = go(go, exprRoot);
    auto t2 = std::chrono::high_resolution_clock::now();

    return std::make_tuple(
        diagram,
        std::chrono::duration_cast<duration_type>(t2 - t1)
    );
}

auto compare_ast(int32 const varCount, int32 const replicationCount)
{
    int32 constexpr M = 3;
    int64 const initNodeCount = 1'000'000;
    std::mt19937_64 rngOperation(8946);
    std::mt19937_64 rngArity(846522);
    auto const Sep = "\t";
    auto const Eol = "\n";
    std::cout << "#"                  << Sep
              << "var-count"          << Sep
              << "tree-node-count"    << Sep
              << "diagram-node-count" << Sep
              << "bin-creation-[ms]"  << Sep
              << "nary-creation-[ms]" << Eol;

    for (auto i = 0; i < replicationCount; ++i)
    {
        std::unique_ptr<expr_node> root = make_expression_tree(
            varCount,
            rngOperation,
            rngArity
        );
        int64 const treeNodeCount = tree_leaf_count(*root);

        duration_type binDuration;
        duration_type naryDuration;
        int64 diagramNodeCount;

        std::cout << i                    << Sep
                  << varCount             << Sep
                  << treeNodeCount        << Sep;

        {
            teddy::mdd_manager<M> manager(varCount, initNodeCount);
            auto [diagram, time] = make_diagram_bin<M>(manager, *root);
            diagramNodeCount = manager.get_node_count(diagram);
            binDuration = time;
        }
        std::cout << diagramNodeCount    << Sep
                  << binDuration.count() << Sep;

        {
            teddy::mdd_manager<M> manager(varCount, initNodeCount);
            auto [diagram, time] = make_diagram_nary<M>(
                manager,
                *root
            );
            naryDuration = time;

            if (manager.get_node_count(diagram) != diagramNodeCount)
            {
                std::terminate();
            }
        }

        std::cout << naryDuration.count() << std::endl;
    }
}

int main(int, char** argv)
{
    // min stack size = 64 Mb
    // const rlim_t kStackSize = 64L * 1024L * 1024L;
    // struct rlimit rl;

    // auto result = getrlimit(RLIMIT_STACK, &rl);
    // if (result == 0)
    // {
    //     if (rl.rlim_cur < kStackSize)
    //     {
    //         rl.rlim_cur = kStackSize;
    //         result = setrlimit(RLIMIT_STACK, &rl);
    //         if (result != 0)
    //         {
    //             fprintf(stderr, "setrlimit returned result = %d\n", result);
    //         }
    //     }
    // }

    auto const varCount = std::stoi(argv[1]);
    auto const replicationCount = std::stoi(argv[2]);
    compare_ast(varCount, replicationCount);
}
