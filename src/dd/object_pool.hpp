#ifndef _MIX_DD_OBJECT_POOL_
#define _MIX_DD_OBJECT_POOL_

#include <deque>
#include <utility>
#include "graph.hpp"

namespace mix::dd
{
    template<class T>
    class object_pool
    {
    private:
        std::deque<T*> pool;

    public:
        ~object_pool();

        template<class... Args>
        auto get_object (Args&&... args) -> T*;
        auto put_object (T* const p)     -> void;
    };    

    template<class T>
    object_pool<T>::~object_pool()
    {
        for (auto ptr : this->pool)
        {
            delete ptr;
        }
    }

    template<class T>
    template<class... Args>
    auto object_pool<T>::get_object
        (Args&&... args) -> T*
    {
        if (this->pool.empty())
        {
            return new T {std::forward<Args>(args)...};
        }
        
        const auto objptr {this->pool.back()};
        this->pool.pop_back();
        *objptr = T {std::forward<Args>(args)...};
        return objptr;
    }

    template<class T>
    auto object_pool<T>::put_object
        (T* const p) -> void
    {
        this->pool.push_back(p);
    }
}

#endif