#ifndef _MIX_DD_LEVEL_ITERATOR_
#define _MIX_DD_LEVEL_ITERATOR_

#include <vector>
#include <set>
#include <iterator>
#include "graph.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData, size_t N, class ValueType>
    class dd_level_iterator;

    template<class VertexData, class ArcData, size_t N, class ValueType>
    auto swap( dd_level_iterator<VertexData, ArcData, N, ValueType>& lhs
             , dd_level_iterator<VertexData, ArcData, N, ValueType>& rhs ) noexcept -> void;

    template<class VertexData, class ArcData, size_t N, class ValueType>
    class dd_level_iterator
    {
    public:
        using value_type        = ValueType;
        using reference         = ValueType&;
        using pointer           = ValueType*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;

    private:
        using vertex_t           = vertex<VertexData, ArcData, N>;
        using vertex_container_t = std::set<vertex_t*>;
        using level_container_t  = std::vector< vertex_container_t >;
        using level_iterator_t   = typename level_container_t::iterator;
        using vertex_iterator_t  = typename vertex_container_t::iterator; 

    private:
        level_container_t levels_;
        level_iterator_t  levelIterator_;
        vertex_iterator_t vertexIterator_;

    public:
        dd_level_iterator ( vertex_t* const root
                          , const size_t    variableCount );
        dd_level_iterator (dd_level_iterator&  other);
        dd_level_iterator (dd_level_iterator&& other);

        auto operator=  (dd_level_iterator rhs)              -> dd_level_iterator&;
        auto operator!= (const dd_level_iterator& rhs) const -> bool;
        auto operator== (const dd_level_iterator& rhs) const -> bool;

        auto operator*  () const -> reference;
        auto operator-> () const -> pointer;
        auto operator++ ()       -> dd_level_iterator&;
        auto operator++ (int)    -> dd_level_iterator;

        friend auto swap<VertexData, ArcData, N, ValueType>
            ( dd_level_iterator<VertexData, ArcData, N, ValueType>& lhs
            , dd_level_iterator<VertexData, ArcData, N, ValueType>& rhs ) noexcept -> void;

    private:
        auto advance_iterators () -> void;
    };    

    template<class VertexData, class ArcData, size_t N, class ValueType>
    dd_level_iterator<VertexData, ArcData, N, ValueType>::dd_level_iterator
        (vertex_t* const root, const size_t variableCount) :
        levels_(variableCount + 1)
    {
        if (root)
        {
            levels_.at(root->index).insert(root);
            levelIterator_  = levels_.begin() + root->index;
            vertexIterator_ = (*levelIterator_).begin();
        }
        else
        {
            levelIterator_  = levels_.end();
        }
    }

    template<class VertexData, class ArcData, size_t N, class ValueType>
    dd_level_iterator<VertexData, ArcData, N, ValueType>::dd_level_iterator
        (dd_level_iterator& other) :
        levels_         {other.levels_}
      , levelIterator_  {levels_.begin() + std::distance(other.levels_.begin(), other.levelIterator_)}
      , vertexIterator_ {(*levelIterator_).begin()}
    {
        std::advance(vertexIterator_, std::distance((*other.levelIterator_).begin(), other.vertexIterator_));
    }

    template<class VertexData, class ArcData, size_t N, class ValueType>
    dd_level_iterator<VertexData, ArcData, N, ValueType>::dd_level_iterator
        (dd_level_iterator&& other) :
        levels_         {std::move(other.levels_)}
      , levelIterator_  {std::move(other.levelIterator_)}
      , vertexIterator_ {std::move(other.vertexIterator_)}
    {
    }

    template<class VertexData, class ArcData, size_t N, class ValueType>
    auto dd_level_iterator<VertexData, ArcData, N, ValueType>::operator!=
        (const dd_level_iterator& rhs) const -> bool
    {
        return ! (*this == rhs);
    }

    template<class VertexData, class ArcData, size_t N, class ValueType>
    auto dd_level_iterator<VertexData, ArcData, N, ValueType>::operator=
        (dd_level_iterator rhs) -> dd_level_iterator&
    {
        using std::swap;
        swap(*this, rhs);
        return *this;
    }

    template<class VertexData, class ArcData, size_t N, class ValueType>
    auto dd_level_iterator<VertexData, ArcData, N, ValueType>::operator==
        (const dd_level_iterator& rhs) const -> bool
    {
        return (levelIterator_ == levels_.end() && rhs.levelIterator_ == rhs.levels_.end())
            || (levelIterator_ == rhs.levelIterator_ && vertexIterator_ == rhs.vertexIterator_);
    }

    template<class VertexData, class ArcData, size_t N, class ValueType>
    auto dd_level_iterator<VertexData, ArcData, N, ValueType>::operator*
        () const -> reference
    {
        return **vertexIterator_;
    }

    template<class VertexData, class ArcData, size_t N, class ValueType>
    auto dd_level_iterator<VertexData, ArcData, N, ValueType>::operator->
        () const -> pointer
    {
        return *vertexIterator_;
    }

    template<class VertexData, class ArcData, size_t N, class ValueType>
    auto dd_level_iterator<VertexData, ArcData, N, ValueType>::operator++
        () -> dd_level_iterator&
    {
        advance_iterators();
        return *this;
    }

    template<class VertexData, class ArcData, size_t N, class ValueType>
    auto dd_level_iterator<VertexData, ArcData, N, ValueType>::operator++
        (int) -> dd_level_iterator
    {
        dd_level_iterator ret {*this};
        ++(*this);
        return ret;
    }

    template<class VertexData, class ArcData, size_t N, class ValueType>
    auto dd_level_iterator<VertexData, ArcData, N, ValueType>::advance_iterators
        () -> void
    {
        const auto currVertex {*vertexIterator_};

        for (size_t i {0}; i < N; ++i)
        {
            const auto son {currVertex->son(i)};
            if (son)
            {
                levels_.at(son->index).insert(son);
            }
        }
        
        ++vertexIterator_;
        if (vertexIterator_ == (*levelIterator_).end())
        {
            // More memmory friendly, but can slow down the iteration.
            (*levelIterator_).clear();
            do
            {
                ++levelIterator_;
            } while ((levelIterator_ != levels_.end()) && (*levelIterator_).empty());

            if (levelIterator_ != levels_.end())
            {
                vertexIterator_ = (*levelIterator_).begin();
            }
        }
    }

    template<class VertexData, class ArcData, size_t N, class ValueType>
    auto swap( dd_level_iterator<VertexData, ArcData, N, ValueType>& lhs
             , dd_level_iterator<VertexData, ArcData, N, ValueType>& rhs ) noexcept -> void
    {
        using std::swap;
        swap(lhs.levels_,         rhs.levels_        );
        swap(lhs.levelIterator_,  rhs.levelIterator_ );
        swap(lhs.vertexIterator_, rhs.vertexIterator_);
    }
}

#endif