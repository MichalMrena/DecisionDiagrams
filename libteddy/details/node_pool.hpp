#ifndef LIBTEDDY_DETAILS_NODE_POOL_HPP
#define LIBTEDDY_DETAILS_NODE_POOL_HPP

#include <libteddy/details/config.hpp>
#include <libteddy/details/debug.hpp>
#include <libteddy/details/node.hpp>
#include <libteddy/details/tools.hpp>

#include <cassert>
#include <new>
#include <vector>

namespace teddy
{
template<class Data, class Degree>
class node_pool
{
public:
    using node_t = node<Data, Degree>;

public:
    node_pool(int64 mainPoolSize, int64 overflowPoolSize);
    node_pool(node_pool&& other) noexcept;
    ~node_pool();

    node_pool(node_pool const&)       = delete;
    auto operator= (node_pool const&) = delete;
    auto operator= (node_pool&&)      = delete;

    [[nodiscard]] auto get_available_node_count () const -> int64;

    [[nodiscard]] auto get_main_pool_size () const -> int64;

    template<class... Args>
    [[nodiscard]] auto create (Args&&... args) -> node_t*;

    auto destroy (node_t* node) -> void;

    auto grow () -> void;

private:
    auto get_current_pool () const -> node_t*;

    [[nodiscard]] static auto allocate_pool (int64 size) -> node_t*;
    static auto deallocate_pool (node_t* poolPtr) -> void;

private:
    node_t* mainPool_;
    std::vector<node_t*> overflowPools_;
    node_t* freeNodeList_;
    int64 currentPoolIndex_;
    int64 nextPoolNodeIndex_;
    int64 mainPoolSize_;
    int64 overflowPoolSize_;
    int64 availableNodes_;
};

template<class Data, class Degree>
node_pool<Data, Degree>::node_pool(
    int64 const mainPoolSize,
    int64 const overflowPoolSize
) :
    mainPool_(allocate_pool(mainPoolSize)),
    overflowPools_(),
    freeNodeList_(nullptr),
    currentPoolIndex_(-1),
    nextPoolNodeIndex_(0),
    mainPoolSize_(mainPoolSize),
    overflowPoolSize_(overflowPoolSize),
    availableNodes_(mainPoolSize)
{
    #ifdef LIBTEDDY_VERBOSE
    debug::out(
        "node_pool: Allocating initial pool with size ",
        mainPoolSize_,
        ".\n"
    );
    #endif
}

template<class Data, class Degree>
node_pool<Data, Degree>::node_pool(node_pool&& other) noexcept :
    mainPool_(utils::exchange(other.mainPool_, nullptr)),
    overflowPools_(static_cast<std::vector<node_t*>&&>(other.overflowPools_)),
    freeNodeList_(utils::exchange(other.freeNodeList_, nullptr)),
    currentPoolIndex_(utils::exchange(other.currentPoolIndex_, -1)),
    nextPoolNodeIndex_(utils::exchange(other.nextPoolNodeIndex_, -1)),
    mainPoolSize_(utils::exchange(other.mainPoolSize_, -1)),
    overflowPoolSize_(utils::exchange(other.overflowPoolSize_, -1)),
    availableNodes_(utils::exchange(other.availableNodes_, -1))
{
}

template<class Data, class Degree>
node_pool<Data, Degree>::~node_pool()
{
    if (this->get_current_pool() != mainPool_)
    {
        // Destroy main pool
        for (int64 i = 0; i < mainPoolSize_; ++i)
        {
            (mainPool_ + i)->~node_t();
        }
        deallocate_pool(mainPool_);

        // Destroy other fully used pools
        for (int64 i = 0; i < currentPoolIndex_; ++i)
        {
            node_t* const pool = overflowPools_[as_uindex(i)];
            for (int64 k = 0; k < overflowPoolSize_; ++k)
            {
                (pool + k)->~node_t();
            }
            deallocate_pool(pool);
        }
    }

    // Destroy current partially used pool (main or overflow)
    node_t* const pool = this->get_current_pool();
    for (int64 k = 0; k < nextPoolNodeIndex_; ++k)
    {
        (pool + k)->~node_t();
    }
    deallocate_pool(pool);
}

template<class Data, class Degree>
auto node_pool<Data, Degree>::get_available_node_count() const -> int64
{
    return availableNodes_;
}

template<class Data, class Degree>
auto node_pool<Data, Degree>::get_main_pool_size() const -> int64
{
    return mainPoolSize_;
}

template<class Data, class Degree>
template<class... Args>
auto node_pool<Data, Degree>::create(Args&&... args) -> node_t*
{
    assert(availableNodes_ > 0);
    --availableNodes_;

    node_t* node = nullptr;
    if (freeNodeList_)
    {
        node          = freeNodeList_;
        freeNodeList_ = freeNodeList_->get_next();
        node->~node_t();
    }
    else
    {
        node = this->get_current_pool() + nextPoolNodeIndex_;
        ++nextPoolNodeIndex_;
    }

    return static_cast<node_t*>(::new (node) node_t (args...));
}

template<class Data, class Degree>
auto node_pool<Data, Degree>::destroy(node_t* const node) -> void
{
    ++availableNodes_;
    node->set_next(freeNodeList_);
    freeNodeList_ = node;
}

template<class Data, class Degree>
auto node_pool<Data, Degree>::grow() -> void
{
    #ifdef LIBTEDDY_VERBOSE
    debug::out(
        "node_pool: Allocating overflow pool with size ",
        overflowPoolSize_,
        ".\n"
    );
    #endif

    overflowPools_.push_back(allocate_pool(overflowPoolSize_));
    currentPoolIndex_  = ssize(overflowPools_) - 1;
    nextPoolNodeIndex_ = 0;
    availableNodes_ += overflowPoolSize_;
}

template<class Data, class Degree>
auto node_pool<Data, Degree>::get_current_pool() const -> node_t*
{
    return overflowPools_.empty()
             ? mainPool_
             : overflowPools_[as_uindex(currentPoolIndex_)];
}

template<class Data, class Degree>
auto node_pool<Data, Degree>::allocate_pool(int64 const size) -> node_t*
{
    return static_cast<node_t*>(
        ::operator new (as_usize(size) * sizeof(node_t))
    );
}

template<class Data, class Degree>
auto node_pool<Data, Degree>::deallocate_pool(node_t* const poolPtr) -> void
{
    ::operator delete (poolPtr);
}
} // namespace teddy

#endif