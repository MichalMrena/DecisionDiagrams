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
auto sp_system_count_div (MultiwayNode const& root) -> Int
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
                for (auto i = 0; i < count; ++i)
                {
                    nominator *= self(self, *sonptr);
                }
                denominator *= factorial<Int>(count);
            }
            return nominator / denominator;
        }
    });
    return 2 * go(root);
}

template<class Int>
auto sp_system_count_binom (MultiwayNode const& root) -> Int
{
    auto const componentCount = leaf_count(root);
    auto const go = [&](
        auto self,
        MultiwayNode const& node,
        int64 const leavesLeft,
        bool const breakSymetry
    ) -> Int
    {
        auto product = Int{1};
        auto const n = leavesLeft;
        auto const k = leaf_count(node);

        product *= n_over_k<Int>(
            n - static_cast<int32>(breakSymetry),
            k - static_cast<int32>(breakSymetry)
        );

        if (node.is_variable())
        {
            return product;
        }

        auto k1 = k;

        auto const sonGroups = group(node.get_args());
        for (auto const [son, count] : sonGroups)
        {
            if (count == 1)
            {
                product *= self(self, *son, k1, false);
                k1 -= leaf_count(*son);
            }
            else
            {
                product *= n_over_k<Int>(k1, count * leaf_count(*son));

                for (auto i = 0; i < count; ++i)
                {
                    product *= self(self,
                        *son,
                        (count - i) * leaf_count(*son),
                        true
                    );
                }
                k1 -= count * leaf_count(*son);
            }
        }
        return product;
    };
    return 2 * go(go, root, componentCount, false);
}

template<class Int>
auto sp_system_count_div (int32 componentCount) -> Int
{
    auto uniqueTable = MwUniqueTableType();
    auto cache = MwCacheType();
    auto gen = SimpleMwAstGenerator(componentCount, uniqueTable, cache);
    auto spCount = Int{0};
    while (not gen.is_done())
    {
        spCount += sp_system_count_div<Int>(*gen.get());
        gen.advance();
    }
    for (auto const& [key, nodeptr] : uniqueTable)
    {
        delete nodeptr;
    }
    return spCount;
}

template<class Int>
auto sp_system_count_binom (int32 componentCount) -> Int
{
    auto uniqueTable = MwUniqueTableType();
    auto cache = MwCacheType();
    auto gen = SimpleMwAstGenerator(componentCount, uniqueTable, cache);
    auto spCount = Int{0};
    while (not gen.is_done())
    {
        spCount += sp_system_count_binom<Int>(*gen.get());
        gen.advance();
    }
    for (auto const& [key, nodeptr] : uniqueTable)
    {
        delete nodeptr;
    }
    return spCount;
}

template class tree_count_memo<int64>;
template class tree_count_memo<Integer>;

template auto mw_tree_count (tree_count_memo<int64>&, int32) -> int64;
template auto mw_tree_count (tree_count_memo<Integer>&, int32) -> Integer;

template auto mw_tree_counts (int32) -> std::vector<int64>;
template auto mw_tree_counts (int32) -> std::vector<Integer>;

template auto sp_system_count_div(int32 n) -> int64;
template auto sp_system_count_div(int32 n) -> Integer;

template auto sp_system_count_binom(int32 n) -> int64;
template auto sp_system_count_binom(int32 n) -> Integer;

template auto sp_system_count_div(MultiwayNode const&) -> int64;
template auto sp_system_count_div(MultiwayNode const&) -> Integer;

template auto sp_system_count_binom(MultiwayNode const&) -> int64;
template auto sp_system_count_binom(MultiwayNode const&) -> Integer;