#ifndef _MIX_DD_LIST_MAP_
#define _MIX_DD_LIST_MAP_

#include <functional>
#include <vector>
#include <initializer_list>
#include <stdexcept>

namespace mix::dd
{
    template<class Key, class T, class KeyEqual = std::equal_to<Key>>
    class list_map
    {
    public:
        using value_type     = std::pair<const Key, T>;
        using iterator       = typename std::vector<value_type>::iterator;
        using const_iterator = typename std::vector<value_type>::const_iterator;

    private:
        std::vector<value_type> data;

    public:
        ~list_map() = default;
        list_map(const list_map&) = delete;

        list_map(list_map&& other);
        list_map(const size_t initialCapacity = 4);
        list_map(std::initializer_list<value_type> init);

        auto at (const Key& k)       -> T&;
        auto at (const Key& k) const -> const T&;

        auto operator[] (Key&& k) -> T&;

        auto begin () -> iterator;
        auto end   () -> iterator;

        auto begin () const -> const_iterator;
        auto end   () const -> const_iterator;
    };
    
    template<class Key, class T, class KeyEqual>
    list_map<Key, T, KeyEqual>::list_map
        (list_map&& other) :
        data {std::move(other.data)}
    {
    }

    template<class Key, class T, class KeyEqual>
    list_map<Key, T, KeyEqual>::list_map
        (const size_t initialCapacity)
    {
        this->data.reserve(initialCapacity);
    }

    template<class Key, class T, class KeyEqual>
    list_map<Key, T, KeyEqual>::list_map
        (std::initializer_list<value_type> init) :
        data {init}
    {
    }

    template<class Key, class T, class KeyEqual>
    auto list_map<Key, T, KeyEqual>::at
        (const Key& k) -> T&
    {
        KeyEqual eq;

        for (auto& pair : this->data)
        {
            if (eq(pair.first, k))
            {
                return pair.second;
            }
        }

        throw std::out_of_range {"Key not found."};
    }

    template<class Key, class T, class KeyEqual>
    auto list_map<Key, T, KeyEqual>::at
        (const Key& k) const -> const T&
    {
        KeyEqual eq;

        for (auto& pair : this->data)
        {
            if (eq(pair.first, k))
            {
                return pair.second;
            }
        }

        throw std::out_of_range {"Key not found."};
    }

    template<class Key, class T, class KeyEqual>
    auto list_map<Key, T, KeyEqual>::operator[]
        (Key&& k) -> T&
    {
        KeyEqual eq;

        for (auto& pair : this->data)
        {
            if (eq(pair.first, k))
            {
                return pair.second;
            }
        }

        this->data.emplace_back(std::forward<Key>(k), T {});

        return this->data.back().second;
    }

    template<class Key, class T, class KeyEqual>
    auto list_map<Key, T, KeyEqual>::begin
        () -> iterator
    {
        return this->data.begin();
    }

    template<class Key, class T, class KeyEqual>
    auto list_map<Key, T, KeyEqual>::end
        () -> iterator
    {
        return this->data.end();
    }

    template<class Key, class T, class KeyEqual>
    auto list_map<Key, T, KeyEqual>::begin
        () const -> const_iterator
    {
        return this->data.cbegin();
    }

    template<class Key, class T, class KeyEqual>
    auto list_map<Key, T, KeyEqual>::end
        () const -> const_iterator
    {
        return this->data.cend();
    }
}

#endif