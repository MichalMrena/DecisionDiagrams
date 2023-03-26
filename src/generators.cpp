#include "generators.hpp"
#include "libteddy/details/types.hpp"
#include "libteddy/details/utils.hpp"
#include "src/utils.hpp"
#include "trees.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <memory>
#include <vector>

using teddy::as_uindex;
using teddy::as_usize;

BinAstGenerator::BinAstGenerator(int32 leafcount, int32 nextvar) :
    leafcount_ (leafcount),
    nextvar_   (nextvar),
    lhssizes_  (teddy::utils::fill_vector(
        leafcount / 2,
        [](auto const s){ return s + 1; }
    )),
    opsit_(begin(Ops)),
    lhssizesit_(begin(lhssizes_))
{
    this->reset_lhsgen();
    this->reset_rhsgen();
    this->make_tree();
}

auto BinAstGenerator::get () const -> BinaryNode const&
{
    return *node_;
}

auto BinAstGenerator::advance () -> void
{
    this->advance_state();
    this->make_tree();
}

auto BinAstGenerator::is_done () const -> bool
{
    return opsit_ == end(Ops);
}

auto BinAstGenerator::advance_state () -> void
{
    if (1 == leafcount_)
    {
        opsit_ = end(Ops);
    }
    else
    {
        auto resetrhsgen  = false;
        auto resetlhsgen  = false;
        auto resetlhssize = false;

        rhsgen_->advance();
        if (rhsgen_->is_done())
        {
            resetrhsgen = true;
            lhsgen_->advance();
            if (lhsgen_->is_done())
            {
                resetlhsgen = true;
                ++lhssizesit_;
                if (lhssizesit_ == end(lhssizes_))
                {
                    resetlhssize = true;
                    ++opsit_;
                }
            }
        }

        if (opsit_ == end(Ops))
        {
            return;
        }

        if (resetlhssize)
        {
            this->reset_lhssizeit();
        }

        if (resetlhsgen)
        {
            this->reset_lhsgen();
        }

        if (resetrhsgen)
        {
            this->reset_rhsgen();
        }
    }
}

auto BinAstGenerator::make_tree () -> void
{
    if (1 == leafcount_)
    {
        node_ = std::make_unique<BinaryNode>(
            BinaryNode {LeafNode {nextvar_}}
        );
    }
    else
    {
        auto const& lhs = lhsgen_->get();
        auto const& rhs = rhsgen_->get();
        node_ = std::make_unique<BinaryNode>(
            BinaryNode {BinOpNode {*opsit_, &lhs, &rhs}}
        );
    }
}

auto BinAstGenerator::reset_lhsgen () -> void
{
    if (leafcount_ > 1)
    {
        lhsgen_ = std::make_unique<BinAstGenerator>(
            *lhssizesit_,
            nextvar_
        );
    }
}

auto BinAstGenerator::reset_rhsgen () -> void
{
    if (leafcount_ > 1)
    {
        rhsgen_ = std::make_unique<BinAstGenerator>(
            leafcount_ - *lhssizesit_,
            *lhssizesit_ + nextvar_
        );
    }
}

auto BinAstGenerator::reset_lhssizeit () -> void
{
    lhssizesit_ = begin(lhssizes_);
}

SonVarCountsGenerator::SonVarCountsGenerator(int32 const ownerVarCount) :
    counts_({}),
    varCount_(ownerVarCount),
    isDone_(this->owner_is_leaf())
{
    if (not this->owner_is_leaf())
    {
        // in the worst case, each son of the owner will be leaf
        // i.e. we will have ${varCount_} ones int the vector
        counts_.reserve(as_usize(varCount_));
        this->reset();
    }
}

auto SonVarCountsGenerator::get () const -> std::vector<int32> const&
{
    assert(not this->is_done());
    return counts_;
}

auto SonVarCountsGenerator::advance () -> void
{
    assert(not this->is_done());

    if (this->is_all_ones())
    {
        isDone_ = true;
    }
    else
    {
        auto const decPos = this->find_first_non_one();
        --(*decPos);
        auto const oneCount = static_cast<int32>(
            std::distance(decPos, end(counts_))
        );

        // so that we can set any element w/o need of push_backs
        counts_.resize(as_usize(varCount_));

        auto const nextNum = std::min(*decPos, oneCount);
        auto const nextNumRepeats = oneCount / nextNum;
        auto const tailNum = oneCount % nextNum;

        for (auto i = 0; i < nextNumRepeats; ++i)
        {
            *(decPos + i + 1) = nextNum;
        }
        if (tailNum != 0)
        {
            *(decPos + nextNumRepeats + 1) = tailNum;
        }

        // set the corrent size
        auto const newEnd = std::next(
            decPos,
            nextNumRepeats + (tailNum != 0 ? 1 : 0) + 1
        );

        counts_.resize(as_usize(std::distance(begin(counts_), newEnd)));
    }
}

auto SonVarCountsGenerator::is_done () const -> bool
{
    return isDone_;
}

auto SonVarCountsGenerator::reset () -> void
{
    // do nothing if this one belongs to a leaf generator
    if (not this->owner_is_leaf())
    {
        counts_ = {varCount_ - 1, 1};
        isDone_ = false;
    }
}

auto SonVarCountsGenerator::find_first_non_one
    () -> std::vector<int32>::iterator
{
    for (auto i = ssize(counts_) - 1; i >= 0; --i)
    {
        if (counts_[as_usize(i)] != 1)
        {
            return begin(counts_) + i;
        }
    }
    unreachable();
    return end(counts_);
}

auto SonVarCountsGenerator::is_all_ones () const -> bool
{
    return counts_[0] == 1;
}

auto SonVarCountsGenerator::owner_is_leaf () const -> bool
{
    return varCount_ == 1;
}

CombinationGenerator::CombinationGenerator(
    std::vector<int32> base,
    int32 const k,
    int32 const fixedCount
) :
    base_(std::move(base)),
    mask_({}),
    current_(as_usize(k)),
    fixedCount_(fixedCount),
    isDone_(false)
{
    this->reset();
}

auto CombinationGenerator::get () const -> std::vector<int32> const&
{
    return current_;
}

auto CombinationGenerator::get_base () const -> std::vector<int32> const&
{
    return base_;
}

auto CombinationGenerator::get_mask () const -> std::vector<int32> const&
{
    return mask_;
}

auto CombinationGenerator::get_k () const -> int32
{
    return static_cast<int32>(std::ssize(current_));
}

auto CombinationGenerator::get_fixed_count () const -> int32
{
    return fixedCount_;
}

auto CombinationGenerator::get_nonfixed () const -> std::span<int32 const>
{
    return std::span<int32 const>(
        current_.data() + fixedCount_,
        as_usize(std::ssize(current_) - fixedCount_)
    );
}

auto CombinationGenerator::advance () -> void
{
    this->advance_state();
    if (not this->is_done())
    {
        this->fill_current();
    }
}

auto CombinationGenerator::is_done () const -> bool
{
    return isDone_;
}

auto CombinationGenerator::reset () -> void
{
    auto const n = size(base_);
    auto const k = size(current_);
    mask_.resize(k, 1);
    mask_.resize(n, 0);
    this->fill_current();
    isDone_ = false;
}

auto CombinationGenerator::reset (std::vector<int32> base) -> void
{
    base_ = std::move(base);
    auto const n = size(base_);
    auto const k = size(current_);
    mask_.resize(k, 1);
    mask_.resize(n, 0);
    this->fill_current();
    isDone_ = false;
}

auto CombinationGenerator::reset_nonfixed (std::vector<int32> base) -> void
{
    // TODO verify
    std::copy(begin(base), end(base), begin(base_) + fixedCount_);
    auto const n = size(base_);
    auto const k = size(current_);
    mask_.resize(k, 1);
    mask_.resize(n, 0);
    this->fill_current();
    isDone_ = false;
}

auto CombinationGenerator::advance_state () -> void
{
    isDone_ = not std::prev_permutation(begin(mask_) + fixedCount_, end(mask_));
}

auto CombinationGenerator::fill_current () -> void
{
    auto out = begin(current_);
    for (auto i = 0; i < ssize(mask_); ++i)
    {
        if (mask_[as_uindex(i)])
        {
            *out = base_[as_uindex(i)];
            ++out;
        }
    }
}

CombinationRGenerator::CombinationRGenerator(
    std::vector<MultiwayNode*> base,
    int32 const k
) :
    base_(std::move(base)),
    current_(as_usize(k)),
    counter_(as_usize(k)),
    counterBase_(as_usize(k)),
    isDone_(empty(base) || k == 0)
{
    this->fill_current();
}

auto CombinationRGenerator::get () const -> std::vector<MultiwayNode*> const&
{
    return current_;
}

auto CombinationRGenerator::advance () -> void
{
    this->advance_state();
    if (not this->is_done())
    {
        this->fill_current();
    }
}

auto CombinationRGenerator::is_done () const -> bool
{
    return isDone_;
}

auto CombinationRGenerator::reset () -> void
{
    isDone_ = empty(base_);
    std::ranges::fill(counter_, 0);
    std::ranges::fill(counterBase_, 0);
    this->fill_current();
}

auto CombinationRGenerator::advance_state () -> void
{
    auto const n = static_cast<int32>(ssize(base_));
    auto const k = static_cast<int32>(ssize(counter_));

    auto overflow = false;
    for (auto i = 0; i < k; ++i)
    {
        auto& c = counter_[as_uindex(i)];
        ++c;
        overflow = c == n;
        if (overflow)
        {
            auto& base = counterBase_[as_uindex(i)];
            ++base;
            c = base;
            for (auto j = i - 1; j >= 0; --j)
            {
                counterBase_[as_uindex(j)] = base;
                counter_[as_uindex(j)] = counterBase_[as_uindex(j)];
            }
        }
        else
        {
            break;
        }
    }
    isDone_ = overflow;
}

auto CombinationRGenerator::fill_current () -> void
{
    for (auto i = 0u; i < size(counter_); ++i)
    {
        current_[i] = base_[as_uindex(counter_[i])];
    }
}

auto IDSource::get_current () const -> int64
{
    return id_;
}

auto IDSource::advance () -> void
{
    ++id_;
}

SimpleMwAstGenerator::SimpleMwAstGenerator(
    int32 const varCount,
    MwUniqueTableType& uniqueTable,
    MwCacheType& cache,
    IDSource& idSrc
) :
    uniqueTable_(&uniqueTable),
    cache_(&cache),
    sonVarCountsGenerator_(varCount),
    currentTree_(nullptr),
    idSrc_(&idSrc),
    isDone_(varCount < 1),
    isLeaf_(varCount == 1)
{
    if (not isLeaf_)
    {
        this->reset_son_generators();
    }
    this->make_tree();
}

auto SimpleMwAstGenerator::get () const -> MultiwayNode*
{
    return currentTree_;
}

auto SimpleMwAstGenerator::get (std::vector<MultiwayNode*>& out) const -> void
{
    assert(not this->is_done());
    out.push_back(currentTree_);
}

auto SimpleMwAstGenerator::advance () -> void
{
    assert(not this->is_done());
    this->advance_state();
    if (not this->is_done())
    {
        this->make_tree();
    }
}

auto SimpleMwAstGenerator::is_done () const -> bool
{
    return isDone_;
}

auto SimpleMwAstGenerator::reset () -> void
{
    sonVarCountsGenerator_.reset();
    this->reset_son_generators();
    this->make_tree();
    isDone_ = false;
}

namespace
{
auto make_all_trees (
    int32 const varCount,
    MwUniqueTableType& uniqueTable,
    MwCacheType& cache,
    IDSource& idSrc,
    bool const useCache
) -> std::vector<MultiwayNode*>
{
    auto trees = std::vector<MultiwayNode*>();
    // To future me:
    // I know, if branches contain the same code. Variable gen should be of
    // type MwAstGenerator. However, to use it with unique_ptr, raw ptr,
    // or variant and initialize it with ternary expression is so
    // verbose that, the following piece of code is actually the most clean
    // and elegant solution.

    if (useCache)
    {
        auto gen = CachedMwAstGenerator(varCount, uniqueTable, cache, idSrc);
        while (not gen.is_done())
        {
            gen.get(trees);
            gen.advance();
        }
    }
    else
    {
        auto gen = SimpleMwAstGenerator(varCount, uniqueTable, cache, idSrc);
        while (not gen.is_done())
        {
            gen.get(trees);
            gen.advance();
        }
    }
    return trees;
}
}

auto SimpleMwAstGenerator::reset_son_generators () -> void
{
    if (not isLeaf_)
    {
        sonGenerators_.clear();
        auto const counts = sonVarCountsGenerator_.get();
        auto const countGroups = group(counts);
        for (auto const [varCount, treeCount] : countGroups)
        {
            if (treeCount == 1 || varCount < 3)
            {
                for (auto i = 0; i < treeCount; ++i)
                {
                    sonGenerators_.emplace_back(
                        std::make_unique<CachedMwAstGenerator>(
                            varCount,
                            *uniqueTable_,
                            *cache_,
                            *idSrc_
                        )
                    );
                }
            }
            else
            {
                sonGenerators_.emplace_back(
                    std::make_unique<CombinatorialMwAstGenerator>(
                        varCount,
                        treeCount,
                        *uniqueTable_,
                        *cache_,
                        *idSrc_
                    )
                );
            }
        }
    }
}

auto SimpleMwAstGenerator::make_tree () -> void
{
    auto key = MultiwayNode{};

    if (isLeaf_)
    {
        key.data_ = LeafNode{0};
    }
    else
    {
        auto sons = std::vector<MultiwayNode*>();
        sons.reserve(size(sonGenerators_));
        for (auto const& sonGenUPtr : sonGenerators_)
        {
            sonGenUPtr->get(sons);
        }
        std::ranges::sort(sons, [](MultiwayNode* l, MultiwayNode* r)
        {
            return l->id_ < r->id_;
        });

        key.data_ = NAryOpNode{
            Operation::Undefined,
            std::move(sons)
        };
    }

    auto it  = uniqueTable_->find(key);
    if (it == end(*uniqueTable_))
    {
        auto* newNode = new MultiwayNode{key};
        newNode->id_ = idSrc_->get_current();
        idSrc_->advance();
        uniqueTable_->try_emplace(std::move(key), newNode);
        currentTree_ = newNode;
    }
    else
    {
        currentTree_ = it->second;
    }
}

auto SimpleMwAstGenerator::advance_state () -> void
{
    auto overflow = false;
    for (auto& sonGenerator : sonGenerators_)
    {
        sonGenerator->advance();
        overflow = sonGenerator->is_done();
        if (overflow)
        {
            sonGenerator->reset();
        }
        else
        {
            break;
        }
    }

    if (isLeaf_)
    {
        isDone_ = true;
    }
    else if (overflow)
    {
        sonVarCountsGenerator_.advance();
        if (sonVarCountsGenerator_.is_done())
        {
            isDone_ = true;
        }
        else
        {
            this->reset_son_generators();
        }
    }
}

CombinatorialMwAstGenerator::CombinatorialMwAstGenerator(
    int32 const varCount,
    int32 const repetitionCount,
    MwUniqueTableType& uniqueTable,
    MwCacheType& cache,
    IDSource& idSrc
) :
    combination_(
        make_all_trees(varCount, uniqueTable, cache, idSrc, true),
        repetitionCount
    )
{
}

auto CombinatorialMwAstGenerator::get
    (std::vector<MultiwayNode*>& out) const -> void
{
    auto const& current = combination_.get();
    out.insert(end(out), begin(current), end(current));
}

auto CombinatorialMwAstGenerator::is_done() const -> bool
{
    return combination_.is_done();
}

auto CombinatorialMwAstGenerator::advance() -> void
{
    combination_.advance();
}

auto CombinatorialMwAstGenerator::reset () -> void
{
    combination_.reset();
}

CachedMwAstGenerator::CachedMwAstGenerator(
    int32 varCount,
    MwUniqueTableType& uniqueTable,
    MwCacheType& cache,
    IDSource& idSrc
)
{
    auto it = cache.find(varCount);
    if (it == end(cache))
    {
        auto [newIt, isIn] = cache.emplace(
            varCount,
            make_all_trees(varCount, uniqueTable, cache, idSrc, false)
        );
        it = newIt;
    }

    cached_ = &(it->second);
    current_ = begin(*cached_);
}

auto CachedMwAstGenerator::get (std::vector<MultiwayNode*>& out) const -> void
{
    out.push_back(*current_);
}

auto CachedMwAstGenerator::is_done () const -> bool
{
    return current_ == end(*cached_);
}

auto CachedMwAstGenerator::advance () -> void
{
    ++current_;
}

auto CachedMwAstGenerator::reset () -> void
{
    current_ = begin(*cached_);
}

SeriesParallelTreeGenerator::SeriesParallelTreeGenerator
    (MultiwayNode& root) :
    root_(&root),
    operations_({Operation::And, Operation::Or}),
    leafGroups_(make_leaf_groups(root)),
    indices_(as_usize(leaf_count(root))),
    combinations_({}),
    isDone_(false),
    operationsSwapped_(false)
{
    this->reset_tail_combinations(0);
    this->fill_indices();
}

auto SeriesParallelTreeGenerator::get () const -> MultiwayNode const&
{
    return *root_;
}

auto SeriesParallelTreeGenerator::get_indices
    () const -> std::vector<int32> const&
{
    return indices_;
}

auto SeriesParallelTreeGenerator::get_op_at_level
    (int64 const level) const -> Operation
{
    return operations_[as_uindex(level & 1)];
}

auto SeriesParallelTreeGenerator::is_done () const -> bool
{
    return isDone_;
}

auto SeriesParallelTreeGenerator::advance () -> void
{
    this->advance_state();
    if (not this->is_done())
    {
        this->fill_indices();
    }
}

auto SeriesParallelTreeGenerator::advance_state () -> void
{
    auto it = rbegin(combinations_);
    auto endIt = rend(combinations_);
    while (it != endIt)
    {
        it->advance();
        if (it->is_done())
        {
            ++it;
        }
        else
        {
            this->reset_tail_combinations(
                ssize(combinations_) - std::distance(rbegin(combinations_), it)
            );
            break;
        }
    }

    if (it == endIt)
    {
        std::swap(operations_[0], operations_[1]);
        isDone_ = operationsSwapped_;
        if (not isDone_)
        {
            this->reset_tail_combinations(0);
        }
        operationsSwapped_ = true;
    }
}

auto SeriesParallelTreeGenerator::fill_indices () -> void
{
    auto iOut = begin(indices_);
    for (auto const& gen : combinations_)
    {
        for (auto const i : gen.get())
        {
            *iOut = i;
            ++iOut;
        }
    }
}

auto SeriesParallelTreeGenerator::reset_tail_combinations
    (int64 const headCount) -> void
{
    auto base = teddy::utils::fill_vector(
        leaf_count(*root_),
        teddy::utils::identity
    );

    for (auto i = 0; i < headCount; ++i)
    {
        set_diff(base, combinations_[as_uindex(i)].get());
    }

    combinations_.erase(begin(combinations_) + headCount, end(combinations_));
    for (auto i = headCount; i < ssize(leafGroups_); ++i)
    {
        auto const [groupSize, doFix] = leafGroups_[as_uindex(i)];
        auto& gen = combinations_.emplace_back(
            base,
            groupSize,
            static_cast<int32>(doFix)
        );
        set_diff(base, gen.get());
    }
}

auto SeriesParallelTreeGenerator::make_leaf_groups
    (MultiwayNode const& root) -> std::vector<GroupDescription>
{
    auto toFix = int64{0};
    auto prevGroupId = int64{-1};
    auto groups = std::vector<GroupDescription>();

    auto const go = [&](
        auto self,
        MultiwayNode const& node,
        int64 const parentId,
        int64 const thisId
    ) -> void
    {
        if (node.is_variable())
        {
            if (parentId != prevGroupId)
            {
                groups.emplace_back(0, toFix > 0);
                prevGroupId = parentId;
                toFix -= static_cast<int64>(toFix > 0);
            }
            ++groups.back().size_;
        }
        else
        {
            auto const sonGroups = group(node.get_args());
            auto sonId = thisId + 1;
            for (auto const [son, count] : sonGroups)
            {
                if (count > 1 && not son->is_variable())
                {
                    toFix += count;
                }

                for (auto i = 0; i < count; ++i)
                {
                    self(self, *son, thisId, sonId);
                    ++sonId;
                }
            }
        }
    };
    go(go, root, -1, 0);

    return groups;
}

SeriesParallelGenerator::SeriesParallelGenerator (int32 const varCount) :
    uniqueTable_({}),
    cache_({}),
    idSrc_({}),
    treeGenerator_(varCount, uniqueTable_, cache_, idSrc_),
    fromTreeGenerator_(*treeGenerator_.get())
{
}

SeriesParallelGenerator::~SeriesParallelGenerator()
{
    for (auto const& [key, nodeptr] : uniqueTable_)
    {
        delete nodeptr;
    }
}

auto SeriesParallelGenerator::get () const -> MultiwayNode const&
{
    return fromTreeGenerator_.get();
}

auto SeriesParallelGenerator::get_tree_gen
    () const -> SeriesParallelTreeGenerator const&
{
    return fromTreeGenerator_;
}

auto SeriesParallelGenerator::is_done () const -> bool
{
    return treeGenerator_.is_done();
}

auto SeriesParallelGenerator::advance () -> void
{
    fromTreeGenerator_.advance();
    if (fromTreeGenerator_.is_done())
    {
        treeGenerator_.advance();
        if (not treeGenerator_.is_done())
        {
            fromTreeGenerator_ = SeriesParallelTreeGenerator(
                *treeGenerator_.get()
            );
        }
    }
}

SPGenerator::SPGenerator(
    std::vector<int32> base,
    std::vector<CombinationGenerator> groupCombinGens,
    std::vector<std::unique_ptr<ISPIndexGenerator>> groupGens
) :
    base_(std::move(base)),
    groupCombinGens_(std::move(groupCombinGens)),
    groupGens_(std::move(groupGens)),
    isDone_(false)
{
}

auto SPGenerator::get (std::vector<int32>& out) const -> void
{
    for (auto const& groupGen : groupGens_)
    {
        groupGen.get()->get(out);
    }
}

auto SPGenerator::get () const -> std::vector<int32>
{
    auto result = std::vector<int32>();
    this->get(result);
    return result;
}

auto SPGenerator::advance () -> void
{
    // Advance group generators
    auto groupGenOverflow = false;
    for (auto& groupGen : groupGens_)
    {
        groupGen->advance();
        groupGenOverflow = groupGen->is_done();
        if (groupGenOverflow)
        {
            groupGen->reset();
        }
        else
        {
            break;
        }
    }

    if (not groupGenOverflow)
    {
        return;
    }

    // Advance group combination gens
    auto groupCombinGenOverflow = false;
    for (auto i = ssize(groupCombinGens_) - 1; i >= 0 ; --i)
    {
        auto& groupCombinGen = groupCombinGens_[as_uindex(i)];
        groupCombinGen.advance();
        groupCombinGenOverflow = groupCombinGen.is_done();
        if (not groupCombinGenOverflow)
        {
            this->reset_tail_combinations(i);
            break;
        }
    }

    isDone_ = groupCombinGenOverflow;

    if (not this->is_done())
    {
        this->set_group_generators();
    }
}

auto SPGenerator::is_done () const -> bool
{
    return isDone_;
}

auto SPGenerator::reset () -> void
{
    this->reset_tail_combinations(-1);
    this->set_group_generators();
    isDone_ = false;
}

auto SPGenerator::reset (std::vector<int32> base) -> void
{
    base_ = std::move(base);
    this->reset_tail_combinations(-1);
    this->set_group_generators();
    isDone_ = false;
}

auto SPGenerator::reset_nonfixed (std::vector<int32>) -> void
{
    // TODO
    unreachable();
}

auto SPGenerator::reset_tail_combinations (int64 headCount) -> void
{
    auto base = base_;
    for (auto i = 0; i <= headCount; ++i)
    {
        auto& groupCombin = groupCombinGens_[as_uindex(i)];
        set_diff(base, groupCombin.get());
    }

    for (auto i = headCount + 1; i < ssize(groupCombinGens_); ++i)
    {
        auto& groupCombin = groupCombinGens_[as_uindex(i)];
        groupCombin.reset(base);
        set_diff(base, groupCombin.get());
    }
}

auto SPGenerator::set_group_generators () -> void
{
    for (auto i = 0; i < ssize(groupGens_); ++i)
    {
        groupGens_[as_uindex(i)]->reset(
            groupCombinGens_[as_uindex(i)].get()
        );
    }
}

GroupSPGenerator::GroupSPGenerator(
    std::vector<int32> base,
    std::vector<SPGenerator> sonGens
) :
    base_(std::move(base)),
    sonGens_(std::move(sonGens)),
    isDone_(false)
{
}

auto GroupSPGenerator::get (std::vector<int32>& out) const -> void
{
    out.insert(end(out), begin(base_), end(base_));
}

auto GroupSPGenerator::advance () -> void
{
    auto genOverflow = false;
    auto i = ssize(sonGens_) - 1;
    for (; i >= 0; --i)
    {
        auto& gen = sonGens_[as_uindex(i)];
        gen.advance();
        genOverflow = gen.is_done();
        if (not genOverflow)
        {
            break;
        }
    }

    isDone_ = genOverflow;

    if (not isDone_)
    {
        this->reset_tail_generators(i);
    }
}

auto GroupSPGenerator::is_done () const -> bool
{
    return isDone_;
}

auto GroupSPGenerator::reset () -> void
{
    this->reset_tail_generators(0);
    isDone_ = false;
}

auto GroupSPGenerator::reset (std::vector<int32> base) -> void
{
    base_ = std::move(base);
    this->reset_tail_generators(-1);
    isDone_ = false;
}

auto GroupSPGenerator::reset_nonfixed (std::vector<int32>) -> void
{
    unreachable();
}

auto GroupSPGenerator::reset_tail_generators (int64 headCount) -> void
{
    auto base = base_;
    auto used = std::vector<int32>();
    for (auto j = 0; j <= headCount; ++j)
    {
        sonGens_[as_uindex(j)].get(used);
    }
    set_diff(base, used);

    for (auto j = headCount + 1; j < ssize(sonGens_); ++j)
    {
        auto& gen = sonGens_[as_uindex(j)];
        gen.reset(base);
        set_diff(base, gen.get());
    }
    assert(base.empty()); // !!!
}

SimpleSPGenerator::SimpleSPGenerator(
    std::vector<int32> base,
    int32 const k,
    bool const fix
) :
    combination_(std::move(base), k, static_cast<int32>(fix))
{
}

auto SimpleSPGenerator::get () const -> std::vector<int32> const&
{
    return combination_.get();
}

auto SimpleSPGenerator::get (std::vector<int32>& out) const -> void
{
    out.insert(end(out), begin(combination_.get()), end(combination_.get()));
}

auto SimpleSPGenerator::advance () -> void
{
    combination_.advance();
}

auto SimpleSPGenerator::is_done () const -> bool
{
    return combination_.is_done();
}

auto SimpleSPGenerator::reset () -> void
{
    combination_.reset();
}

auto SimpleSPGenerator::reset (std::vector<int32> base) -> void
{
    combination_.reset(std::move(base));
}

auto SimpleSPGenerator::reset_nonfixed (std::vector<int32> base) -> void
{
    combination_.reset_nonfixed(std::move(base));
}

namespace
{
auto mk_spgen (
    MultiwayNode const& node,
    std::vector<int32> base,
    bool const fix
) -> SPGenerator
{
    auto baseCopy = base;
    auto groupCombinGens = std::vector<CombinationGenerator>();
    auto groupGens = std::vector<std::unique_ptr<ISPIndexGenerator>>();

    auto const groups = group_by(node.get_args(), &MultiwayNode::id_);

    for (auto const [son, count] : groups)
    {
        auto const k = count * static_cast<int32>(leaf_count(*son));
        groupCombinGens.emplace_back(base, k);
        set_diff(base, groupCombinGens.back().get());
    }

    for (auto i = 0; i < ssize(groups); ++i)
    {
        auto const [son, count] = groups[as_uindex(i)];
        auto groupBase = groupCombinGens[as_uindex(i)].get();
        auto groupBaseCopy = groupBase;
        if (son->is_variable())
        {
            auto simpleGen = std::make_unique<SimpleSPGenerator>(
                std::move(groupBase),
                count,
                fix || count > 1
            );
            groupGens.emplace_back(std::move(simpleGen));
        }
        else
        {
            auto perGroupGens = std::vector<SPGenerator>();

            for (auto j = 0; j < count; ++j)
            {
                perGroupGens.emplace_back(
                    mk_spgen(*son, groupBase, fix || count > 1)
                );
                set_diff(groupBase, perGroupGens.back().get());
            }

            groupGens.emplace_back(
                std::make_unique<GroupSPGenerator>(
                    std::move(groupBaseCopy),
                    std::move(perGroupGens)
                )
            );
        }
    }

    return SPGenerator(
        std::move(baseCopy),
        std::move(groupCombinGens),
        std::move(groupGens)
    );
}
}

auto make_spgen (
    MultiwayNode const& root
) -> SPGenerator
{
    auto const n = leaf_count(root);
    auto base = teddy::utils::fill_vector(n, teddy::utils::identity);
    return mk_spgen(root, std::move(base), false);
}