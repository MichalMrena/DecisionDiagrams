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

MwAstGenerator::MwAstGenerator(
    int32 const varCount,
    std::unordered_map<
        MultiwayNode,
        MultiwayNode*,
        MwNodeHash,
        MwNodeEquals
    >& uniqueTable
) :
    sonVarCountsGenerator_(varCount),
    uniqueTable_(&uniqueTable),
    isDone_(false),
    isLeaf_(varCount == 1)
{
    if (not isLeaf_)
    {
        this->reset_son_generators();
    }
    this->make_tree();
}

auto MwAstGenerator::get () const -> MultiwayNode*
{
    assert(not this->is_done());
    return currentTree_;
}

auto MwAstGenerator::advance () -> void
{
    assert(not this->is_done());

    this->advance_state();
    if (not this->is_done())
    {
        this->make_tree();
    }
}

auto MwAstGenerator::is_done () const -> bool
{
    return isDone_;
}

auto MwAstGenerator::reset () -> void
{
    sonVarCountsGenerator_.reset();
    this->reset_son_generators();
    this->make_tree();
    isDone_ = false;
}

auto MwAstGenerator::advance_state () -> void
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

auto MwAstGenerator::make_tree () -> void
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
            sons.push_back(sonGenUPtr->get());
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

auto MwAstGenerator::reset_son_generators () -> void
{
    if (not isLeaf_)
    {
        sonGenerators_.clear();
        auto const& sonVarCounts = sonVarCountsGenerator_.get();
        for (auto const sonVarCount : sonVarCounts)
        {
            sonGenerators_.emplace_back(
                std::make_unique<MwAstGenerator>(sonVarCount, *uniqueTable_)
            );
        }
    }
}