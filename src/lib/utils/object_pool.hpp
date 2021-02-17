#ifndef MIX_UTILS_OBJECT_POOL_HPP
#define MIX_UTILS_OBJECT_POOL_HPP

#include <memory>
#include <utility>
#include <vector>
#include <deque>

namespace mix::utils
{
    /**
        @brief Operators new and delete hidden behind pool interface.
     */
    template<class T>
    class dummy_object_pool
    {
    public:
        dummy_object_pool (std::size_t const);

    public:
        template<class... Args>
        [[nodiscard]] auto try_create (Args&&... args) -> T*;

        template<class... Args>
        [[nodiscard]] auto force_create (Args&&... args) -> T*;

        auto destroy (T* const p) -> void;
    };

    /**
        @brief Simple pool of pre-allocated objects in a continuous storage.
     */
    template<class T>
    class object_pool
    {
    public:
        object_pool (std::size_t const size);
        object_pool (object_pool const&) = delete;
        object_pool (object_pool&&)      = default;

        auto operator= (object_pool const&) -> object_pool& = delete;
        auto operator= (object_pool&&)      -> object_pool& = default;

    public:
        template<class... Args>
        [[nodiscard]] auto try_create (Args&&... args) -> T*;

        template<class... Args>
        [[nodiscard]] auto force_create (Args&&... args) -> T*;

        auto destroy (T* const p) -> void;

    private:
        using pool_iterator = typename std::vector<T>::iterator;

    private:
        std::vector<T>              mainPool_;
        std::vector<std::vector<T>> overflowPools_;
        std::vector<T*>             freeObjects_;
        std::vector<T>*             currentPool_;
        pool_iterator               nextObject_;
    };

// dummy_object_pool definitions:

    template<class T>
    dummy_object_pool<T>::dummy_object_pool
        (std::size_t const)
    {
    }

    template<class T>
    template<class... Args>
    auto dummy_object_pool<T>::try_create
        (Args&&... args) -> T*
    {
        return this->force_create(std::forward<Args>(args)...);
    }

    template<class T>
    template<class... Args>
    auto dummy_object_pool<T>::force_create
        (Args&&... args) -> T*
    {
        return new T(std::forward<Args>(args)...);
    }

    template<class T>
    auto dummy_object_pool<T>::destroy
        (T* const p) -> void
    {
        delete p;
    }

// object_pool definitions:

    template<class T>
    object_pool<T>::object_pool
        (std::size_t const size) :
        mainPool_    (size),
        currentPool_ (std::addressof(mainPool_)),
        nextObject_  (std::begin(mainPool_))
    {
    }

    template<class T>
    template<class... Args>
    auto object_pool<T>::try_create
        (Args&&... args) -> T*
    {
        using alloc_t  = decltype(currentPool_->get_allocator());
        using traits_t = std::allocator_traits<alloc_t>;

        auto p = static_cast<T*>(nullptr);

        if (nextObject_ != std::end(*currentPool_))
        {
            p = std::addressof(*nextObject_);
            ++nextObject_;
        }
        else if (!freeObjects_.empty())
        {
            p = freeObjects_.back();
            freeObjects_.pop_back();
        }

        if (p)
        {
            auto alloc = currentPool_->get_allocator();
            traits_t::construct(alloc, p, std::forward<Args>(args)...);
        }

        return p;
    }

    template<class T>
    template<class... Args>
    auto object_pool<T>::force_create
        (Args&&... args) -> T*
    {
        overflowPools_.emplace_back(mainPool_.size() / 2);
        currentPool_ = std::addressof(overflowPools_.back());
        nextObject_  = std::begin(*currentPool_);
        return this->try_create(std::forward<Args>(args)...);
    }

    template<class T>
    auto object_pool<T>::destroy
        (T* const p) -> void
    {
        freeObjects_.push_back(p);
    }
}

#endif