#ifndef MIX_DD_NODE_POOL_HPP
#define MIX_DD_NODE_POOL_HPP

#include "node.hpp"

#include <array>
#include <cstddef>
#include <memory>

namespace teddy
{
    template<class T>
    class mem_wrap
    {
    private:
        std::array<std::byte, sizeof(T)> bytes_;
    public:
        auto get ()
        {
            return reinterpret_cast<T*>(bytes_.data());
        }
    };

    template<class Data, degree D>
    class node_pool
    {
    public:
        using node_t = node<Data, D>;
        using sons_t = typename node_t::sons_t;

    public:
        node_pool  (std::size_t);
        node_pool  (node_pool const&) = delete;
        node_pool  (node_pool&&)      = default;
        ~node_pool ();

        auto operator= (node_pool const&) -> node_pool& = delete;
        auto operator= (node_pool&&)      -> node_pool& = default;

        auto set_overflow_ratio (std::size_t) -> void;

        template<class... Args>
        [[nodiscard]] auto try_create (Args&&...) -> node_t*;

        template<class... Args>
        [[nodiscard]] auto force_create (Args&&...) -> node_t*;

        auto destroy (node_t*) -> void;

    private:
        using pool_it = typename std::vector<mem_wrap<node_t>>::iterator;

    private:
        std::vector<mem_wrap<node_t>>              mainPool_;
        std::vector<std::vector<mem_wrap<node_t>>> overflowPools_;
        std::vector<mem_wrap<node_t>>*             currentPoolPtr_;
        node_t*                                    freeNode_;
        pool_it                                    nextPoolNodeIt_;
        std::size_t                                overflowRatio_;
    };

    template<class Data, degree D>
    node_pool<Data, D>::node_pool
        (std::size_t const initSize) :
        mainPool_       (initSize),
        overflowPools_  (),
        currentPoolPtr_ (&mainPool_),
        freeNode_       (nullptr),
        nextPoolNodeIt_ (std::begin(mainPool_)),
        overflowRatio_  (2)
    {
    }

    template<class Data, degree D>
    node_pool<Data, D>::~node_pool
        ()
    {
        if (currentPoolPtr_ != &mainPool_)
        {
            // Destroy main pool.
            for (auto& mem : mainPool_)
            {
                std::destroy_at(mem.get());
            }

            // Destroy other fully used pools.
            auto poolPtr = &overflowPools_.first();
            while (poolPtr != currentPoolPtr_)
            {
                for (auto& mem : *poolPtr)
                {
                    std::destroy_at(mem.get());
                }
                ++poolPtr;
            }
        }

        // Destroy current partially used pool (main or overflow).
        auto it = std::begin(*currentPoolPtr_);
        while (it != nextPoolNodeIt_)
        {
            std::destroy_at(it->get());
            ++it;
        }
    }

    template<class Data, degree D>
    auto node_pool<Data, D>::set_overflow_ratio
        (std::size_t const d) -> void
    {
        overflowRatio_ = d;
    }

    template<class Data, degree D>
    template<class... Args>
    auto node_pool<Data, D>::try_create
        (Args&&... as) -> node_t*
    {
        auto p = static_cast<node_t*>(nullptr);

        if (nextPoolNodeIt_ != std::end(*currentPoolPtr_))
        {
            p = nextPoolNodeIt_->get();
            ++nextPoolNodeIt_;
        }
        else if (freeNode_)
        {
            p = freeNode_;
            freeNode_ = freeNode_->get_next();
            std::destroy_at(p);
        }

        if (p)
        {
            return std::construct_at(p, std::forward<Args>(as)...);
        }

        return p;
    }

    template<class Data, degree D>
    template<class... Args>
    auto node_pool<Data, D>::force_create
        (Args&&... as) -> node_t*
    {
        overflowPools_.emplace_back(mainPool_.size() / overflowRatio_);
        currentPoolPtr_ = &overflowPools_.back();
        nextPoolNodeIt_ = std::begin(*currentPoolPtr_);
        return this->try_create(std::forward<Args>(as)...);
    }

    template<class Data, degree D>
    auto node_pool<Data, D>::destroy
        (node_t* const p) -> void
    {
        p->set_next(freeNode_);
        freeNode_ = p;
    }
}

#endif