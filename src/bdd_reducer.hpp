#ifndef MIX_DD_BDD_REDUCER
#define MIX_DD_BDD_REDUCER

#include <vector>
#include <utility>
#include <algorithm>
#include <map>
#include "bdd.hpp"
#include "typedefs.hpp"

namespace mix::dd
{
    template<class VertexData = def_vertex_data_t
           , class ArcData = def_arc_data_t>
    class bdd_reducer
    {
    private:
        using bdd_t           = bdd<VertexData, ArcData>;
        using arc             = typename graph<VertexData, ArcData>::arc;
        using vertex          = typename graph<VertexData, ArcData>::vertex;
        using key_vertex_pair = std::pair<std::pair<id_t, id_t>, vertex*>;

        std::vector<std::vector<vertex*>> levels;
        std::map<id_t, vertex*> subgraph;
        size_t nextId {0};

    public:
        auto reduce (bdd_t& diagram) -> void;

    private:
        auto fill_levels (const bdd_t& diagram) -> void;

        auto reset () -> void;
    };    

    template<class VertexData, class ArcData>
    auto bdd_reducer<VertexData, ArcData>::reduce
        (bdd_t& diagram) -> void
    {
        this->fill_levels(diagram);

        std::vector<vertex*> notUsedAnymore;

        for (size_t i {levels.size()}; i > 0;)
        {
            --i;
            std::vector<key_vertex_pair> keyedVertices;
            
            for (vertex* u : this->levels[i])
            {
                if (diagram.is_leaf(u))
                {
                    keyedVertices.push_back(
                        std::make_pair(
                            std::make_pair(diagram.value(u), -1)
                          , u
                        )
                    );
                }
                else if (bdd_t::low(u)->id == bdd_t::high(u)->id)
                {
                    u->id = bdd_t::low(u)->id;
                    notUsedAnymore.push_back(u);
                }
                else
                {
                    keyedVertices.push_back(
                        std::make_pair(
                            std::make_pair(bdd_t::low(u)->id, bdd_t::high(u)->id)
                          , u
                        )
                    );
                }
            }

            // TODO linear bucket sort
            std::sort(keyedVertices.begin(), keyedVertices.end());

            auto oldKey {std::make_pair(-1, -1)};

            for (auto& kvPair : keyedVertices)
            {
                auto& key {kvPair.first};
                vertex* u {kvPair.second};
                if (key == oldKey)
                {
                    u->id = this->nextId;
                    notUsedAnymore.push_back(u);
                }
                else
                {
                    ++this->nextId;
                    u->id = this->nextId;

                    subgraph[this->nextId] = u;
                    
                    if (! diagram.is_leaf(u))
                    {
                        u->forwardStar[0].target = this->subgraph[bdd_t::low(u)->id];
                        u->forwardStar[1].target = this->subgraph[bdd_t::high(u)->id];
                    }

                    oldKey = key;
                }
            }
        }
        
        // TODO moc velka zlozitost, konzultovat explicitne ulozenie value
        for (vertex* v : notUsedAnymore)
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
        levels.resize(diagram.variableCount + 2);

        diagram.traverse(diagram.root, [this](vertex* const v) {
            levels[v->level].push_back(v);
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