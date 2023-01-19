#ifndef TEDDY_SRC_GENERATORS_HPP
#define TEDDY_SRC_GENERATORS_HPP

#include <memory>
#include <unordered_map>
#include <vector>

#include "trees.hpp"

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
 *  @brief Generates all unique multiway ASTs with given number of variables.
 *  Does not put any operation into internal nodes. This has to be done later,
 *  by altering AND and OR between levels of the tree.
 */
class MwAstGenerator
{
public:
    MwAstGenerator(
        int32 const varCount,
        std::unordered_map<
            MultiwayNode,
            MultiwayNode*,
            MwNodeHash,
            MwNodeEquals
        >& uniqueTable
    );

    auto get () const -> MultiwayNode*;

    auto advance () -> void;

    auto is_done () const -> bool;

    auto reset () -> void;

private:
    auto advance_state () -> void;

    auto make_tree () -> void;

    auto reset_son_generators () -> void;

public:
    SonVarCountsGenerator sonVarCountsGenerator_;
    std::vector<std::unique_ptr<MwAstGenerator>> sonGenerators_;
    std::unordered_map<
        MultiwayNode,
        MultiwayNode*,
        MwNodeHash,
    MwNodeEquals>* uniqueTable_;
    MultiwayNode* currentTree_;
    bool isDone_;
    bool isLeaf_;
};

#endif