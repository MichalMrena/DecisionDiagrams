#ifndef _MIX_DD_OBJECT_POOL_
#define _MIX_DD_OBJECT_POOL_

#include <deque>
#include <utility>

namespace mix::dd
{
    template<class T>
    class object_pool
    {
    public:
        ~object_pool();

        template<class... Args>
        auto create_object  (Args&&... args) -> T*;
        auto release_object (T* const p)     -> void;

    private:
        std::deque<T*> pool_;
    };    

    template<class T>
    object_pool<T>::~object_pool()
    {
        for (auto ptr : pool_)
        {
            delete ptr;
        }
    }

    template<class T>
    template<class... Args>
    auto object_pool<T>::create_object
        (Args&&... args) -> T*
    {
        if (pool_.empty())
        {
            return new T {std::forward<Args>(args)...};
        }
        
        const auto objptr {pool_.back()};
        pool_.pop_back();
        *objptr = T {std::forward<Args>(args)...};
        return objptr;
    }

    template<class T>
    auto object_pool<T>::release_object
        (T* const p) -> void
    {
        pool_.push_back(p);
    }
}

#endif