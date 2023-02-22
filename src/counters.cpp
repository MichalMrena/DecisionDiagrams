#include "counters.hpp"
#include "generators.hpp"
#include "trees.hpp"
#include "utils.hpp"
#include <iostream>
#include <numeric>
#include <tuple>

using teddy::as_uindex;
using teddy::as_usize;

template<class Int>
tree_count_memo<Int>::tree_count_memo(int32 const n) :
    memo_(as_usize(n + 1), -1)
{
    memo_[1] = 1;
    memo_[2] = 1;
}

template<class Int>
auto tree_count_memo<Int>::put (int32 const key, Int val) -> void
{
    memo_[as_uindex(key)] = std::move(val);
}

template<class Int>
auto tree_count_memo<Int>::try_get (int32 const key) -> std::optional<Int>
{
    return memo_[as_uindex(key)] == -1
        ? std::nullopt
        : std::make_optional(memo_[as_uindex(key)]);
}

template<class Int>
auto tree_count_memo<Int>::get_memo () const -> std::vector<Int>
{
    return memo_;
}

template<class Int>
auto mw_tree_count (tree_count_memo<Int>& treeMemo, int32 const n) -> Int
{
    auto const memoized = treeMemo.try_get(n);
    if (memoized.has_value())
    {
        return *memoized;
    }

    auto value = Int();
    auto partitionGen = SonVarCountsGenerator(n);
    while (not partitionGen.is_done())
    {
        auto const groups = group(partitionGen.get());
        auto counts = std::vector<Int>();
        counts.reserve(as_usize(n));
        for (auto const [elem, count] : groups)
        {
            if (count == 1)
            {
                counts.push_back(mw_tree_count<Int>(treeMemo, elem));
            }
            else if (elem < 3)
            {
                counts.push_back(teddy::utils::int_pow(
                    mw_tree_count<Int>(treeMemo, elem),
                    count
                ));
            }
            else
            {
                counts.push_back(combin_r<Int>(
                    mw_tree_count<Int>(treeMemo, elem), count)
                );
            }
        }
        value += std::reduce(
            begin(counts),
            end(counts),
            Int(1),
            std::multiplies<>()
        );
        partitionGen.advance();
    }
    treeMemo.put(n, value);
    return value;
}

template<class Int>
auto mw_tree_counts (int32 const n) -> std::vector<Int>
{
    auto memo = tree_count_memo<Int>(n);
    mw_tree_count(memo, n);
    return memo.get_memo();
}

template<class Int>
auto sp_system_count (int32 componentCount) -> Int
{
    MwUniqueTableType uniqueTable_;
    MwCacheType cache_;
    auto gen = SimpleMwAstGenerator(componentCount, uniqueTable_, cache_);
    auto count = Int{0};
    while (not gen.is_done())
    {
        auto binoms = std::vector<std::pair<int32, int32>>();
        auto leftCount = componentCount;
        auto const& root = *gen.get();
        for_each_dfs(root, [&](MultiwayNode const& node, auto, auto)
        {
            if (has_leaf_son(node))
            {
                auto sonCount = 0;
                for (auto* son : node.get_args())
                {
                    sonCount += static_cast<int32>(son->is_variable());
                }
                binoms.emplace_back(leftCount, sonCount);
                leftCount -= sonCount;
            }
        });

        auto product = Int{1};
        for (auto const& [n, k] : binoms)
        {
            product *= n_over_k<Int>(n, k);
        }
        count += product;

        gen.advance();
    }
    return 2 * count;
}

template<class Int>
auto sp_system_count_2 (int32 componentCount) -> Int
{
    auto const has_has_leaf_son = [](MultiwayNode const& node)
    {
        if (node.is_operation())
        {
            for (auto* son : node.get_args())
            {
                if (has_leaf_son(*son))
                {
                    return true;
                }
            }
        }
        return false;
    };

    auto const leaf_son_count = [](MultiwayNode const& node)
    {
        auto count = 0;
        for (auto* son : node.get_args())
        {
            if (son->is_variable())
            {
                ++count;
            }
        }
        return count;
    };

    auto uniqueTable = MwUniqueTableType();
    auto cache = MwCacheType();
    auto gen = SimpleMwAstGenerator(componentCount, uniqueTable, cache);
    auto spCount = Int{0};
    while (not gen.is_done())
    {
        // auto binoms = std::vector<std::tuple<int32, int32>>();
        auto leftCount = componentCount;
        auto const& root = *gen.get();
            // std::cout << dump_dot(root) << "\n";
        auto product = Int{1};
        for_each_dfs(root, [&](MultiwayNode const& node, auto, auto)
        {
            if (not has_has_leaf_son(node))
            {
                return;
            }

            auto sons = std::vector<MultiwayNode*>();
            for (auto* son : node.get_args())
            {
                if (has_leaf_son(*son))
                {
                    sons.push_back(son);
                }
            }

            std::ranges::sort(sons);
            auto const groups = group(sons);
            for (auto const [sonptr, count] : groups)
            {
                auto localProduct = Int{1};
                for (auto i = 0; i < count; ++i)
                {
                    auto const n = leftCount;
                    auto const k = leaf_son_count(*sonptr);
                    localProduct *= n_over_k<Int>(n, k);
                    leftCount -= k;
                }
                localProduct /= factorial(count);
                product *= localProduct;
            }
        });
        if (has_leaf_son(root))
        {
            auto const n = leftCount;
            auto const k = leaf_son_count(root);
            product *= n_over_k<Int>(n, k);
        }

        spCount += 2 * product;

        gen.advance();
    }
    return spCount;
}

template<class Int>
auto sp_system_count_3 (MultiwayNode const& root) -> Int
{
    auto leftCount = leaf_count(root);
    auto const go = teddy::utils::fix([&]
        (auto self, MultiwayNode const& node) -> Int
    {
        if (node.is_variable())
        {
            auto const ret = leftCount;
            --leftCount;
            return ret;
        }
        else
        {
            auto nominator = Int{1};
            auto denominator = Int{1};
            auto sons = node.get_args();
            std::ranges::sort(sons);
            auto groups = group(sons);
            for (auto const [sonptr, count] : groups)
            {
                // TODO, delit to tu?
                for (auto i = 0; i < count; ++i)
                {
                    nominator *= self(self, *sonptr);
                }
                denominator *= factorial<Int>(count);
            }
            return nominator / denominator;
        }
    });
    return go(root);
}

template<class Int>
auto sp_system_count_4 (MultiwayNode const& root) -> Int
{
    auto const componentCount = leaf_count(root);
    auto const go = teddy::utils::fix([&]
        (auto self, MultiwayNode const& node, int64 leavesLeft) -> Int
    {
        if (node.is_variable())
        {
            return 1;
        }

        auto const sonGroups = group_by(node.get_args(),
            [](MultiwayNode* const n)
        {
            return leaf_count(*n);
        });

        auto product = Int{1};
        for (auto const [son, count] : sonGroups)
        {
            if (count == 1)
            {
                auto const n = leavesLeft;
                auto const k = leaf_count(*son);
                leavesLeft -= k;
                product *= n_over_k<Int>(n, k) * self(self, *son, k);
            }
            else if (son->is_variable())
            {
                auto const n = leavesLeft;
                auto const k = count;
                product *= n_over_k<Int>(n, k);
                leavesLeft -= k;
            }
            else
            {
                for (auto i = 0; i < count; ++i)
                {
                    auto const n = leavesLeft - 1;
                    auto const k = leaf_count(*son) - 1;
                    leavesLeft -= k + 1;
                    product *= n_over_k<Int>(n, k)
                             * self(self, *son, k + 1);
                }
            }
        }
        return product;
    });
    return go(root, componentCount);
}

template<class Int>
auto sp_system_count_3 (int32 componentCount) -> Int
{
    auto uniqueTable = MwUniqueTableType();
    auto cache = MwCacheType();
    auto gen = SimpleMwAstGenerator(componentCount, uniqueTable, cache);
    auto spCount = Int{0};
    while (not gen.is_done())
    {
        spCount += sp_system_count_3<Int>(*gen.get());
        gen.advance();
    }
    return 2 * spCount;
}

template<class Int>
auto sp_system_count_4 (int32 componentCount) -> Int
{
    auto uniqueTable = MwUniqueTableType();
    auto cache = MwCacheType();
    auto gen = SimpleMwAstGenerator(componentCount, uniqueTable, cache);
    auto spCount = Int{0};
    while (not gen.is_done())
    {
        spCount += sp_system_count_4<Int>(*gen.get());
        gen.advance();
    }
    return 2 * spCount;
}

template class tree_count_memo<int64>;
template class tree_count_memo<Integer>;

template auto mw_tree_count (tree_count_memo<int64>&, int32) -> int64;
template auto mw_tree_count (tree_count_memo<Integer>&, int32) -> Integer;

template auto mw_tree_counts (int32) -> std::vector<int64>;
template auto mw_tree_counts (int32) -> std::vector<Integer>;

template auto sp_system_count(int32 n) -> int64;
template auto sp_system_count(int32 n) -> Integer;

template auto sp_system_count_2(int32 n) -> int64;
template auto sp_system_count_2(int32 n) -> Integer;

template auto sp_system_count_3(int32 n) -> int64;
template auto sp_system_count_3(int32 n) -> Integer;

template auto sp_system_count_4(int32 n) -> int64;
template auto sp_system_count_4(int32 n) -> Integer;

template auto sp_system_count_3(MultiwayNode const&) -> int64;
template auto sp_system_count_3(MultiwayNode const&) -> Integer;

template auto sp_system_count_4(MultiwayNode const&) -> int64;
template auto sp_system_count_4(MultiwayNode const&) -> Integer;