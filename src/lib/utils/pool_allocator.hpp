#ifndef MIX_UTILS_POOL_ALLOCATOR_
#define MIX_UTILS_POOL_ALLOCATOR_

#include <functional>
#include <memory>

namespace mix::utils
{
    template<class Pool>
    class pool_allocator
    {
    public:
        using value_type   = typename Pool::value_type;
        using pointer      = typename Pool::pointer;
        using is_recycling = void;

    public:
        pool_allocator  (Pool& pool);
        auto allocate   (std::size_t)                  -> pointer;
        auto deallocate (pointer const p, std::size_t) -> void;

    private:
        using pool_ref = std::reference_wrapper<Pool>;

    private:
        pool_ref pool_;
    };

    template<class Pool>
    pool_allocator<Pool>::pool_allocator
        (Pool& pool) :
        pool_ {std::ref(pool)}
    {
    }

    template<class Pool>
    auto pool_allocator<Pool>::allocate
        (std::size_t) -> pointer
    {
        return pool_.get().allocate_memory();
    }

    template<class Pool>
    auto pool_allocator<Pool>::deallocate
        (pointer const p, std::size_t) -> void
    {
        return pool_.get().release_memory(p);
    }
}

#endif