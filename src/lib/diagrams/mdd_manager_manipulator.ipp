#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include <cmath>
#include <iterator>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    template<template<std::size_t> class Op>
    auto mdd_manager<VertexData, ArcData, P>::apply
        (mdd_t const& lhs, mdd_t const& rhs) -> mdd_t
    {
        auto const go = [this](auto&& go, auto l, auto r)
        {
            auto const cacheIterator = manager_.template cache_find<Op<P>>(l, r);
            if (cacheIterator->matches(l, r))
            {
                return cacheIterator->result;
            }

            auto const lhsVal = manager_.get_vertex_value(l);
            auto const rhsVal = manager_.get_vertex_value(r);
            auto const opVal  = Op<P> () (lhsVal, rhsVal);
            auto u = static_cast<vertex_t*>(nullptr);

            if (!is_nondetermined<P>(opVal))
            {
                u = manager_.terminal_vertex(opVal);
            }
            else
            {
                auto const lhsLevel  = manager_.get_vertex_level(l);
                auto const rhsLevel  = manager_.get_vertex_level(r);
                auto const topLevel  = std::min(lhsLevel, rhsLevel);
                auto const topVertex = topLevel == lhsLevel ? l : r;
                auto const topIndex  = topVertex->get_index();
                auto const domain    = this->get_domain(topIndex);
                auto const sons      = utils::fill_array_n<P>(domain, [=, &go](auto const i)
                {
                    auto const first  = lhsLevel == topLevel ? l->get_son(i) : l;
                    auto const second = rhsLevel == topLevel ? r->get_son(i) : r;
                    return go(go, first, second);
                });

                u = manager_.internal_vertex(topIndex, sons);
            }

            manager_.template cache_put<Op<P>>(cacheIterator, l, r, u);
            return u;
        };

        auto const v = go(go, lhs.get_root(), rhs.get_root());
        auto const d = mdd_t(v);
        manager_.adjust_sizes();
        return d;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::restrict_var
        (mdd_t const& d, index_t const i, log_t const val) -> mdd_t
    {
        return this->transform(d, [=, this](auto&& l_this, auto const v)
        {
            auto const domain = this->get_domain(v->get_index());
            if (v->get_index() == i)
            {
                // We make redundant vertex that will be handled by the vertex manager.
                return utils::fill_array_n<P>(domain, [son = v->get_son(val)](auto const)
                {
                    return son;
                });
            }
            else
            {
                // Nothing to restrict here so we just continue downwards.
                return utils::fill_array_n<P>(domain, [this, &l_this, v](auto const i)
                {
                    return this->transform_step(l_this, v->get_son(i));
                });
            }
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::booleanize
        (mdd_t const& d) -> mdd_t
    {
        return this->apply<GREATER_EQUAL>(d, this->constant(1));
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<template<std::size_t> class Op>
    auto mdd_manager<VertexData, ArcData, P>::left_fold
        (mdd_v const& ds) -> mdd_t
    {
        return this->left_fold<Op>(std::begin(ds), std::end(ds));
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<template<std::size_t> class Op>
    auto mdd_manager<VertexData, ArcData, P>::tree_fold
        (mdd_v& ds) -> mdd_t
    {
        return this->tree_fold<Op>(std::begin(ds), std::end(ds));
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<template<std::size_t> class Op, class InputIt>
    auto mdd_manager<VertexData, ArcData, P>::left_fold
        (InputIt first, InputIt last) -> mdd_t
    {
        auto r = std::move(*first);
        ++first;

        while (first != last)
        {
            r = this->apply<Op>(r, *first);
            ++first;
        }

        return r;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<template<std::size_t> class Op, class RandomIt>
    auto mdd_manager<VertexData, ArcData, P>::tree_fold
        (RandomIt first, RandomIt last) -> mdd_t
    {
        auto const count      = std::distance(first, last);
        auto const numOfSteps = static_cast<std::size_t>(std::ceil(std::log2(count)));
        auto currentCount     = count;

        for (auto step = 0u; step < numOfSteps; ++step)
        {
            auto const justMoveLast = currentCount & 1;
            currentCount = (currentCount >> 1) + justMoveLast;
            auto const pairCount    = currentCount - justMoveLast;

            for (auto i = 0u; i < pairCount; ++i)
            {
                *(first + i) = this->apply<Op>( *(first + 2 * i)
                                              , *(first + 2 * i + 1) );
            }

            if (justMoveLast)
            {
                *(first + currentCount - 1) = std::move(*(first + 2 * (currentCount - 1)));
            }
        }

        return mdd_t {std::move(*first)};
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Transformator>
    auto mdd_manager<VertexData, ArcData, P>::transform
        (mdd_t const& d, Transformator&& transform_sons) -> mdd_t
    {
        auto const root = this->transform_step(transform_sons, d.get_root());
        transformMemo_.clear();
        return mdd_t {root};
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Transformator>
    auto mdd_manager<VertexData, ArcData, P>::transform_step
        (Transformator&& transform_sons, vertex_t* const v) -> vertex_t*
    {
        auto const memoIt = transformMemo_.find(v);
        if (transformMemo_.end() != memoIt)
        {
            return memoIt->second;
        }

        if (manager_.is_leaf_vertex(v))
        {
            return v;
        }

        auto const u = manager_.internal_vertex( v->get_index()
                                               , transform_sons(transform_sons, v) );
        transformMemo_.emplace(v, u);
        return u;
    }
}