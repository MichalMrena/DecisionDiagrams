#ifndef _MIX_UTILS_DOUBLE_TOP_STACK_
#define _MIX_UTILS_DOUBLE_TOP_STACK_

#include <vector>
#include <utility>

namespace mix::utils
{
    template<class T>
    class double_top_stack
    {
    public:
        using size_type = typename std::vector<T>::size_type;

    private:
        std::vector<T> data;

    public:
        template<class U>
        auto push      (U&& item) -> void;
        auto pop       ()         -> void;
        auto top       () const   -> const T&;
        auto under_top () const   -> const T&;
        auto size      () const   -> size_type;
        auto clear     ()         -> void;  
    };    

    template<class T>
    template<class U>
    auto double_top_stack<T>::push 
        (U&& item) -> void
    {
        this->data.push_back(std::forward<U>(item));
    }

    template<class T>
    auto double_top_stack<T>::pop 
        () -> void
    {
        this->data.pop_back();
    }

    template<class T>
    auto double_top_stack<T>::top 
        () const -> const T&
    {
        return this->data.back();
    }

    template<class T>
    auto double_top_stack<T>::under_top 
        () const -> const T&
    {
        return this->data[this->data.size() - 2];
    }

    template<class T>
    auto double_top_stack<T>::size 
        () const  -> size_type
    {
        return this->data.size();
    }

    template<class T>
    auto double_top_stack<T>::clear 
        () -> void
    {
        this->data.clear();
    }
}

#endif  