#include <libteddy/teddy.hpp>
#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <exception>
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
    SeriesParallelGenerator const& gen
)
{
    using diagram_t = teddy::mdd_manager<3>::diagram_t;

    auto indexIts = std::vector<std::vector<int32>::const_iterator>();

    auto const combinations = gen.get_tree_gen().get_combinations();
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

    auto dfs = teddy::utils::fix([&]
        (auto self, MultiwayNode const& node) -> diagram_t
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
    });
    return dfs(root);
}

auto print_count_per_tree (int32 n)
{
    auto id = 0;
    auto gen = SeriesParallelGenerator(n);
    std::cout << "id" << "\t" << "div" << "\t" << "combin" << "\n";
    while (not gen.is_done())
    {
        if (id >= 44 && id <= 55)
        {
            auto const& root = gen.get();
            std::cout << id                             << "\t"
                      << sp_system_count_3<int64>(root) << "\t"
                      << sp_system_count_4<int64>(root) << "\n";
            std::cout << dump_dot(root) << "\n" << "---" << "\n";
        }
        gen.advance();
        ++id;
    }
}

auto main () -> int
{
    //
    // Brute-force counting of mwtrees
    //
    // auto const expected = std::vector {
    //     -1,
    //     1, 1, 2, 5, 12, 33, 90, 261, 766, 2'312, 7'068,
    //     21'965, 68'954, 218'751, 699'534, 2'253'676, 7'305'788,
    //     23'816'743, 78'023'602, 256'738'751
    // };

    // std::cout << "n"         << "\t"
    //           << "unique"    << "\t"
    //           << "correct"   << "\t"
    //           << "total"     << "\t"
    //           << "unique nodes" << "\t"
    //           << "time[ms]"  << std::endl;

    // auto uniqueTable = MwUniqueTableType();
    // auto cache = MwCacheType();
    // auto rootStorage = std::vector<MultiwayNode*>();
    // for (auto varCount = 1; varCount < ssize(expected); ++varCount)
    // {
    //     namespace ch     = std::chrono;
    //     auto const start = ch::high_resolution_clock::now();
    //     auto gen         = SimpleMwAstGenerator(varCount, uniqueTable, cache);
    //     auto totalCount  = 0;
    //     auto uniqueCount = 0;
    //     while (not gen.is_done())
    //     {
    //         gen.get(rootStorage);
    //         ++uniqueCount;
    //         ++totalCount;
    //         gen.advance();
    //         rootStorage.clear();
    //     }
    //     auto const end = ch::high_resolution_clock::now();
    //     auto const duration = ch::duration_cast<ch::milliseconds>(end - start);
    //     std::cout << varCount    << "\t"
    //               << uniqueCount << "\t"
    //               << expected[as_uindex(varCount)] << "\t"
    //               << totalCount  << "\t"
    //               << size(uniqueTable) << "\t"
    //               << duration.count()  << std::endl;

    // }
    // for (auto const& [key, nodeptr] : uniqueTable)
    // {
    //     delete nodeptr;
    // }

    //
    // Semi-analytical counting of mwtrees.
    //
    // auto constexpr N = 100;
    // auto memo = tree_count_memo<Integer>(N);
    // for (auto i = 1; i <= N; ++i)
    // {
    //     std::cout << i << "\t" << mw_tree_count<Integer>(memo, i) << "\n";
    // }

    //
    // Combination generator test
    //
    // auto gen = CombinationGenerator({7}, 1);
    // while (not gen.is_done())
    // {
    //     for (auto const x : gen.get())
    //     {
    //         std::cout << x << " ";
    //     }
    //     std::cout << "\n";
    //     gen.advance();
    // }

    //
    // Series-parallel tree test
    //
    using diagram_t = teddy::mdd_manager<3>::diagram_t;
    using node_t = std::remove_pointer_t<
        decltype(diagram_t().unsafe_get_root())
    >;
    using pair_t = std::pair<node_t*, int32>;
    using pair_hash_t = decltype([](pair_t const& p)
    {
        return std::hash<node_t*>()(p.first);
    });
    using pair_eq_t = decltype([](pair_t const& l, pair_t const& r)
    {
        return l.first == r.first;
    });

    auto manager = teddy::mdd_manager<3>(10, 1'000'000);
    std::cout << "n"                    << "\t"
              << "gen"                  << "\t"
              << "naive"                << "\t"
              << "div"                  << "\t"
              << "combin"               << "\t"
              << "gen-unique-(correct)" << "\n";
    for (auto n = 2; n < 9; ++n)
    {
        // if (n != 5)
        // {
        //     continue;
        // }

        auto id = 0;
        auto gen = SeriesParallelGenerator(n);
        auto memo = std::unordered_set<
            pair_t,
            pair_hash_t,
            pair_eq_t
        >();
        while (not gen.is_done())
        {
            auto const& root = gen.get();
            auto diagram = make_diagram(manager, root, gen);

            // auto pair = std::make_pair(diagram.unsafe_get_root(), id);
            // std::cout << "# " << id;
            // auto memoized = memo.find(pair);
            // if (memoized != end(memo))
            // {
            //     std::cout << "\t" << "duplicate with " << memoized->second;
            // }
            // std::cout << "\n";
            // std::cout << dump_dot(root, gen) << "\n";

            memo.emplace(diagram.unsafe_get_root(), id);
            gen.advance();
            ++id;
        }
        std::cout << n << "\t"
                  << id << "\t"
                  << sp_system_count_2<int64>(n) << "\t"
                  << sp_system_count_3<int64>(n) << "\t"
                  << sp_system_count_4<int64>(n) << "\t"
                  << ssize(memo) << "\n";
    }

    // auto leaf = MultiwayNode{LeafNode{}};
    // auto node1 = MultiwayNode{
    //     NAryOpNode{
    //         Operation::Undefined,
    //         {&leaf, &leaf}
    //     }
    // };
    // auto node2 = MultiwayNode{
    //     NAryOpNode{
    //         Operation::Undefined,
    //         {&node1, &node1}
    //     }
    // };
    // auto root = MultiwayNode{
    //     NAryOpNode{
    //         Operation::Undefined,
    //         {&node2, &node2, &node2}
    //     }
    // };
    // std::cout << sp_system_count_3<int64>(node2) << "\n";
    // std::cout << sp_system_count_4<int64>(node2) << "\n";

    // print_count_per_tree(4);

    //
    // Semi-analytical counting of series-parallel systems
    //
    // for (auto n = 2; n < 20; ++n)
    // {
    //     std::cout << n << "\t" << sp_system_count<Integer>(n) << "\n";
    // }

    // vplyv unikatnosti vrcholov

    std::cout << "=== end of main ===" << '\n';
}
