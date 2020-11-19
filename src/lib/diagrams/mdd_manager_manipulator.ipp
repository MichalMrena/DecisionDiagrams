#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include <cmath>
#include <vector>
#include <algorithm>

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
        return this->transform(d, [i, val, this](auto const v, auto&& l_this)
        {
            if (v->get_index() == i)
            {
                return utils::fill_array<P>([son = v->get_son(val)](auto const)
                {
                    return son;
                });
            }
            else
            {
                return utils::fill_array<P>([this, &l_this, v](auto const i)
                {
                    return this->transform_step(v->get_son(i), l_this);
                });
            }
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
        auto const memoKey = make_apply_key(lhs, op, rhs);
        auto const memoIt  = applyMemo_.find(memoKey);
        if (applyMemo_.end() != memoIt)
        {
            return memoIt->second;
        }

        auto const lhsVal = vertexManager_.get_terminal_value(lhs);
        auto const rhsVal = vertexManager_.get_terminal_value(rhs);
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
            auto const sons      = utils::fill_array<P>([=](auto const i)
            {
                auto const first  = lhsLevel == level ? lhs->get_son(i) : lhs;
                auto const second = rhsLevel == level ? rhs->get_son(i) : rhs;
                return this->apply_step(first, op, second);
            });

            u = vertexManager_.internal_vertex(index, sons);
        }

        applyMemo_.emplace(memoKey, u);
        return u;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Op>
    auto mdd_manager<VertexData, ArcData, P>::make_apply_key
        (vertex_t* const lhs, Op op, vertex_t* const rhs) -> apply_key_t
    {
        if constexpr (op_is_commutative(op))
        {
            return lhs < rhs ? apply_key_t {lhs, op_id(op), rhs}
                             : apply_key_t {rhs, op_id(op), lhs};
        }
        else
        {
            return apply_key_t {lhs, op_id(op), rhs};
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

        if (vertexManager_.is_leaf(v))
        {
            return v;
        }

        auto const u = vertexManager_.internal_vertex(v->get_index(), transform_sons(v, transform_sons));
        transformMemo_.emplace(v, u);
        return u;
    }
}