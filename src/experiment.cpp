#include <libteddy/teddy.hpp>
#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "counters.hpp"
#include "iterators.hpp"
#include "libteddy/details/operators.hpp"
#include "libteddy/details/types.hpp"
#include "trees.hpp"
#include "generators.hpp"

using teddy::as_uindex;
using teddy::as_usize;

template<int64 M, int64 N>
auto make_truth_vector (
    teddy::mdd_manager<M> const& manager,
    BinaryNode const& root
)
{
    auto eval_node = [](
        auto self,
        std::vector<int32> const& vs,
        BinaryNode const& n
    ) -> int32
    {
        if (n.is_operation())
        {
            auto const l = self(self, vs, n.get_left());
            auto const r = self(self, vs, n.get_right());
            return n.evaluate(l, r);
        }
        else
        {
            return vs[as_uindex(n.get_index())];
        }
    };

    auto const ds  = manager.get_domains();
    auto it        = domain_iterator(ds, manager.get_order());
    auto const end = domain_iterator_sentinel();
    auto vec       = std::bitset<as_usize(teddy::utils::int_pow(M, N))>();
    auto i         = 0ul;

    while (it != end)
    {
        vec[i] = eval_node(eval_node, *it, root);
        ++i;
        ++it;
    }

    return vec;
}

// F :: diagram -> ()
template<int64 M, int64 N, class F>
auto for_each_bin_ast (teddy::mdd_manager<M>& manager, F f)
{
    auto memo = std::unordered_set<void*>();
    auto gen  = BinAstGenerator(N, 0);
    auto i    = 1;
    while (not gen.is_done())
    {
        auto& root = gen.get();
        auto d = manager.from_expression_tree(root);

            std::cout << i << '\n';
            std::cout << dump_dot(root);
            std::cout << "---" << '\n';

        if (not memo.contains(d.get_root()))
        {
            std::invoke(f, d);
            memo.emplace(d.get_root());
        }
            ++i;

        gen.advance();
    }
}

auto bin_to_mw_ast (
    BinaryNode const& binRoot,
    auto& unique,
    auto* leaf
) -> MultiwayNode*
{
    auto transform = teddy::utils::fix([&](
        auto self,
        BinaryNode const& binNode
    ) -> MultiwayNode*
    {
        if (binNode.is_operation())
        {
            auto mappedSons = std::vector<MultiwayNode*>();
            mappedSons.push_back(self(self, binNode.get_left()));
            mappedSons.push_back(self(self, binNode.get_right()));
            return new MultiwayNode{NAryOpNode{
                binNode.get_operation(),
                std::move(mappedSons)
            }};
        }
        else
        {
            return leaf;
        }
    });

    auto reduce = teddy::utils::fix([](
        auto self,
        MultiwayNode* node
    ) -> MultiwayNode*
    {
        if (node->is_operation())
        {
            auto mappedSons = std::vector<MultiwayNode*>();
            for (auto* son : node->get_args())
            {
                mappedSons.push_back(self(self, son));
            }

            auto newSons = std::vector<MultiwayNode*>();
            for (auto* son : mappedSons)
            {
                if (son->is_operation()
                 && son->get_operation() == node->get_operation())
                {
                    auto& sonsSons = son->get_args();
                    newSons.insert(
                        end(newSons),
                        begin(sonsSons),
                        end(sonsSons)
                    );
                    delete son;
                }
                else
                {
                    newSons.push_back(son);
                }
            }
            std::ranges::sort(newSons);
            std::get<NAryOpNode>(node->data_).args_ = std::move(newSons);
        }
        return node;
    });

    auto uniquize = teddy::utils::fix([&unique](
        auto self,
        MultiwayNode* node
    ) -> MultiwayNode*
    {
        if (node->is_operation())
        {
            auto mappedSons = std::vector<MultiwayNode*>();
            for (auto* son : node->get_args())
            {
                mappedSons.push_back(self(self, son));
            }
            std::ranges::sort(mappedSons);

            auto newNodeKey = MultiwayNode{NAryOpNode{
                node->get_operation(),
                std::move(mappedSons)
            }};
            delete node;

            auto it = unique.find(newNodeKey);
            if (it == end(unique))
            {
                auto newNode = new MultiwayNode(newNodeKey);
                unique.try_emplace(newNodeKey, newNode);
                return newNode;
            }
            else
            {
                return it->second;
            }
        }
        else
        {
            return node;
        }
    });

    return uniquize(reduce(transform(binRoot)));
}

// F :: MultiwayNode const& -> ()
template<class F>
auto for_each_mw_ast (int32 const varCount, F f)
{
    auto unique = std::unordered_map<
        MultiwayNode,
        MultiwayNode*,
        MwNodeHash,
        MwNodeEquals
    >();

    auto leaf = new MultiwayNode{LeafNode{0}};
    auto memo = std::unordered_set<MultiwayNode*>();
    auto gen  = BinAstGenerator(varCount, 0);
    while (not gen.is_done())
    {
        auto& binRoot = gen.get();
        auto* mwRoot = bin_to_mw_ast(binRoot, unique, leaf);
        if (not memo.contains(mwRoot))
        {
            std::invoke(f, *mwRoot);
            memo.insert(mwRoot);
        }
        gen.advance();
    }

    for (auto const& [key, val] : unique)
    {
        delete val;
    }
    delete leaf;
}

auto count_binary_trees (int32 const n) -> int64
{
    // https://oeis.org/A248748

    auto gen   = BinAstGenerator(n, 0);
    auto count = int64{0};

    while (not gen.is_done())
    {
        gen.advance();
        ++count;
    }

    return count;
}

auto count_multiway_trees (int32 const varCount) -> int64
{
    auto count = int64{0};
    for_each_mw_ast(varCount, [&count](auto const&)
    {
        ++count;
    });
    return count;
}

template<int64 M>
auto create_series(
    teddy::mdd_manager<M>& manager,
    auto                   diagram
)
{
    auto series = std::vector<typename teddy::mdd_manager<M>::diagram_t>();
    series.reserve(M - 1);
    for (auto j = 1u; j < M; ++j)
    {
        auto seriesDiagram = manager.template apply<teddy::ops::GREATER_EQUAL>(
            diagram,
            manager.constant(j)
        );
        series.emplace_back(seriesDiagram);
    }
    return series;
}

template<int64 M, int64 N>
auto compare_series_parallel () -> void
{
    auto m = teddy::mdd_manager<M>(N, 1'000'000);

    auto singlebetter = int64{0};
    auto seriesbetter = int64{0};
    auto equalsize    = int64{0};

    for_each_bin_ast<M, N>(m, [&](auto const& single)
    {
        auto const series      = create_series<M>(m, single);
        auto const singlecount = m.node_count(single);
        auto const seriescount = std::invoke([&]()
        {
            auto unique = std::unordered_set<void*>();
            for (auto const& sd : series)
            {
                m.nodes_.traverse_pre(sd.get_root(), [&unique](auto const n)
                {
                    unique.insert(n);
                });
            }
            return std::size(unique);
        });
        singlebetter += static_cast<int64>(singlecount < seriescount);
        seriesbetter += static_cast<int64>(seriescount < singlecount);
        equalsize    += static_cast<int64>(seriescount == singlecount);
    });

    auto const total = (singlebetter + seriesbetter + equalsize);
    auto const relativeequal = static_cast<double>(equalsize)
                             / static_cast<double>(total);
    std::cout << singlebetter  << "\t"
              << seriesbetter  << "\t"
              << equalsize     << "\t"
              << relativeequal << '\n';
}

auto make_diagram (
    teddy::mdd_manager<3>& manager,
    MultiwayNode const& root,
    SeriesParallelTreeGenerator const& gen
)
{
    using diagram_t = teddy::mdd_manager<3>::diagram_t;

    auto indexIts = std::vector<std::vector<int32>::const_iterator>();

    auto const combinations = gen.get_combinations();
    auto leafCombinIt = begin(combinations);
    for_each_dfs(root, [&](MultiwayNode const& node, auto, auto)
    {
        if (has_leaf_son(node))
        {
            indexIts.push_back(begin(leafCombinIt->get()));
            ++leafCombinIt;
        }
    });

    auto indexItsIt = begin(indexIts);

    auto dfs = [&](auto self, MultiwayNode const& node) -> diagram_t
    {
        auto ds = std::vector<diagram_t>();
        if (has_leaf_son(node))
        {
            auto& indexIt = *indexItsIt;
            ++indexItsIt;

            for (auto* son : node.get_args())
            {
                if (son->is_variable())
                {
                    auto const index = *indexIt;
                    ++indexIt;
                    ds.emplace_back(manager.variable(index));
                }
                else
                {
                    ds.emplace_back(self(self, *son));
                }
            }
        }
        else if (not node.is_variable())
        {
            for (auto* son : node.get_args())
            {
                ds.emplace_back(self(self, *son));
            }
        }
        else
        {
            unreachable();
            return diagram_t();
        }

        switch (node.get_operation())
        {
        case Operation::And:
            return manager.left_fold<teddy::ops::AND>(ds);

        case Operation::Or:
            return manager.left_fold<teddy::ops::OR>(ds);

        default:
            unreachable();
            return diagram_t();
        }
    };
    return dfs(dfs, root);
}

auto make_diagram (
    teddy::mdd_manager<3>& manager,
    MultiwayNode const& root
)
{
    using diagram_t = teddy::mdd_manager<3>::diagram_t;

    auto const mk_node = [&manager]
        (auto& self, MultiwayNode const& node) -> diagram_t
    {
        if (node.is_variable())
        {
            return manager.variable(node.get_index());
        }
        else
        {
            auto args = std::vector<diagram_t>();
            for (auto* son : node.get_args())
            {
                args.push_back(self(self, *son));
            }
            switch (node.get_operation())
            {
            case Operation::And:
                return manager.left_fold<teddy::ops::AND>(args);
            case Operation::Or:
                return manager.left_fold<teddy::ops::OR>(args);
            default:
                unreachable();
                return nullptr;
            }
        }
    };
    return mk_node(mk_node, root);
}

auto unique_sp_count (
    MultiwayNode& root,
    teddy::mdd_manager<3>& manager,
    std::unordered_set<void*>& globalMemo
) -> int64
{
    auto memo = std::unordered_set<void*>();
    auto gen = SeriesParallelTreeGenerator2(root);
    while (not gen.is_done())
    {
        // auto diagram = make_diagram(manager, root, gen);
        auto diagram = make_diagram(manager, gen.get());
        memo.emplace(diagram.unsafe_get_root());
        globalMemo.emplace(diagram.unsafe_get_root());
        gen.advance();
    }
    return ssize(memo);
}

auto print_count_per_tree (int32 n)
{
    auto uniqueTable = MwUniqueTableType();
    auto manager = teddy::mdd_manager<3>(10, 1'000'000);
    auto cache = MwCacheType();
    auto memo = std::unordered_set<void*>();
    auto gen = SimpleMwAstGenerator(n, uniqueTable, cache);
    auto id = 0;
    auto sumDiv = int64(0);
    auto sumCombin = int64(0);
    auto sumCorrect = int64(0);

    std::cout << std::setw(7)  << "tree#"
              << std::setw(5)  << "div"
              << std::setw(8)  << "combin"
              << std::setw(19) << "unique(per-tree)"   << "\n";
    while (not gen.is_done())
    {
        auto* root = gen.get();
        auto countDiv = sp_system_count_3<int64>(*root);
        auto countCombin = sp_system_count_4<int64>(*root);
        auto countCorrect = unique_sp_count(*root, manager, memo);
        sumDiv += countDiv;
        sumCombin += countCombin;
        sumCorrect += countCorrect;
        std::cout << std::setw(7)  << id
                  << std::setw(5)  << countDiv
                  << std::setw(8)  << countCombin
                  << std::setw(19) << countCorrect << "\n";
        gen.advance();
        ++id;
    }
    std::cout << std::setw(7)  << "sum"
              << std::setw(5)  << sumDiv
              << std::setw(8)  << sumCombin
              << std::setw(19) << sumCorrect << "\n";
    std::cout << "unique total = " << ssize(memo) << "\n";
}

auto main () -> int
{
    std::cout << "n"          << "\t"
              << "gen"        << "\t"
              << "gen-unique" << "\t"
              << "div"        << "\n";
    for (auto n = 2; n < 10; ++n)
    {
        using diagram_t = teddy::mdd_manager<3>::diagram_t;
        auto uniqueTable = MwUniqueTableType();
        auto manager = teddy::mdd_manager<3>(10, 1'000'000);
        auto cache = MwCacheType();
        auto memo = std::unordered_set<diagram_t>();
        auto gen = SimpleMwAstGenerator(n, uniqueTable, cache);
        auto id = 0;
        while (not gen.is_done())
        {
            auto const& root = gen.get();
            auto spGen = SeriesParallelTreeGenerator2(*root);
            while (not spGen.is_done())
            {
                auto const diagram = make_diagram(manager, spGen.get());
                memo.emplace(std::move(diagram));
                spGen.advance();
                ++id;
            }
            gen.advance();
        }
        std::cout << n                           << "\t"
                  << id                          << "\t"
                  << ssize(memo)                 << "\t\t"
                  << sp_system_count_3<int64>(n) << "\n";

        for (auto const& [key, nodeptr] : uniqueTable)
        {
            delete nodeptr;
        }
    }

    // std::cout << "~~~~~~~~~~~~" << "\n";
    // print_count_per_tree(5);
    // std::cout << "~~~~~~~~~~~~" << "\n";



    // TODO vplyv unikatnosti vrcholov

    std::cout << "=== end of main ===" << '\n';
}
