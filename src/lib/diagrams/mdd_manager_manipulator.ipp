#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include <cmath>
#include <vector>
#include <algorithm>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    template<template<std::size_t> class Op>
    auto mdd_manager<VertexData, ArcData, P>::apply
        (mdd_t const& lhs, mdd_t const& rhs) -> mdd_t
    {
        auto const ret = this->apply_step<Op>(lhs.get_root(), rhs.get_root());
        // TODO if not memoize then clear apply memo
        return mdd_t {ret};
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::restrict_var
        (mdd_t const& d, index_t const i, log_t const val) -> mdd_t
    {
        return this->transform(d, [=, this](auto const v, auto&& l_this)
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
                    return this->transform_step(v->get_son(i), l_this);
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
    template<template<std::size_t> class Op>
    auto mdd_manager<VertexData, ArcData, P>::apply_step
        (vertex_t* const lhs, vertex_t* const rhs) -> vertex_t*
    {
        auto const memoKey = make_apply_key<Op<P>>(lhs, rhs);
        auto const memoIt  = applyMemo_.find(memoKey);
        if (applyMemo_.end() != memoIt)
        {
            return memoIt->second;
        }

        auto const lhsVal = vertexManager_.get_vertex_value(lhs);
        auto const rhsVal = vertexManager_.get_vertex_value(rhs);
        auto const opVal  = Op<P> () (lhsVal, rhsVal);
        auto u = static_cast<vertex_t*>(nullptr); // TODO IILambda?

        if (!is_nondetermined<P>(opVal))
        {
            u = vertexManager_.terminal_vertex(opVal);
        }
        else
        {
            auto const lhsLevel  = vertexManager_.get_vertex_level(lhs);
            auto const rhsLevel  = vertexManager_.get_vertex_level(rhs);
            auto const topLevel  = std::min(lhsLevel, rhsLevel);
            auto const topVertex = topLevel == lhsLevel ? lhs : rhs;
            auto const topIndex  = topVertex->get_index();
            auto const domain    = this->get_domain(topIndex);
            auto const sons      = utils::fill_array_n<P>(domain, [=, this](auto const i)
            {
                auto const first  = lhsLevel == topLevel ? lhs->get_son(i) : lhs;
                auto const second = rhsLevel == topLevel ? rhs->get_son(i) : rhs;
                return this->apply_step<Op>(first, second);
            });

            u = vertexManager_.internal_vertex(topIndex, sons);
        }

        applyMemo_.emplace(memoKey, u);
        return u;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Op>
    auto mdd_manager<VertexData, ArcData, P>::make_apply_key
        (vertex_t* const lhs, vertex_t* const rhs) -> apply_key_t
    {
        if constexpr (op_is_commutative(Op()))
        {
            return lhs < rhs ? apply_key_t {lhs, op_id(Op()), rhs}
                             : apply_key_t {rhs, op_id(Op()), lhs};
        }
        else
        {
            return apply_key_t {lhs, op_id(Op()), rhs};
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Transformator>
    auto mdd_manager<VertexData, ArcData, P>::transform
        (mdd_t const& d, Transformator&& transform_sons) -> mdd_t
    {
        auto const root = this->transform_step(d.get_root(), transform_sons);
        transformMemo_.clear();
        return mdd_t {root};
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Transformator>
    auto mdd_manager<VertexData, ArcData, P>::transform_step
        (vertex_t* const v, Transformator&& transform_sons) -> vertex_t*
    {
        auto const memoIt = transformMemo_.find(v);
        if (transformMemo_.end() != memoIt)
        {
            return memoIt->second;
        }

        if (vertexManager_.is_leaf_vertex(v))
        {
            return v;
        }

        auto const u = vertexManager_.internal_vertex( v->get_index()
                                                     , transform_sons(v, transform_sons) );
        transformMemo_.emplace(v, u);
        return u;
    }
}