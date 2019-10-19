#ifndef MIX_UTILS_DOUBLE_TOP_STACK
#define MIX_UTILS_DOUBLE_TOP_STACK

#include <vector>
#include <utility>

namespace mix::utils
{
    template<class T>
    class double_top_stack
    {
    private:
        std::vector<T> data;

    public:
        template<class U>
        auto push      (U&& item) -> void;
        auto pop       ()         -> void;
        auto top       ()         -> T&;
        auto under_top ()         -> T&;
        auto size      ()         -> size_t;
    };    

    template<class T>
    template<class U>
    auto double_top_stack<T>::push (U&& item) -> void
    {
        this->data.push_back(std::forward<U>(item));
    }

    template<class T>
    auto double_top_stack<T>::pop () -> void
    {
        this->data.pop_back();
    }

    template<class T>
    auto double_top_stack<T>::top () -> T&
    {
        return this->data.back();
    }

    template<class T>
    auto double_top_stack<T>::under_top () -> T&
    {
        return this->data[this->data.size() - 2];
    }

    template<class T>
    auto double_top_stack<T>::size () -> size_t
    {
        return this->data.size();
    }
}

#endif  