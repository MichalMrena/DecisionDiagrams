#ifndef MIX_DD_VERTEX_LEVEL_ITERATOR_HPP
#define MIX_DD_VERTEX_LEVEL_ITERATOR_HPP

#include <utility>
#include <iterator>

namespace mix::dd
{
    template<class MapIterator>
    class vertex_level_iterator
    {
    public:
        using map_traits_t      = std::iterator_traits<MapIterator>;
        using value_type        = typename map_traits_t::value_type::second_type;
        using reference         = typename map_traits_t::value_type::second_type;
        using pointer           = typename map_traits_t::value_type::second_type;
        using iterator_category = typename map_traits_t::iterator_category;
        using difference_type   = std::ptrdiff_t;

    public:
        template<class MapIteratorRef>
        vertex_level_iterator (MapIteratorRef&& mapIterator);

        auto operator== (vertex_level_iterator const& rhs) const -> bool;
        auto operator!= (vertex_level_iterator const& rhs) const -> bool;
        auto operator*  () const -> reference;
        auto operator-> () const -> pointer;
        auto operator++ ()       -> vertex_level_iterator&;
        auto operator++ (int)    -> vertex_level_iterator;

    private:
        MapIterator iterator_;
    };

    template<class MapIterator>
    template<class MapIteratorRef>
    vertex_level_iterator<MapIterator>::vertex_level_iterator
        (MapIteratorRef&& mapIterator) :
        iterator_ {std::forward<MapIteratorRef>(mapIterator)}
    {
    }

    template<class MapIterator>
    auto vertex_level_iterator<MapIterator>::operator==
        (vertex_level_iterator const& rhs) const -> bool
    {
        return iterator_ == rhs.iterator_;
    }

    template<class MapIterator>
    auto vertex_level_iterator<MapIterator>::operator!=
        (vertex_level_iterator const& rhs) const -> bool
    {
        return iterator_ != rhs.iterator_;
    }

    template<class MapIterator>
    auto vertex_level_iterator<MapIterator>::operator*
        () const -> reference
    {
        auto&& [key, vertex] = *iterator_;
        return vertex;
    }

    template<class MapIterator>
    auto vertex_level_iterator<MapIterator>::operator->
        () const -> pointer
    {
        auto&& [key, vertex] = *iterator_;
        return vertex;
    }

    template<class MapIterator>
    auto vertex_level_iterator<MapIterator>::operator++
        () -> vertex_level_iterator&
    {
        ++iterator_;
        return *this;
    }

    template<class MapIterator>
    auto vertex_level_iterator<MapIterator>::operator++
        (int) -> vertex_level_iterator
    {
        auto const ret = *this;
        ++(*this);
        return ret;
    }
}

#endif