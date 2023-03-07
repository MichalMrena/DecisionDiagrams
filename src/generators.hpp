#ifndef TEDDY_SRC_GENERATORS_HPP
#define TEDDY_SRC_GENERATORS_HPP

#include <array>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

#include "libteddy/details/utils.hpp"
#include "trees.hpp"
#include "utils.hpp"

/**
 *  @brief Generates all unique binary ASTs with given number of variables
 *  using AND and OR operations.
 */
class BinAstGenerator
{
public:
    BinAstGenerator(int32 leafcount, int32 nextvar);

    auto get () const -> BinaryNode const&;

    auto advance () -> void;

    auto is_done () const -> bool;

private:
    auto advance_state () -> void;

    auto make_tree () -> void;

    auto reset_lhsgen () -> void;

    auto reset_rhsgen () -> void;

    auto reset_lhssizeit () -> void;

private:
    inline static auto Ops = {Operation::And, Operation::Or};

    int32 leafcount_;
    int32 nextvar_;
    std::vector<int32> lhssizes_; // TODO to iota range

    decltype(Ops)::iterator opsit_;
    std::vector<int32>::iterator lhssizesit_;

    std::unique_ptr<BinAstGenerator> lhsgen_;
    std::unique_ptr<BinAstGenerator> rhsgen_;

    std::unique_ptr<BinaryNode> node_;
};

/**
 *  @brief Generates all unqiue variants of son sizes of a root node (owner)
 *  of a tree with given number of variables.
 *
 *  In other words. Given a number of leaves n. This generator provides all ways
 *  how we can split the leaves among the sons.
 */
class SonVarCountsGenerator
{
public:
    SonVarCountsGenerator(int32 ownerVarCount);

    auto get () const -> std::vector<int32> const&;

    auto advance () -> void;

    auto is_done () const -> bool;

    auto reset () -> void;

private:
    auto find_first_non_one () -> std::vector<int32>::iterator;

    auto is_all_ones () const -> bool;

    auto owner_is_leaf () const -> bool;

private:
    std::vector<int32> counts_;
    int32 varCount_;
    bool isDone_;
};

/**
 *  @brief Generates all combinations from a base set.
 */
class CombinationGenerator
{
public:
    CombinationGenerator(std::vector<int32> base, int32 k);

    auto get () const -> std::vector<int32> const&;

    auto get_base () const -> std::vector<int32> const&;

    auto get_mask () const -> std::vector<int32> const&;

    auto advance () -> void;

    auto is_done () const -> bool;

    auto reset () -> void;

private:
    auto advance_state () -> void;

    auto fill_current () -> void;

private:
    std::vector<int32> base_;
    std::vector<int32> mask_;
    std::vector<int32> current_;
    bool isDone_;
};

/**
 *  @brief Generates all combinations with repretions from a base set.
 */
class CombinationRGenerator
{
public:
    CombinationRGenerator(std::vector<MultiwayNode*> base, int32 k);

    auto get () const -> std::vector<MultiwayNode*> const&;

    auto advance () -> void;

    auto is_done () const -> bool;

    auto reset () -> void;

private:
    auto advance_state () -> void;

    auto fill_current () -> void;

private:
    std::vector<MultiwayNode*> base_;
    std::vector<MultiwayNode*> current_;
    std::vector<int32> counter_;
    std::vector<int32> counterBase_;
    bool isDone_;
};

using MwUniqueTableType =
    std::unordered_map<
        MultiwayNode,
        MultiwayNode*,
        MwNodeHash,
    MwNodeEquals>;

using MwCacheType = std::unordered_map<int32, std::vector<MultiwayNode*>>;

/**
 *  @brief Interface for multiway AST generators.
 */
class MwAstGenerator
{
public:
    virtual ~MwAstGenerator() = default;

    virtual auto get (std::vector<MultiwayNode*>& out) const -> void = 0;

    virtual auto advance () -> void = 0;

    virtual auto is_done () const -> bool = 0;

    virtual auto reset () -> void = 0;
};

/**
 *  @brief Generates all multiway ASTs with given number of variables.
 *  Does not put any operation into internal nodes. This has to be done later,
 *  by altering AND and OR between levels of the tree.
 */
class SimpleMwAstGenerator : public MwAstGenerator
{
public:
    SimpleMwAstGenerator(
        int32 varCount,
        MwUniqueTableType& uniqueTable,
        MwCacheType& cache
    );

    auto get () const -> MultiwayNode*;

    auto get (std::vector<MultiwayNode*>& out) const -> void override;

    auto advance () -> void override;

    auto is_done () const -> bool override;

    auto reset () -> void override;

private:
    auto reset_son_generators () -> void;

    auto make_tree () -> void;

    auto advance_state () -> void;

private:
    MwUniqueTableType* uniqueTable_;
    MwCacheType* cache_;
    SonVarCountsGenerator sonVarCountsGenerator_;
    std::vector<std::unique_ptr<MwAstGenerator>> sonGenerators_;
    MultiwayNode* currentTree_;
    bool isDone_;
    bool isLeaf_;
};

/**
 *  @brief Uses simple generator to generate unique series of multiway ASTs
 *  using @c SimpleMwAstGenerator .
 */
class CombinatorialMwAstGenerator : public MwAstGenerator
{
public:
    CombinatorialMwAstGenerator(
        int32 varCount,
        int32 repetitionCount,
        MwUniqueTableType& uniqueTable,
        MwCacheType& cache
    );

    auto get (std::vector<MultiwayNode*>& out) const -> void override;

    auto is_done () const -> bool override;

    auto advance () -> void override;

    auto reset () -> void override;

private:
    CombinationRGenerator combination_;
};

/**
 *  @brief Cached version of @c SimpleMwAstGenerator .
 */
class CachedMwAstGenerator : public MwAstGenerator
{
public:
    CachedMwAstGenerator(
        int32 varCount,
        MwUniqueTableType& uniqueTable,
        MwCacheType& cache
    );

    auto get (std::vector<MultiwayNode*>& out) const -> void override;

    auto is_done () const -> bool override;

    auto advance () -> void override;

    auto reset () -> void override;

private:
    std::vector<MultiwayNode*> const* cached_;
    std::vector<MultiwayNode*>::const_iterator current_;
};

/**
 *  @brief Generates all series-parallel system using a topology given as tree.
 */
class SeriesParallelTreeGenerator
{
public:
    SeriesParallelTreeGenerator(MultiwayNode& root);

    auto get () const -> MultiwayNode const&;

    auto get_indices () const -> std::vector<int32> const&;

    auto get_leaf_group_sizes () const -> std::vector<int64> const&;

    auto get_op_at_level (int64 level) const -> Operation;

    auto is_done () const -> bool;

    auto advance () -> void;

private:
    auto advance_state () -> void;

    auto fill_indices () -> void;

    auto reset_tail_combinations (int64 headCount) -> void;

    static auto count_leaf_groups
        (MultiwayNode const& root) -> std::vector<int64>;

    static auto set_diff (
        std::vector<int32>& lhs,
        std::vector<int32> const& rhs
    ) -> void;

private:
    inline static constexpr auto Operations = std::array<Operation, 2>
    {
        Operation::And,
        Operation::Or
    };

private:
    MultiwayNode* root_;
    std::array<Operation, 2> operations_;
    std::vector<int64> leafGroupSizes_;
    std::vector<int32> indices_;
    std::vector<CombinationGenerator> combinations_;
    bool isDone_;
    bool operationsSwapped_;
};

/**
 *  @brief Generates all series-parallel system with given number of variables.
 */
class SeriesParallelGenerator
{
public:
    SeriesParallelGenerator(int32 varCount);

    ~SeriesParallelGenerator();

    auto get () const -> MultiwayNode const&;

    auto get_tree_gen () const -> SeriesParallelTreeGenerator const&;

    auto is_done () const -> bool;

    auto advance () -> void;

private:
    MwUniqueTableType uniqueTable_;
    MwCacheType cache_;
    SimpleMwAstGenerator treeGenerator_;
    SeriesParallelTreeGenerator fromTreeGenerator_;
};

#endif