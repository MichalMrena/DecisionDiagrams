#ifndef TEDDY_SRC_GENERATORS_HPP
#define TEDDY_SRC_GENERATORS_HPP

#include <array>
#include <memory>
#include <span>
#include <unordered_map>
#include <variant>
#include <vector>

#include "libteddy/details/types.hpp"
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
    CombinationGenerator(
        std::vector<int32> base,
        int32 k,
        int32 fixedCount = 0
    );

    auto get () const -> std::vector<int32> const&;

    auto get_base () const -> std::vector<int32> const&;

    auto get_mask () const -> std::vector<int32> const&;

    auto get_k () const -> int32;

    auto get_fixed_count () const -> int32;

    auto advance () -> void;

    auto is_done () const -> bool;

    auto reset () -> void;

    auto reset (std::vector<int32> base) -> void;

private:
    auto fill_current () -> void;

private:
    std::vector<int32> base_;
    std::vector<int32> mask_;
    std::vector<int32> current_;
    int32 fixedCount_;
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
 *  @brief Sequence
 */
class IDSource
{
public:
    auto get_current () const -> int64;
    auto advance () -> void;

private:
    int64 id_{0};
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
        MwCacheType& cache,
        IDSource& idSrc
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
    IDSource* idSrc_;
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
        MwCacheType& cache,
        IDSource& idSrc
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
        MwCacheType& cache,
        IDSource& idSrc
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
 *  @brief Describes leaf group.
*/
struct GroupDescription
{
    int64 size_;
    bool fixed_;
    GroupDescription(int64 const size, bool const fixed) :
        size_(size),
        fixed_(fixed)
    {
    }
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

    auto get_op_at_level (int64 level) const -> Operation;

    auto is_done () const -> bool;

    auto advance () -> void;

private:
    auto advance_state () -> void;

    auto fill_indices () -> void;

    auto reset_tail_combinations (int64 headCount) -> void;

    static auto make_leaf_groups
        (MultiwayNode const& root) -> std::vector<GroupDescription>;

private:
    inline static constexpr auto Operations = std::array<Operation, 2>
    {
        Operation::And,
        Operation::Or
    };

private:
    MultiwayNode* root_;
    std::array<Operation, 2> operations_;
    std::vector<GroupDescription> leafGroups_;
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
    IDSource idSrc_;
    SimpleMwAstGenerator treeGenerator_;
    SeriesParallelTreeGenerator fromTreeGenerator_;
};

/**
 * @brief Interface for SP index generators.
 */
class ISPIndexGenerator
{
public:
    virtual ~ISPIndexGenerator () = default;
    auto get () const -> std::vector<int32>;
    virtual auto get (std::vector<int32>& out) const -> void = 0;
    virtual auto advance () -> void = 0;
    virtual auto is_done () const -> bool = 0;
    virtual auto reset () -> void = 0;
    virtual auto reset (std::vector<int32> base) -> void = 0;
    // virtual auto make_diagram () -> diagram_t;
};

/**
 *  @brief TODO
 */
class SPGenerator : public ISPIndexGenerator
{
public:
    SPGenerator(
        std::vector<int32> base,
        std::vector<CombinationGenerator> sonBaseGens,
        std::vector<std::unique_ptr<ISPIndexGenerator>> groupGens
    );
    auto get (std::vector<int32>& out) const -> void override;
    auto advance () -> void override;
    auto is_done () const -> bool override;
    auto reset () -> void override;
    auto reset (std::vector<int32> base) -> void override;

private:
    auto reset_bases_tail (int64 headCount) -> void;
    auto reset_son_generators () -> void;

private:
    std::vector<int32> base_;
    std::vector<CombinationGenerator> sonBaseGens_;
    std::vector<std::unique_ptr<ISPIndexGenerator>> sonGens_;
    // GroupSPGenerator | SimpleSPGenerator | SPGenerator
    bool isDone_;
};

/**
 *  @brief TODO
 */
class GroupSPGenerator : public ISPIndexGenerator
{
public:
    GroupSPGenerator(
        std::vector<int32> base,
        std::vector<CombinationGenerator> combinGens,
        std::vector<SPGenerator> sonGens
    );
    auto get (std::vector<int32>& out) const -> void override;
    auto advance () -> void override;
    auto is_done () const -> bool override;
    auto reset () -> void override;
    auto reset (std::vector<int32> base) -> void override;

private:
    auto reset_bases_tail (int64 headCount) -> void;
    auto reset_son_generators () -> void;

private:
    std::vector<int32> base_;
    std::vector<CombinationGenerator> sonBaseGens_;
    std::vector<SPGenerator> sonGens_;
    bool isDone_;
};

/**
 *  @brief TODO
 */
class SimpleSPGenerator : public ISPIndexGenerator
{
public:
    SimpleSPGenerator(
        std::vector<int32> base,
        int32 k
    );
    auto get (std::vector<int32>& out) const -> void override;
    auto advance () -> void override;
    auto is_done () const -> bool override;
    auto reset () -> void override;
    auto reset (std::vector<int32> base) -> void override;

private:
    CombinationGenerator combination_;
};

/**
 *  @brief TODO
 */
auto make_spgen (MultiwayNode const& node) -> SPGenerator;

#endif