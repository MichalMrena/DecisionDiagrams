#ifndef LIBTEDDY_DETAILS_NODE_POOL_HPP
#define LIBTEDDY_DETAILS_NODE_POOL_HPP

#include <libteddy/details/debug.hpp>
#include <libteddy/details/node.hpp>

#include <new>
#include <utility>
#include <vector>

namespace teddy
{
template<class Data, degree Degree>
class node_pool
{
public:
    using node_t = node<Data, Degree>;

public:
    node_pool(int64 mainPoolSize, int64 overflowPoolSize);
    node_pool(node_pool const&) = delete;
    node_pool(node_pool&& other) noexcept;
    ~node_pool();

    auto operator= (node_pool other) -> node_pool&;

    [[nodiscard]] auto get_available_node_count () const -> int64;

    [[nodiscard]] auto get_main_pool_size () const -> int64;

    template<class... Args>
    [[nodiscard]] auto create (Args&&... args) -> node_t*;

    auto destroy (node_t* node) -> void;

    auto grow () -> void;

private:
    auto get_current_pool () const -> node_t*;
    auto get_current_pool_end () const -> node_t*;

    auto swap (node_pool& other) -> void;

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

template<class Data, degree Degree>
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
    debug::out(
        "node_pool: Allocating initial pool with size ",
        mainPoolSize_,
        ".\n"
    );
}

template<class Data, degree Degree>
node_pool<Data, Degree>::node_pool(node_pool&& other) noexcept :
    mainPool_(std::exchange(other.mainPool_, nullptr)),
    overflowPools_(std::move(other.overflowPools_)),
    freeNodeList_(std::exchange(other.freeNodeList_, nullptr)),
    currentPoolIndex_(other.currentPoolIndex_),
    nextPoolNodeIndex_(other.nextPoolNodeIndex_),
    mainPoolSize_(other.mainPoolSize_),
    overflowPoolSize_(other.overflowPoolSize_),
    availableNodes_(other.availableNodes_)
{
}

template<class Data, degree Degree>
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
            auto const pool = overflowPools_[as_uindex(i)];
            for (int64 k = 0; k < overflowPoolSize_; ++k)
            {
                (pool + k)->~node_t();
            }
            deallocate_pool(pool);
        }
    }

    // Destroy current partially used pool (main or overflow)
    auto const pool = this->get_current_pool();
    for (int64 k = 0; k < nextPoolNodeIndex_; ++k)
    {
        (pool + k)->~node_t();
    }
    deallocate_pool(pool);
}

template<class Data, degree Degree>
auto node_pool<Data, Degree>::operator= (node_pool other) -> node_pool&
{
    this->swap(other);
    return *this;
}

template<class Data, degree Degree>
auto node_pool<Data, Degree>::get_available_node_count() const -> int64
{
    return availableNodes_;
}

template<class Data, degree Degree>
auto node_pool<Data, Degree>::get_main_pool_size() const -> int64
{
    return mainPoolSize_;
}

template<class Data, degree Degree>
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

    return static_cast<node_t*>(
        ::new (node) node_t(std::forward<Args>(args)...)
    );
}

template<class Data, degree Degree>
auto node_pool<Data, Degree>::destroy(node_t* const node) -> void
{
    ++availableNodes_;
    node->set_next(freeNodeList_);
    freeNodeList_ = node;
}

template<class Data, degree Degree>
auto node_pool<Data, Degree>::grow() -> void
{
    debug::out(
        "node_pool: Allocating overflow pool with size ",
        overflowPoolSize_,
        ".\n"
    );

    overflowPools_.push_back(allocate_pool(overflowPoolSize_));
    currentPoolIndex_  = ssize(overflowPools_) - 1;
    nextPoolNodeIndex_ = 0;
    availableNodes_ += overflowPoolSize_;
}

template<class Data, degree Degree>
auto node_pool<Data, Degree>::get_current_pool() const -> node_t*
{
    return overflowPools_.empty()
             ? mainPool_
             : overflowPools_[as_uindex(currentPoolIndex_)];
}

template<class Data, degree Degree>
auto node_pool<Data, Degree>::get_current_pool_end() const -> node_t*
{
    return overflowPools_.empty()
             ? mainPool_ + mainPoolSize_
             : overflowPools_[as_uindex(currentPoolIndex_)] + overflowPoolSize_;
}

template<class Data, degree Degree>
auto node_pool<Data, Degree>::swap(node_pool& other) -> void
{
    using std::swap;
    swap(mainPool_, other.mainPool_);
    swap(overflowPools_, other.overflowPools_);
    swap(currentPoolIndex_, other.currentPoolIndex_);
    swap(freeNodeList_, other.freeNodeList_);
    swap(nextPoolNodeIndex_, other.nextPoolNodeIndex_);
    swap(availableNodes_, other.availableNodes_);
    swap(mainPoolSize_, other.mainPoolSize_);
    swap(overflowPoolSize_, other.overflowPoolSize_);
}

template<class Data, degree Degree>
auto node_pool<Data, Degree>::allocate_pool(int64 const size) -> node_t*
{
    return static_cast<node_t*>(
        ::operator new (as_usize(size) * sizeof(node_t))
    );
}

template<class Data, degree Degree>
auto node_pool<Data, Degree>::deallocate_pool(node_t* const poolPtr) -> void
{
    ::operator delete (poolPtr);
}
} // namespace teddy

#endif