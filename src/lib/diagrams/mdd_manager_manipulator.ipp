#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include <cmath>
#include <vector>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    template<class Op>
    auto mdd_manager<VertexData, ArcData, P>::apply
        (mdd_t const& lhs, Op op, mdd_t const& rhs) -> mdd_t
    {
        auto const ret = this->apply_step(lhs.get_root(), op, rhs.get_root());
        // TODO if not memoize then clear apply memo
        return mdd_t {ret};
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::restrict_var
        (mdd_t const& d, index_t const i, log_t const val) -> mdd_t
    {
        return this->transform(T_RESTRICT, d, [i, val](auto const v)
        {
            return v->get_index() == i ? v->get_son(val) : v;
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Op>
    auto mdd_manager<VertexData, ArcData, P>::left_fold
        (mdd_v mdds, Op op) -> mdd_t
    {
        return this->left_fold(std::begin(mdds), std::end(mdds), op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Op>
    auto mdd_manager<VertexData, ArcData, P>::tree_fold
        (mdd_v mdds, Op op) -> mdd_t
    {
        return this->tree_fold(std::begin(mdds), std::end(mdds), op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class InputIt, class Op>
    auto mdd_manager<VertexData, ArcData, P>::left_fold
        (InputIt first, InputIt last, Op op) -> mdd_t
    {
        auto r = std::move(*first);
        ++first;

        while (first != last)
        {
            r = this->apply(r, op, *first);
            ++first;
        }

        return r;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class RandomIt, class Op>
    auto mdd_manager<VertexData, ArcData, P>::tree_fold
        (RandomIt first, RandomIt last, Op op) -> mdd_t
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
                *(first + i) = this->apply( *(first + 2 * i)
                                          , op
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
    template<class Op>
    auto mdd_manager<VertexData, ArcData, P>::apply_step
        (vertex_t* const lhs, Op op, vertex_t* const rhs) -> vertex_t*
    {
        auto const key    = apply_key_t {lhs, op_id(op), rhs};
        auto const memoIt = applyMemo_.find(key);
        if (applyMemo_.end() != memoIt)
        {
            return memoIt->second;
        }

        auto const lhsVal = vertexManager_.get_value(lhs); // TODO project value
        auto const rhsVal = vertexManager_.get_value(rhs);
        auto const opVal  = op(lhsVal, rhsVal);
        auto u = static_cast<vertex_t*>(nullptr);

        if (!is_nondetermined<P>(opVal))
        {
            u = vertexManager_.terminal_vertex(opVal);
        }
        else
        {
            auto const lhsLevel  = vertexManager_.get_level(lhs);
            auto const rhsLevel  = vertexManager_.get_level(rhs);
            auto const level     = std::min(lhsLevel, rhsLevel);
            auto const topVertex = level == lhsLevel ? lhs : rhs;
            auto const index     = topVertex->get_index();

            auto sons = son_a {};
            for (auto i = 0u; i < P; ++i)
            {
                auto const first  = lhsLevel == level ? lhs->get_son(i) : lhs;
                auto const second = rhsLevel == level ? rhs->get_son(i) : rhs;
                sons[i] = this->apply_step(first, op, second);
            }

            u = vertexManager_.internal_vertex(index, sons);
        }

        applyMemo_.emplace(key, u);
        return u;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Transformator>
    auto mdd_manager<VertexData, ArcData, P>::transform
        (trans_id_t const opId, mdd_t const& d, Transformator&& op) -> mdd_t
    {
        auto const root = this->transform_step(opId, d.get_root(), op);
        transformMemo_.clear(); // temporarily just like this
        return mdd_t {root};
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Transformator>
    auto mdd_manager<VertexData, ArcData, P>::transform_step
        (trans_id_t const opId, vertex_t* const v, Transformator&& op) -> vertex_t*
    {
        auto const key = transform_key_t {v, opId};
        auto const memoIt = transformMemo_.find(key);
        if (transformMemo_.end() != memoIt)
        {
            return memoIt->second;
        }

        auto const transformedV = op(v);
        auto newV = static_cast<vertex_t*>(nullptr);
        if (v != transformedV)
        {
            newV = transformedV;
        }
        else if (this->vertexManager_.is_leaf(v))
        {
            newV = v;
        }
        else
        {
            auto sons = son_a {};
            for (auto i = 0u; i < P; ++i)
            {
                sons[i] = this->transform_step(opId, v->get_son(i), op);
            }
            newV = vertexManager_.internal_vertex(v->get_index(), sons);
        }

        transformMemo_.emplace(key, newV);

        return newV;
    }
}