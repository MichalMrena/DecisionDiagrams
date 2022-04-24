#ifndef MIX_DD_NODE_POOL_HPP
#define MIX_DD_NODE_POOL_HPP

#include "node.hpp"
#include "debug.hpp"

#include <array>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>

namespace teddy
{
    template<class Data, degree D>
    class node_pool
    {
    public:
        using node_t = node<Data, D>;
        using sons_t = typename node_t::sons_t;

    public:
        node_pool  (std::size_t mainPoolSize);
        node_pool  (std::size_t mainPoolSize, std::size_t overflowPoolSize);
        node_pool  (node_pool const&) = delete;
        node_pool  (node_pool&&);
        ~node_pool ();

        auto operator= (node_pool) -> node_pool&;

        auto available_node_count () const -> std::size_t;

        auto main_pool_size () const -> std::size_t;

        template<class... Args>
        [[nodiscard]]
        auto create (Args&&...) -> node_t*;

        auto destroy (node_t*) -> void;

        auto grow () -> void;

    private:
        auto current_pool () const -> node_t*;

        auto swap (node_pool&) -> void;

        static auto allocate_pool (std::size_t) -> node_t*;
        static auto deallocate_pool (node_t*) -> void;

    private:
        node_t*              mainPool_;
        std::vector<node_t*> overflowPools_;
        node_t*              freeNodeList_;
        std::size_t          currentPoolIndex_;
        std::size_t          nextPoolNodeIndex_;
        std::size_t          mainPoolSize_;
        std::size_t          overflowPoolSize_;
        std::size_t          availableNodes_;
    };

    template<class Data, degree D>
    node_pool<Data, D>::node_pool
        (std::size_t const mainPoolSize) :
        node_pool(mainPoolSize, mainPoolSize / 2)
    {
    }

    template<class Data, degree D>
    node_pool<Data, D>::node_pool
        (std::size_t const mainPoolSize, std::size_t const overflowPoolSize) :
        mainPool_          (allocate_pool(mainPoolSize)),
        overflowPools_     ({}),
        currentPoolIndex_  (std::numeric_limits<std::size_t>::max()),
        freeNodeList_      (nullptr),
        nextPoolNodeIndex_ (0),
        availableNodes_    (mainPoolSize),
        mainPoolSize_      (mainPoolSize),
        overflowPoolSize_  (overflowPoolSize)
    {
        debug::out( "node_pool: Allocating initial pool with size "
                  , mainPoolSize_, ".\n" );
    }

    template<class Data, degree D>
    node_pool<Data, D>::node_pool
        (node_pool&& other) :
        mainPool_          (std::exchange(other.mainPool_, nullptr)),
        overflowPools_     (std::move(other.overflowPools_)),
        currentPoolIndex_  (other.currentPoolIndex_),
        freeNodeList_      (std::exchange(other.freeNodeList_, nullptr)),
        nextPoolNodeIndex_ (other.nextPoolNodeIndex_),
        availableNodes_    (other.availableNodes_),
        mainPoolSize_      (other.mainPoolSize_),
        overflowPoolSize_  (other.overflowPoolSize_)
    {
    }

    template<class Data, degree D>
    node_pool<Data, D>::~node_pool
        ()
    {
        if (this->current_pool() != mainPool_)
        {
            // Destroy main pool.
            for (auto i = 0ull; i < mainPoolSize_; ++i)
            {
                std::destroy_at(mainPool_[i]);
            }
            deallocate_pool(mainPool_);

            // Destroy other fully used pools.
            for (auto i = 0ull; i < overflowPoolSize_; ++i)
            {
                auto pool = overflowPools_[i];
                for (auto k = 0ull; k < overflowPoolSize_; ++k)
                {
                    std::destroy_at(pool[k]);
                }
                deallocate_pool(pool);
            }
        }

        // Destroy current partially used pool (main or overflow).
        auto pool = this->current_pool();
        for (auto i = 0ull; i < nextPoolNodeIndex_; ++i)
        {
            std::destroy_at(pool[i]);
        }
        deallocate_pool(pool);
    }

    template<class Data, degree D>
    auto node_pool<Data, D>::operator=
        (node_pool other) -> node_pool&
    {
        this->swap(other);
        return *this;
    }

    template<class Data, degree D>
    auto node_pool<Data, D>::available_node_count
        () const -> std::size_t
    {
        return availableNodes_;
    }

    template<class Data, degree D>
    auto node_pool<Data, D>::main_pool_size
        () const -> std::size_t
    {
        return mainPoolSize_;
    }

    template<class Data, degree D>
    template<class... Args>
    auto node_pool<Data, D>::create
        (Args&&... as) -> node_t*
    {
        assert(availableNodes_ > 0);
        --availableNodes_;

        auto p = static_cast<node_t*>(nullptr);
        if (freeNodeList_)
        {
            p = freeNodeList_;
            freeNodeList_ = freeNodeList_->get_next();
            std::destroy_at(p);
        }
        else
        {
            p = this->current_pool()[nextPoolNodeIndex_];
            ++nextPoolNodeIndex_;
        }

        return std::construct_at(p, std::forward<Args>(as)...);
    }

    template<class Data, degree D>
    auto node_pool<Data, D>::destroy
        (node_t* const p) -> void
    {
        ++availableNodes_;
        p->set_next(freeNodeList_);
        freeNodeList_ = p;
    }

    template<class Data, degree D>
    auto node_pool<Data, D>::grow
        () -> void
    {
        debug::out( "node_pool: Allocating overflow pool with size "
                  , overflowPoolSize_, ".\n" );

        overflowPools_.emplace_back(allocate_pool(overflowPoolSize_));
        currentPoolIndex_  = overflowPools_.size() - 1ull;
        nextPoolNodeIndex_ = 0;
        availableNodes_   += overflowPoolSize_;
    }

    template<class Data, degree D>
    auto node_pool<Data, D>::current_pool
        () const -> node_t*
    {
        return overflowPools_.empty()
            ? mainPool_
            : overflowPools_[currentPoolIndex_];
    }

    template<class Data, degree D>
    auto node_pool<Data, D>::swap
        (node_pool& other) -> void
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

    template<class Data, degree D>
    auto node_pool<Data, D>::allocate_pool
        (std::size_t const size) -> node_t*
    {
        return static_cast<node_t*>(::operator new(size * sizeof(node_t)));
    }

    template<class Data, degree D>
    auto node_pool<Data, D>::deallocate_pool
        (node_t* const ptr) -> void
    {
        ::operator delete(ptr);
    }
}

#endif