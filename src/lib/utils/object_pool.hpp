#ifndef MIX_UTILS_OBJECT_POOL_HPP
#define MIX_UTILS_OBJECT_POOL_HPP

#include <memory>
#include <utility>
#include <vector>
#include <deque>

namespace mix::utils
{
    template<class T>
    class dummy_object_pool
    {
    public:
        template<class... Args>
        [[nodiscard]] auto try_create (Args&&... args) -> T*;

        template<class... Args>
        [[nodiscard]] auto force_create (Args&&... args) -> T*;

        auto destroy (T* const p) -> void;

    private:
        using alloc_t  = std::allocator<T>;
        using traits_t = std::allocator_traits<alloc_t>;

    private:
        alloc_t alloc_;
    };

    template<class T>
    class object_pool
    {
    public:
        object_pool (std::size_t const size);

        template<class... Args>
        [[nodiscard]] auto try_create (Args&&... args) -> T*;

        template<class... Args>
        [[nodiscard]] auto force_create (Args&&... args) -> T*;

        auto destroy (T* const p) -> void;

    private:
        using alloc_t  = std::allocator<T>;
        using traits_t = std::allocator_traits<alloc_t>;

    private:
        auto is_from_pool (T* const p) const -> bool;

    private:
        alloc_t        alloc_;
        std::vector<T> objects_;
        std::deque<T*> available_;
    };

// dummy_object_pool definitions:

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
        auto const p = traits_t::allocate(alloc_, 1);
        traits_t::construct(alloc_, p, std::forward<Args>(args)...);
        return p;
    }

    template<class T>
    auto dummy_object_pool<T>::destroy
        (T* const p) -> void
    {
        traits_t::destroy(alloc_, p);
        traits_t::deallocate(alloc_, p, 1);
    }

// object_pool definitions:

    template<class T>
    object_pool<T>::object_pool
        (std::size_t const size) :
        objects_   (size),
        available_ (size)
    {
        auto in    = std::begin(objects_);
        auto endIn = std::end(objects_);
        auto out   = std::begin(available_);
        while (in != endIn)
        {
            *out = std::addressof(*in);
            ++in;
            ++out;
        }
    }

    template<class T>
    template<class... Args>
    auto object_pool<T>::try_create
        (Args&&... args) -> T*
    {
        if (available_.empty())
        {
            return nullptr;
        }

        auto const p = available_.back();
        available_.pop_back();
        traits_t::construct(alloc_, p, std::forward<Args>(args)...);
        return p;
    }

    template<class T>
    template<class... Args>
    auto object_pool<T>::force_create
        (Args&&... args) -> T*
    {
        auto const p = traits_t::allocate(alloc_, 1);
        traits_t::construct(alloc_, p, std::forward<Args>(args)...);
        return p;
    }

    template<class T>
    auto object_pool<T>::destroy
        (T* const p) -> void
    {
        if (this->is_from_pool(p))
        {
            available_.push_back(p);
        }
        else
        {
            traits_t::destroy(alloc_, p);
            traits_t::deallocate(alloc_, p, 1);
        }
    }

    template<class T>
    auto object_pool<T>::is_from_pool
        (T* const p) const -> bool
    {
        auto const first = std::addressof(objects_.front());
        auto const last  = std::addressof(objects_.back());
        return p >= first && p <= last;
    }
}

#endif