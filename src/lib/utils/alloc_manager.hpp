#ifndef MIX_UTILS_ALLOC_MANAGER_
#define MIX_UTILS_ALLOC_MANAGER_

#include <memory>

namespace mix::utils
{
    template<class Alloc>
    class alloc_manager
    {
    public:
        using traits_t   = typename std::allocator_traits<Alloc>;
        using value_type = typename traits_t::value_type;
        using pointer    = typename traits_t::pointer;

    public:
        alloc_manager(Alloc const& alloc);

        template<class... Args>
        auto create    (Args&&... args)  -> pointer;
        auto release   (pointer const p) -> void;
        auto get_alloc () const          -> Alloc;

    private:
        Alloc alloc_;
    };

    template<class Alloc>
    alloc_manager<Alloc>::alloc_manager
        (Alloc const& alloc) :
        alloc_ {alloc}
    {
    }

    template<class Alloc>
    template<class... Args>
    auto alloc_manager<Alloc>::create
        (Args&&... args) -> pointer
    {
        auto const p = std::allocator_traits<Alloc>::allocate(alloc_, 1);
        std::allocator_traits<Alloc>::construct(alloc_, p, std::forward<Args>(args)...);
        return p;
    }

    template<class Alloc>
    auto alloc_manager<Alloc>::release
        (pointer const p) -> void
    {
        std::allocator_traits<Alloc>::destroy(alloc_, p);
        std::allocator_traits<Alloc>::deallocate(alloc_, p, 1);
    }

    template<class Alloc>
    auto alloc_manager<Alloc>::get_alloc
        () const -> Alloc
    {
        return alloc_;
    }
}

#endif
