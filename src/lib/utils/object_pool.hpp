#ifndef MIX_UTILS_OBJECT_POOL_HPP
#define MIX_UTILS_OBJECT_POOL_HPP

#include <memory>
#include <vector>

namespace teddy::utils
{
    /**
     *  @brief Simple pool of pre-allocated objects in a continuous storage.
     */
    template<class T>
    class object_pool
    {
    public:
        object_pool (std::size_t size);
        object_pool (object_pool const&) = delete;
        object_pool (object_pool&&)      = default;

        auto operator= (object_pool const&) -> object_pool& = delete;
        auto operator= (object_pool&&)      -> object_pool& = default;

        auto set_overflow_ratio (std::size_t denom) -> void;

    public:
        template<class... Args>
        [[nodiscard]] auto try_create (Args&&... args) -> T*;

        template<class... Args>
        [[nodiscard]] auto force_create (Args&&... args) -> T*;

        auto destroy (T* p) -> void;

    private:
        using pool_iterator = typename std::vector<T>::iterator;

    private:
        std::vector<T>              mainPool_;
        std::vector<std::vector<T>> overflowPools_;
        std::vector<T*>             freeObjects_;
        std::vector<T>*             currentPool_;
        pool_iterator               nextObject_;
        std::size_t                 overflowRatio_;
    };

// object_pool definitions:

    template<class T>
    object_pool<T>::object_pool
        (std::size_t const size) :
        mainPool_      (size),
        currentPool_   (std::addressof(mainPool_)),
        nextObject_    (std::begin(mainPool_)),
        overflowRatio_ (2)
    {
        overflowPools_.reserve(100);
    }

    template<class T>
    auto object_pool<T>::set_overflow_ratio
        (std::size_t const denom) -> void
    {
        overflowRatio_ = denom;
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
            // TODO ak sa znovupoužíva vrchol (cez next) treba ho deštruovať!
            // riešiť možno cez pole bajtov, aby nemusel byť default konštruktor?
            traits_t::construct(alloc, p, std::forward<Args>(args)...);
        }

        return p;
    }

    template<class T>
    template<class... Args>
    auto object_pool<T>::force_create
        (Args&&... args) -> T*
    {
        overflowPools_.emplace_back(mainPool_.size() / overflowRatio_);
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