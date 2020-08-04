#ifndef MIX_DD_MDD_LEVEL_ITERATOR_HPP
#define MIX_DD_MDD_LEVEL_ITERATOR_HPP

#include "graph.hpp"

#include <vector>
#include <set>
#include <iterator>
#include <type_traits>

namespace mix::dd
{
    template<class VertexData, class ArcData, size_t N, bool IsConst>
    class mdd_level_iterator
    {
    public:
        using vertex_t          = vertex<VertexData, ArcData, N>;
        using value_type        = std::conditional_t<IsConst, vertex_t const, vertex_t>;
        using reference         = value_type&;
        using pointer           = value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;

    public:
        mdd_level_iterator ( vertex_t* const      root
                           , std::size_t const    variableCount );
        mdd_level_iterator ( mdd_level_iterator const&  other );
        mdd_level_iterator ( mdd_level_iterator&& other );

        auto operator=  (mdd_level_iterator rhs)              -> mdd_level_iterator&;
        auto operator!= (mdd_level_iterator const& rhs) const -> bool;
        auto operator== (mdd_level_iterator const& rhs) const -> bool;

        auto operator*  () const -> reference;
        auto operator-> () const -> pointer;
        auto operator++ ()       -> mdd_level_iterator&;
        auto operator++ (int)    -> mdd_level_iterator;

        auto swap (mdd_level_iterator& rhs) -> void;

    private:
        using vertex_container_t = std::set<vertex_t*>; // TODO ak sa použije mark, stačil by tu vektor
        using level_container_t  = std::vector<vertex_container_t>;
        using level_iterator_t   = typename level_container_t::iterator;
        using vertex_iterator_t  = typename vertex_container_t::iterator; 

    private:
        auto advance_iterators () -> void;

    private:
        level_container_t levels_;
        level_iterator_t  levelIterator_;
        vertex_iterator_t vertexIterator_;
    };    

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    auto swap( mdd_level_iterator<VertexData, ArcData, N, IsConst>&
             , mdd_level_iterator<VertexData, ArcData, N, IsConst>& ) noexcept -> void;

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    mdd_level_iterator<VertexData, ArcData, N, IsConst>::mdd_level_iterator
        (vertex_t* const root, std::size_t const variableCount) :
        levels_ (variableCount + 1)
    {
        if (root)
        {
            levels_[root->index].insert(root);
            levelIterator_  = levels_.begin() + root->index; // advance...
            vertexIterator_ = (*levelIterator_).begin();
        }
        else
        {
            levelIterator_  = levels_.end();
        }
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    mdd_level_iterator<VertexData, ArcData, N, IsConst>::mdd_level_iterator
        (mdd_level_iterator const& other) :
        levels_         {other.levels_},
        levelIterator_  {levels_.begin() + std::distance(other.levels_.begin(), other.levelIterator_)},
        vertexIterator_ {(*levelIterator_).begin()}
    {
        std::advance(vertexIterator_, std::distance((*other.levelIterator_).begin(), other.vertexIterator_));
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    mdd_level_iterator<VertexData, ArcData, N, IsConst>::mdd_level_iterator
        (mdd_level_iterator&& other) :
        levels_         {std::move(other.levels_)}
      , levelIterator_  {std::move(other.levelIterator_)}
      , vertexIterator_ {std::move(other.vertexIterator_)}
    {
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    auto mdd_level_iterator<VertexData, ArcData, N, IsConst>::operator!=
        (const mdd_level_iterator& rhs) const -> bool
    {
        return ! (*this == rhs);
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    auto mdd_level_iterator<VertexData, ArcData, N, IsConst>::operator=
        (mdd_level_iterator rhs) -> mdd_level_iterator&
    {
        using std::swap;
        swap(*this, rhs);
        return *this;
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    auto mdd_level_iterator<VertexData, ArcData, N, IsConst>::operator==
        (const mdd_level_iterator& rhs) const -> bool
    {
        auto const bothEnded = levelIterator_ == levels_.end() && rhs.levelIterator_ == rhs.levels_.end();
        if (bothEnded)
        {
            return true;
        }

        // TODO idealne bez const castu, const levelIterator by sa dal vytvoriť podľa atribútu v rovnakej vzdialenosti
        auto const levelItDistanceIsEqual = std::distance(const_cast<level_container_t&>(levels_).begin(), levelIterator_) 
                                         == std::distance(const_cast<level_container_t&>(rhs.levels_).begin(), rhs.levelIterator_);

        if (! levelItDistanceIsEqual)
        {
            return false;
        }
        
        auto const vertexItDistanceIsEqual = std::distance((*levelIterator_).begin(), vertexIterator_) 
                                          == std::distance((*rhs.levelIterator_).begin(), rhs.vertexIterator_);

        return vertexItDistanceIsEqual;
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    auto mdd_level_iterator<VertexData, ArcData, N, IsConst>::operator*
        () const -> reference
    {
        return **vertexIterator_;
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    auto mdd_level_iterator<VertexData, ArcData, N, IsConst>::operator->
        () const -> pointer
    {
        return *vertexIterator_;
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    auto mdd_level_iterator<VertexData, ArcData, N, IsConst>::operator++
        () -> mdd_level_iterator&
    {
        this->advance_iterators();
        return *this;
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    auto mdd_level_iterator<VertexData, ArcData, N, IsConst>::operator++
        (int) -> mdd_level_iterator
    {
        auto ret = mdd_level_iterator {*this};
        ++(*this);
        return ret;
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    auto mdd_level_iterator<VertexData, ArcData, N, IsConst>::advance_iterators
        () -> void
    {
        auto const currVertex = *vertexIterator_;

        for (auto i = 0u; i < N; ++i)
        {
            auto const son = currVertex->son(i);
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
            } 
            while ((levelIterator_ != levels_.end()) && (*levelIterator_).empty());

            if (levelIterator_ != levels_.end())
            {
                vertexIterator_ = (*levelIterator_).begin();
            }
        }
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    auto mdd_level_iterator<VertexData, ArcData, N, IsConst>::swap
        (mdd_level_iterator& rhs) -> void
    {
        using std::swap;
        swap(levels_,         rhs.levels_        );
        swap(levelIterator_,  rhs.levelIterator_ );
        swap(vertexIterator_, rhs.vertexIterator_);
    }

    template<class VertexData, class ArcData, size_t N, bool IsConst>
    auto swap( mdd_level_iterator<VertexData, ArcData, N, IsConst>& lhs
             , mdd_level_iterator<VertexData, ArcData, N, IsConst>& rhs ) noexcept -> void
    {
        lhs.swap(rhs);
    }
}

#endif