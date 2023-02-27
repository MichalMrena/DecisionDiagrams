#ifndef TEDDY_SRC_COUNTER_HPP
#define TEDDY_SRC_COUNTER_HPP

#include "src/trees.hpp"
#include <gmpxx.h>
#include <libteddy/teddy.hpp>
#include <map>
#include <optional>
#include <utility>
#include <vector>

using teddy::int32;
using teddy::int64;
using Integer = mpz_class;

/**
 *  @brief Memo for tree counts.
 */
template<class Int>
class tree_count_memo
{
public:
    tree_count_memo(int32 n);
    auto put (int32 key, Int val) -> void;
    auto try_get (int32 key) -> std::optional<Int>;
    auto get_memo () const -> std::vector<Int>;

private:
    std::vector<Int> memo_;
};

/**
 *  @brief Calculates number of unique multiway trees with @p n leaves.
 */
template<class Int>
auto mw_tree_count (tree_count_memo<Int>& treeMemo, int32 n) -> Int;

/**
 *  @brief Calculates number of unique multiway trees with up to @p n leaves.
 */
template<class Int>
auto mw_tree_counts (int32 n) -> std::vector<Int>;

/**
 *  @brief Calculates the number of series-parallel systems with n components.
 */
template<class Int>
auto sp_system_count (int32 n) -> Int;

/**
 *  @brief Calculates the number of series-parallel systems with n components.
 */
template<class Int>
auto sp_system_count_2 (int32 n) -> Int;

/**
 *  @brief Calculates the number of series-parallel systems with n components.
 */
template<class Int>
auto sp_system_count_3 (int32 n) -> Int;

/**
 *  @brief Calculates the number of series-parallel systems with n components.
 */
template<class Int>
auto sp_system_count_4 (int32 n) -> Int;

/**
 *  @brief Calculates the number of series-parallel systems with given topology.
 */
template<class Int>
auto sp_system_count_3 (MultiwayNode const& root) -> Int;

/**
 *  @brief Calculates the number of series-parallel systems with given topology.
 */
template<class Int>
auto sp_system_count_4 (MultiwayNode const& root) -> Int;

#endif