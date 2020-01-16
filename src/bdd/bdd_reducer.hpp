#ifndef _MIX_DD_BDD_REDUCER_
#define _MIX_DD_BDD_REDUCER_

#include <vector>
#include <utility>
#include <algorithm>
#include <map>
#include "bdd.hpp"
#include "../dd/typedefs.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData>
    class bdd_reducer
    {
    private:
        using bdd_t           = bdd<VertexData, ArcData>;
        using arc_t           = arc<VertexData, ArcData, 2>;
        using vertex_t        = vertex<VertexData, ArcData, 2>;
        using key_vertex_pair = std::pair< std::pair<id_t, id_t>, vertex_t*>;

    private:
        std::vector< std::vector<vertex_t*> > levels;
        std::map<id_t, vertex_t*> subgraph;
        id_t nextId {0};

    public:
        auto reduce (bdd_t& diagram) -> void;

    private:
        auto fill_levels (const bdd_t& diagram) -> void;
        auto reset       () -> void;
    };    

    template<class VertexData, class ArcData>
    auto bdd_reducer<VertexData, ArcData>::reduce
        (bdd_t& diagram) -> void
    {
        this->fill_levels(diagram);

        std::vector<vertex_t*> notUsedAnymore;

        for (size_t i {levels.size()}; i > 0;)
        {
            --i;
            std::vector<key_vertex_pair> keyedVertices;
            
            for (vertex_t* u : this->levels[i])
            {
                if (diagram.is_leaf(u))
                {
                    keyedVertices.emplace_back( std::make_pair(diagram.value(u), -1)
                                              , u );
                }
                else if (u->son(0)->id == u->son(1)->id)
                {
                    u->id = u->son(0)->id;
                    notUsedAnymore.push_back(u);
                }
                else
                {
                    keyedVertices.emplace_back( std::make_pair( u->son(0)->id
                                                              , u->son(1)->id)
                                              , u);
                }
            }

            // TODO linear bucket sort
            std::sort(keyedVertices.begin(), keyedVertices.end());

            auto oldKey {std::make_pair(-1, -1)};

            for (auto& [key, u] : keyedVertices)
            {
                if (key == oldKey)
                {
                    u->id = this->nextId;
                    notUsedAnymore.push_back(u);
                }
                else
                {
                    ++this->nextId;
                    u->id = this->nextId;

                    subgraph.emplace(this->nextId, u);
                    
                    if (! diagram.is_leaf(u))
                    {
                        u->son(0) = this->subgraph.at(u->son(0)->id);
                        u->son(1) = this->subgraph.at(u->son(1)->id);
                    }

                    oldKey = key;
                }
            }
        }
        
        // TODO moc velka zlozitost stacilo by listy prejst nejako individualne
        for (vertex_t* v : notUsedAnymore)
        {
            diagram.leafToVal.erase(v);
            delete v;
        }

        // TODO remove unused vertices MEMMORY leak !!
        this->reset();
    }

    template<class VertexData, class ArcData>
    auto bdd_reducer<VertexData, ArcData>::fill_levels
        (const bdd_t& diagram) -> void
    {
        levels.resize(diagram.variableCount + 1);

        diagram.traverse(diagram.root, [this](vertex_t* const v) {
            levels[v->index].push_back(v);
        });
    }

    template<class VertexData, class ArcData>
    auto bdd_reducer<VertexData, ArcData>::reset
        () -> void
    {
        this->levels.clear();
        this->subgraph.clear();
        this->nextId = 0;
    }
}

#endif