#ifndef MIX_DD_MDD_MANIPULATOR_HPP
#define MIX_DD_MDD_MANIPULATOR_HPP

#include "bdd.hpp"
#include "operators_static_check.hpp"
#include "../utils/hash.hpp"

#include <iterator>
#include <algorithm>
#include <iterator>
#include <functional>
#include <utility>
#include <type_traits>
#include <set>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    class mdd_manipulator
    {
    public:
        using mdd_t    = std::conditional_t< 2 == P
                                           , bdd<VertexData, ArcData, Allocator>
                                           , mdd<VertexData, ArcData, P, Allocator> >;
        using vertex_t = typename mdd_t::vertex_t;
        using arc_t    = typename mdd_t::arc_t;
        using log_t    = typename mdd_t::log_t;

    public:
        explicit mdd_manipulator (Allocator const& = Allocator {});

    public:
        template<class BinOp> auto apply (mdd_t&&      d1, BinOp op, mdd_t&&      d2) -> mdd_t;
        template<class BinOp> auto apply (mdd_t const& d1, BinOp op, mdd_t&&      d2) -> mdd_t;
        template<class BinOp> auto apply (mdd_t&&      d1, BinOp op, mdd_t const& d2) -> mdd_t;
        template<class BinOp> auto apply (mdd_t const& d1, BinOp op, mdd_t const& d2) -> mdd_t;

        auto restrict_var (mdd_t&  diagram, index_t const i, log_t const val) -> mdd_t&;
        auto restrict_var (mdd_t&& diagram, index_t const i, log_t const val) -> mdd_t;

        template<class BinOp>
        auto concat (std::vector<mdd_t> ds, BinOp op) -> mdd_t;

        auto reduce (mdd_t&  diagram) -> mdd_t&;
        auto reduce (mdd_t&& diagram) -> mdd_t;

    private:
        using leaf_val_map       = typename mdd_t::leaf_val_map;
        using val_leaf_arr       = std::array<vertex_t*, log_val_traits<P>::valuecount>;
        using son_arr            = typename vertex_t::star_arr;
        using recursion_key_t    = std::pair<vertex_t const*, vertex_t const*>;
        using in_graph_key_t     = std::array<index_t, P + 1>;
        using recursion_memo_map = std::unordered_map<recursion_key_t, vertex_t*, utils::tuple_hash_t<recursion_key_t>>;
        using in_graph_memo_map  = std::unordered_map<in_graph_key_t,  vertex_t*, utils::tuple_hash_t<in_graph_key_t>>;
        using manager_t          = utils::alloc_manager<Allocator>;

    private:
        template<class BinOp>
        auto apply_step ( vertex_t* const v1
                        , BinOp                 op
                        , vertex_t* const v2 ) -> vertex_t*;

        auto terminal_vertex ( log_t   const  val )  -> vertex_t*;
        auto internal_vertex ( index_t const  index
                             , son_arr const& arcs ) -> vertex_t*;

        auto value1 (vertex_t* const v1) const -> log_t;
        auto value2 (vertex_t* const v2) const -> log_t;
        auto index1 (vertex_t const* const v1) const -> index_t;
        auto index2 (vertex_t const* const v2) const -> index_t;

        auto leaf_index  () const   -> index_t;
        auto recycle     (mdd_t& d) -> void;
        auto apply_reset ()         -> void;

        static auto is_redundant (vertex_t const* const v ) -> bool;

    protected:
        leaf_val_map       leafToVal_;
        val_leaf_arr       valToLeaf_;
        recursion_memo_map recursionMemo_;
        in_graph_memo_map  inGraphMemo_;
        id_t               nextId_;
        mdd_t const*       diagram1_;
        mdd_t const*       diagram2_;
        manager_t          manager_;
    };

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    mdd_manipulator<VertexData, ArcData, P, Allocator>::mdd_manipulator (Allocator const& alloc) :
        valToLeaf_ {{nullptr}},
        nextId_    {0},
        diagram1_  {nullptr},
        diagram2_  {nullptr},
        manager_   {alloc}
    {
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class BinOp>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::apply
        (mdd_t&& d1, BinOp op, mdd_t&& d2) -> mdd_t
    {
        auto newDiagram = this->apply(d1, op, d2);
        this->recycle(d1);
        this->recycle(d2);
        return newDiagram;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class BinOp>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::apply
        (mdd_t&& d1, BinOp op, const mdd_t& d2) -> mdd_t
    {
        auto newDiagram = this->apply(d1, op, d2);
        this->recycle(d1);
        return newDiagram;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class BinOp>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::apply
        (const mdd_t& d1, BinOp op, mdd_t&& d2) -> mdd_t
    {
        auto newDiagram = this->apply(d1, op, d2);
        this->recycle(d2);
        return newDiagram;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class BinOp>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::apply
        (mdd_t const& d1, BinOp op, mdd_t const& d2) -> mdd_t
    {
        static_assert(check_op_v<P, BinOp>, "Operator P doesn't mach manipulator P.");

        diagram1_ = &d1;
        diagram2_ = &d2;

        auto newDiagram = mdd_t { this->apply_step(d1.root_, op, d2.root_)
                                , std::move(leafToVal_)
                                , manager_.get_alloc() };
        this->apply_reset();

        return newDiagram;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::restrict_var
        (mdd_t& diagram, index_t const i, log_t const val) -> mdd_t&
    {
        using vertex_set = std::set<vertex_t*>;

        if (i >= diagram.variable_count())
        {
            return diagram;
        }

        auto const oldVertices = diagram.template fill_container<vertex_set>();

        // "skip" all vertices with given index
        diagram.traverse_pre(diagram.root_, [i, val, &diagram](auto const v)
        {
            if (diagram.is_leaf(v))
            {
                return;
            }

            for (auto j = 0u; j < P; ++j)
            {
                if (!diagram.is_leaf(v->son(j)) && i == v->son(j)->index)
                {
                    v->son(j) = v->son(j)->son(val);
                }
            }
        });

        // possibly change the root
        if (i == diagram.root_->index)
        {
            diagram.root_ = diagram.root_->son(val);
        }

        // identify now unreachable vertices
        auto const newVertices   = diagram.template fill_container<vertex_set>();
        auto unreachableVertices = std::vector<vertex_t*>();
        std::set_difference( std::begin(oldVertices), std::end(oldVertices)
                           , std::begin(newVertices), std::end(newVertices)
                           , std::inserter(unreachableVertices, std::begin(unreachableVertices)) );

        // and release them
        for (auto v : unreachableVertices)
        {
            if (diagram.is_leaf(v))
            {
                diagram.leafToVal_.erase(v);
            }
            manager_.release(v);
        }

        return this->reduce(diagram);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::restrict_var
        (mdd_t&& diagram, index_t const i, log_t const val) -> mdd_t
    {
        return mdd_t(std::move(this->restrict_var(diagram, i, val)));
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::reduce
        (mdd_t&& diagram) -> mdd_t
    {
        return mdd_t {std::move(this->reduce(diagram))};
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::reduce
        (mdd_t& diagram) -> mdd_t&
    {
        using vertex_key_t     = std::array<id_t, P>;
        auto const levels      = diagram.fill_levels();
        auto redundantVertices = std::vector<vertex_t*> {};
        auto newDiagramMap     = std::unordered_map<id_t, vertex_t*>();
        auto nextId            = static_cast<id_t>(0);

        auto make_leaf_key = [](auto val)
        {
            auto arr = vertex_key_t {val};
            std::fill(std::next(std::begin(arr)), std::end(arr), -1);
            return arr;
        };

        auto make_internal_key = [](auto v)
        {
            auto arr = vertex_key_t {};
            std::transform( std::begin(v->forwardStar), std::end(v->forwardStar), std::begin(arr)
                          , [](auto const& arc) { return arc.target->id; } );
            return arr;
        };

        for (auto i = levels.size(); i > 0;)
        {
            --i;
            auto keyedVertices = std::vector<std::pair<vertex_key_t, vertex_t*>>();

            for (auto const u : levels[i])
            {
                if (diagram.is_leaf(u))
                {
                    keyedVertices.emplace_back(make_leaf_key(diagram.value(u)), u);
                }
                else if (mdd_manipulator::is_redundant(u))
                {
                    u->id = u->son(0)->id;
                    redundantVertices.emplace_back(u);
                }
                else
                {
                    keyedVertices.emplace_back(make_internal_key(u), u);
                }
            }

            std::sort(std::begin(keyedVertices), std::end(keyedVertices));

            auto oldKey = make_leaf_key(-1);

            for (auto& [key, u] : keyedVertices)
            {
                if (key == oldKey)
                {
                    u->id = nextId;
                    redundantVertices.emplace_back(u);
                    if (diagram.is_leaf(u))
                    {
                        diagram.leafToVal_.erase(u);
                    }
                }
                else
                {
                    nextId++;
                    u->id = nextId;

                    newDiagramMap.emplace(nextId, u);

                    if (! diagram.is_leaf(u))
                    {
                        for (auto j = 0u; j < P; ++j)
                        {
                            u->son(j) = newDiagramMap.at(u->son(j)->id);
                        }
                    }

                    oldKey = key;
                }
            }
        }

        diagram.root_ = newDiagramMap.at(diagram.root_->id);

        for (auto const v : redundantVertices)
        {
            manager_.release(v);
        }

        return diagram;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class BinOp>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::apply_step 
        (vertex_t* const v1, BinOp op, vertex_t* const v2) -> vertex_t*
    {
        auto const uit = recursionMemo_.find(recursion_key_t {v1, v2});
        if (uit != recursionMemo_.end())
        {
            return (*uit).second;
        }

        auto const val = op(this->value1(v1), this->value2(v2));
        auto       u   = static_cast<vertex_t*>(nullptr);

        if (!is_nondetermined<P>(val))
        {
            u = this->terminal_vertex(val);
        }
        else
        {
            auto       arcs  = son_arr {};
            auto const index = std::min(this->index1(v1), this->index2(v2));

            for (auto i = 0u; i < P; ++i)
            {
                auto const first  = this->index1(v1) == index ? v1->son(i) : v1;
                auto const second = this->index2(v2) == index ? v2->son(i) : v2;

                arcs[i].target = this->apply_step(first, op, second);
            }

            u = this->internal_vertex(index, arcs);
        }

        recursionMemo_.emplace( std::piecewise_construct
                              , std::make_tuple(v1, v2)
                              , std::make_tuple(u) );

        return u;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::terminal_vertex
        (log_t const val) -> vertex_t*
    {
        if (! valToLeaf_[val])
        {
            valToLeaf_[val] = manager_.create(nextId_++, this->leaf_index());
            leafToVal_.emplace(valToLeaf_[val], val);
        }

        return valToLeaf_[val];
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::internal_vertex
        (index_t const index, son_arr const& arcs) -> vertex_t*
    {
        // TODO this->is_redundant()?
        if (std::all_of( std::begin(arcs), std::end(arcs)
                       , [fid = arcs[0].target->id] (auto const& arc)
                         { return arc.target->id == fid; } ))
        {
            return arcs[0].target;
        }

        auto key = in_graph_key_t {index};
        std::transform( std::begin(arcs), std::end(arcs), std::next(std::begin(key))
                      , [](auto&& a) { return a.target->id; } );

        auto const inGraphIt = inGraphMemo_.find(key);
        if (inGraphIt != inGraphMemo_.end())
        {
            return inGraphIt->second;
        }

        auto const newVertex = manager_.create(nextId_++, index, arcs);
        inGraphMemo_.emplace(key, newVertex);

        return newVertex;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::index1
        (vertex_t const* const v1) const -> index_t
    {
        return diagram1_->is_leaf(v1) ? this->leaf_index()
                                      : v1->index;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::index2
        (vertex_t const* const v2) const -> index_t
    {
        return diagram2_->is_leaf(v2) ? this->leaf_index()
                                      : v2->index;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::value1
        (vertex_t* const v1)  const -> log_t
    {
        return diagram1_->value(v1);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::value2
        (vertex_t* const v2)  const -> log_t
    {
        return diagram2_->value(v2);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::is_redundant
        (vertex_t const* const v) -> bool
    {
        return std::all_of( std::begin(v->forwardStar), std::end(v->forwardStar)
                          , [fid = v->son(0)->id] (auto&& arc) 
                            { return arc.target->id == fid; } );
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::leaf_index
        () const -> index_t
    {
        return std::max( diagram1_->leaf_index()
                       , diagram2_->leaf_index() );
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::recycle
        (mdd_t& d) -> void
    {
        d.clear(); // TODO check performance

        // if (! d.root_)
        // {
        //     return;
        // }

        // d.traverse_pre(d.root_, [this](vertex_t* const v) 
        // {
        //     this->manager_.release(v);
        // });

        // d.root_ = nullptr;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_manipulator<VertexData, ArcData, P, Allocator>::apply_reset
        () -> void
    {
        leafToVal_.clear();
        recursionMemo_.clear();
        inGraphMemo_.clear();

        std::fill(std::begin(valToLeaf_), std::end(valToLeaf_), nullptr);

        diagram1_ = nullptr;
        diagram2_ = nullptr;
        nextId_   = 0;
    }
}

#endif