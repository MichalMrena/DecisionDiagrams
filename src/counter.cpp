#include "counter.hpp"
#include "generators.hpp"
#include <numeric>

#define otherwise

using teddy::as_uindex;
using teddy::as_usize;

template<class Int>
tree_count_memo<Int>::tree_count_memo(int32 const n) :
    memo_(n + 1, -1)
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
        k == 1 || k == n ?
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

template class tree_count_memo<int64>;
template class tree_count_memo<Integer>;

template auto mw_tree_count (
    tree_count_memo<int64>& treeMemo,
    int32 const n
) -> int64;
template auto mw_tree_count (
    tree_count_memo<Integer>& treeMemo,
    int32 const n
) -> Integer;

template auto mw_tree_counts (int32 n) -> std::vector<int64>;
template auto mw_tree_counts (int32 n) -> std::vector<Integer>;