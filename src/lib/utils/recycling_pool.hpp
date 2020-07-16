#ifndef MIX_UTILS_RECYCLING_POOL_
#define MIX_UTILS_RECYCLING_POOL_

#include <memory>
#include <deque>
#include <utility>
#include <limits>

namespace mix::utils
{
    template<class T>
    class recycling_pool
    {
    public:
        using value_type = T;
        using pointer    = T*;

    public:
        recycling_pool  (recycling_pool const&) = delete;
        recycling_pool  (recycling_pool&&)      = default;
        recycling_pool  (std::size_t const maxSize = std::numeric_limits<std::size_t>::max());
        ~recycling_pool ();

        [[nodiscard]]
        auto allocate_memory ()          -> pointer;
        auto release_memory  (pointer p) -> void;
        auto release_all     ()          -> void;

    private:
        using allocator_t    = std::allocator<T>;
        using alloc_traits_t = std::allocator_traits<allocator_t>;
        using queue_t        = std::deque<T*>;

    private:
        [[nodiscard]]
        auto allocate_new   () -> pointer;

    private:
        std::size_t maxSize_;
        allocator_t alloc_;
        queue_t     recycled_;
        pointer     lastAllocated_;
    };

    template<class T>
    recycling_pool<T>::recycling_pool
        (std::size_t const maxSize) :
        maxSize_       {maxSize},
        lastAllocated_ {nullptr}
    {
    }

    template<class T>
    recycling_pool<T>::~recycling_pool()
    {
        this->release_all();
    }

    template<class T>
    auto recycling_pool<T>::allocate_memory
        () -> pointer
    {
        if (recycled_.empty())
        {
            return this->allocate_new();
        }
        
        auto const p = recycled_.back();
        recycled_.pop_back();
        return p;
    }

    template<class T>
    auto recycling_pool<T>::release_memory
        (pointer p) -> void
    {
        if (recycled_.size() <= maxSize_)
        {
            recycled_.push_back(p);
        }
        else
        {
            alloc_traits_t::deallocate(alloc_, p, 1);
        }
    }

    template<class T>
    auto recycling_pool<T>::release_all
        () -> void
    {
        for (auto const p : recycled_)
        {
            alloc_traits_t::deallocate(alloc_, p, 1);
        }

        recycled_.clear();
    }

    template<class T>
    auto recycling_pool<T>::allocate_new
        () -> pointer
    {
        return (lastAllocated_ = alloc_traits_t::allocate(alloc_, 1, lastAllocated_));
    }
}

#endif