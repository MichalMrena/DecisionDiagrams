#include "counters.hpp"
#include "generators.hpp"
#include <numeric>
#include <tuple>

#define otherwise

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
auto factorial (Int n) -> Int
{
    auto result = Int(1);
    while (n > 1)
    {
        result *= n;
        --n;
    }
    return result;
}

template<class Int>
auto n_over_k (Int const n, Int const k) -> Int
{
    return
        k == 0 ?
            1 :
        k == 1 ?
            n :
        k > n / 2 ?
            n_over_k<Int>(n, n - k) :
        otherwise
            Int{n * n_over_k<Int>(n - 1, k - 1) / k};
}

template<class Int>
auto combin_r (Int const n, Int const k) -> Int
{
    return n_over_k<Int>(n + k - 1, k);
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
                counts.push_back(mw_tree_count(treeMemo, elem));
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
                    mw_tree_count<Int>(treeMemo,elem), count)
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

    MwUniqueTableType uniqueTable_;
    MwCacheType cache_;
    auto gen = SimpleMwAstGenerator(componentCount, uniqueTable_, cache_);
    auto spCount = Int{0};
    while (not gen.is_done())
    {
        auto binoms = std::vector<std::tuple<int32, int32>>();
        auto leftCount = componentCount;
        auto const& root = *gen.get();
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
            for (auto const& [son, count] : groups)
            {
                auto const n = leftCount;
                auto const k = count * leaf_son_count(*son);
                binoms.emplace_back(n, k);
            }
        });
        if (has_leaf_son(root))
        {
            auto const k = leaf_son_count(root);
            binoms.emplace_back(leftCount, k);
        }

        auto product = Int{1};
        for (auto const& [n, k] : binoms)
        {
            product *= n_over_k<Int>(n, k);
        }
        spCount += product;

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