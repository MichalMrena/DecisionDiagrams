#include "generators.hpp"

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
        auto const newEnd = next(
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

auto SonVarCountsGenerator::find_first_non_one () -> std::vector<int32>::iterator
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

auto CombinationGenerator::get () const -> std::vector<MultiwayNode*> const&
{
    return current_;
}

auto CombinationGenerator::advance () -> void
{
    this->advance_state();
    this->fill_current();
}

auto CombinationGenerator::is_done () const -> bool
{
    return isDone_;
}

auto CombinationGenerator::reset () -> void
{
    isDone_ = empty(base_);
    std::ranges::fill(counter_, 0);
    std::ranges::fill(counterBase_, 0);
    this->fill_current();
}

auto CombinationGenerator::advance_state () -> void
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

auto CombinationGenerator::fill_current () -> void
{
    for (auto i = 0u; i < size(counter_); ++i)
    {
        current_[i] = base_[as_uindex(counter_[i])];
    }
}

SimpleMwAstGenerator::SimpleMwAstGenerator(
    int32 const varCount,
    MwUniqueTableType& uniqueTable,
    MwCacheType& cache
) :
    uniqueTable_(&uniqueTable),
    cache_(&cache),
    sonVarCountsGenerator_(varCount),
    currentTree_(nullptr),
    isDone_(varCount < 1),
    isLeaf_(varCount == 1)
{
    if (not isLeaf_)
    {
        this->reset_son_generators();
    }
    this->make_tree();
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
struct Group
{
    int32 elem_;
    int32 count_;
};

auto group (std::vector<int32> const& xs) -> std::vector<Group>
{
    auto groups = std::vector<Group>();
    auto it = begin(xs);
    auto const last = end(xs);
    while (it != last)
    {
        Group& group = groups.emplace_back(Group{*it, 0});
        while (it != last && *it == group.elem_)
        {
            ++it;
            ++group.count_;
        }
    }
    return groups;
}

auto make_all_trees (
    int32 const varCount,
    MwUniqueTableType& uniqueTable,
    MwCacheType& cache,
    bool const useCache
) -> std::vector<MultiwayNode*>
{
    auto trees = std::vector<MultiwayNode*>();
    if (useCache)
    {
        auto gen = CachedMwAstGenerator(varCount, uniqueTable, cache);
        while (not gen.is_done())
        {
            gen.get(trees);
            gen.advance();
        }
    }
    else
    {
        auto gen = SimpleMwAstGenerator(varCount, uniqueTable, cache);
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
        // sonGenerators_.clear();
        // auto const& sonVarCounts = sonVarCountsGenerator_.get();
        // for (auto const sonVarCount : sonVarCounts)
        // {
        //     sonGenerators_.emplace_back(
        //         std::make_unique<SimpleMwAstGenerator>(
        //             sonVarCount,
        //             *uniqueTable_
        //         )
        //     );
        // }

        sonGenerators_.clear();
        auto const countGroups = group(sonVarCountsGenerator_.get());
        for (auto const [varCount, treeCount] : countGroups)
        {
            if (treeCount == 1 || varCount < 3)
            {
                for (auto i = 0; i < treeCount; ++i)
                {
                    sonGenerators_.emplace_back(
                        // std::make_unique<CachedMwAstGenerator>(
                        std::make_unique<SimpleMwAstGenerator>(
                            varCount,
                            *uniqueTable_,
                            *cache_
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
                        *cache_
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
        std::ranges::sort(sons);

        key.data_ = NAryOpNode{
            Operation::Undefined,
            std::move(sons)
        };
    }

    auto it  = uniqueTable_->find(key);
    if (it == end(*uniqueTable_))
    {
        auto* newNode = new MultiwayNode{key};
        uniqueTable_->try_emplace(key, newNode);
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
    MwCacheType& cache
) :
    combination_(
        make_all_trees(varCount, uniqueTable, cache, true),
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
    MwCacheType& cache
)
{
    auto it = cache.find(varCount);
    if (it == end(cache))
    {
        auto [newIt, isIn] = cache.emplace(
            varCount,
            make_all_trees(varCount, uniqueTable, cache, false)
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