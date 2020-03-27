#ifndef MIX_DD_MDD_
#define MIX_DD_MDD_

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include "graph.hpp"
#include "typedefs.hpp"
#include "../utils/io.hpp"
#include "../utils/string_utils.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData, size_t P> class mdd;
    template<class VertexData, class ArcData, size_t P> class mdd_creator;
    template<class VertexData, class ArcData, size_t P> class mdd_manipulator;

    template<class VertexData, class ArcData, size_t P>
    class mdd
    {
    public:
        using vertex_t = vertex<VertexData, ArcData, P>;
        using log_t    = typename log_val_traits<P>::value_t;

        friend class mdd_creator<VertexData, ArcData, P>;
        friend class mdd_manipulator<VertexData, ArcData, P>;

    public:
        mdd  ()           = default;
        mdd  (const mdd&) = delete;
        mdd  (mdd&&);
        ~mdd ();

        auto to_dot_graph ()                const -> std::string;
        auto get_root     ()                const -> vertex_t*;
        auto get_leaf     (const log_t val) const -> vertex_t*;

    private:
        using leaf_val_map = std::map<const vertex_t*, log_t>;

    private:
        mdd ( vertex_t* const pRoot
            , const index_t   pVariableCount
            , leaf_val_map    pLeafToVal );

        auto fill_levels ()                  const -> std::vector< std::vector<vertex_t*> >;
        auto leaf_index  ()                  const -> index_t;
        auto is_leaf     (const vertex_t* v) const -> bool;

        template<class UnaryFunction>
        auto traverse (vertex_t* const v, UnaryFunction f) const -> void;
    
    private:
        leaf_val_map leafToVal_;
        vertex_t*    root_          {nullptr};
        index_t      variableCount_ {0};
    };    

    template<class VertexData, class ArcData, size_t P>
    mdd<VertexData, ArcData, P>::mdd ( mdd&& other ) :
        leafToVal_     (std::move(other.leafToVal_))
      , root_          (other.root_)
      , variableCount_ (other.variableCount_)
    {
        other.root_          = nullptr;
        other.variableCount_ = 0;
    }

    template<class VertexData, class ArcData, size_t P>
    mdd<VertexData, ArcData, P>::mdd ( vertex_t* const root
                                     , const index_t   variableCount
                                     , leaf_val_map    leafToVal ) :
        leafToVal_     (std::move(leafToVal))
      , root_          (root)
      , variableCount_ (variableCount)
    {
    }

    template<class VertexData, class ArcData, size_t P>
    mdd<VertexData, ArcData, P>::~mdd()
    {
        for (const auto& level : this->fill_levels())
        {
            for (const auto v : level)
            {
                delete v;
            }
        }
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd<VertexData, ArcData, P>::to_dot_graph
        () const -> std::string
    {
        using std::to_string;
        using utils::concat;
        using utils::concat_range;
        using utils::EOL;

        std::vector<std::string> labels;
        std::vector<std::string> arcs;
        std::vector<std::string> ranks;
        std::vector<std::string> squareShapes;

        auto make_label = [this](vertex_t* const v)
        {
            using std::to_string;
            return v->index == this->leaf_index() ? to_string(this->leafToVal_.at(v))
                                                  : "x" + to_string(v->index);
        };

        for (const auto& level : this->fill_levels())
        {
            if (level.empty())
            {
                continue;
            }

            std::vector<std::string> ranksLocal {"{rank = same;"};
            for (const auto v : level)
            {
                labels.emplace_back(concat(v->id , " [label = " , make_label(v) , "];"));
                ranksLocal.emplace_back(concat(v->id , ";"));

                if (! this->is_leaf(v))
                {
                    for (auto val (0u); val < P; ++val)
                    {
                        arcs.emplace_back(concat(v->id , " -> " , v->son(val)->id , " [label = \"" , val , "\"];"));
                    }
                }
            }

            ranksLocal.emplace_back("}");
            ranks.emplace_back(concat_range(ranksLocal, " "));
        }

        for (auto val (0u); val < P; ++val)
        {
            squareShapes.emplace_back(to_string(this->get_leaf(val)->id));
        }
        squareShapes.emplace_back(";");
        
        return concat( "digraph D {"                                                      , EOL
                     , "    " , "node [shape = square] ", concat_range(squareShapes, " ") , EOL
                     , "    " , "node [shape = circle];"                                  , EOL , EOL
                     , "    " , concat_range(labels, concat(EOL, "    "))                 , EOL , EOL
                     , "    " , concat_range(arcs,   concat(EOL, "    "))                 , EOL , EOL
                     , "    " , concat_range(ranks,  concat(EOL, "    "))                 , EOL
                     , "}"                                                                , EOL );
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd<VertexData, ArcData, P>::get_leaf
        (const log_t val) const -> vertex_t*
    {
        // TODO consider storing val_to_leaf explicitly
        // and also finally implement list map....
        for (auto& [leaf, leafVal] : leafToVal_)
        {
            if (leafVal == val)
            {
                return const_cast<vertex_t*>(leaf);
            }
        }

        return nullptr;
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd<VertexData, ArcData, P>::fill_levels
        () const -> std::vector< std::vector<vertex_t*> >
    {
        std::vector< std::vector<vertex_t*> > levels(variableCount_ + 1);

        if (root_)
        {
            this->traverse(root_, [&levels](vertex_t* const v) 
            {
                levels[v->index].push_back(v);
            });
        }

        return levels;
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd<VertexData, ArcData, P>::leaf_index
        () const -> index_t
    {
        return variableCount_;
    }

    template<class VertexData, class ArcData, size_t P>
    auto mdd<VertexData, ArcData, P>::is_leaf
        (const vertex_t* v) const -> bool
    {
        return v->index == this->leaf_index();
    }

    template<class VertexData, class ArcData, size_t P>
    template<class UnaryFunction>
    auto mdd<VertexData, ArcData, P>::traverse
        (vertex_t* const v, UnaryFunction f) const -> void
    {
        // note: This procedure could be implemented non-recursively.
        // In that case there would be no copies of UnaryFunction.
        // On the other hand, stack or something similar would be neccessary
        // to implement the traversal algorithm. For now I have decided that
        // this option would cost more than simple recursion. Keep in mind that
        // depth of the recursion is limited by the height of the diagram.
        //
        // But it might be wise not to capture expensive-to-copy objects in @p f.
        
        v->mark = ! v->mark;

        f(v);

        for (size_t i (0u); i < P; ++i)
        {
            if (! this->is_leaf(v) && v->mark != v->son(i)->mark)
            {
                this->traverse(v->son(i), f);
            }
        }
    }
}

#endif