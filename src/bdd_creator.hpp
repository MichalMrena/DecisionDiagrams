#ifndef _MIX_DD_BDD_CREATOR_
#define _MIX_DD_BDD_CREATOR_

#include <vector>
#include <unordered_map>
#include <cmath>
#include "typedefs.hpp"
#include "bool_function.hpp"
#include "utils/math_utils.hpp"
#include "utils/double_top_stack.hpp"
#include "utils/hash.hpp"
#include "graph.hpp"
#include "bdd.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData>
    class bdd_creator
    {
    private:
        using vertex_t = vertex<VertexData, ArcData, 2>;
        using arc_t    = vertex<VertexData, ArcData, 2>;
        
        struct stack_frame
        {
            vertex_t* vertexPtr;
            size_t level;
        };

        struct vertex_key // TODO replace with vertex pair from graph
        {
            vertex_t* negative;
            vertex_t* positive;

            auto operator== (const vertex_key& rhs) const -> bool;
        };
        
        struct vertex_key_hash
        {
            auto operator() (const vertex_key& key) const -> size_t;
        };

    private:
        using level_map = std::unordered_map<vertex_key, vertex_t*, vertex_key_hash>;

        utils::double_top_stack<stack_frame> stack;
        std::vector<level_map> levels;
        id_t nextId {1};

    public:
        auto create_diagram (const bool_function& input) -> bdd<VertexData, ArcData>;
        // TODO ak by to malo byť rýchle bool_function by nemusel byť virtuálny ale dal by sa dať ako template

    private:
        auto try_insert (const vertex_key key
                       , const index_t level) -> vertex_t*; 

        auto reset () -> void;
    };

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::create_diagram 
        (const bool_function& input) -> bdd<VertexData, ArcData>
    {
        const index_t leafLevel  {static_cast<index_t>(input.variable_count() + 1)};
        const size_t inputsCount {utils::pow(2UL, input.variable_count())};
        // TODO mocnia 2 ide este rychlejsie shiftom...

        this->levels.resize(leafLevel + 1);

        // TODO map interface ano ale za tým by stačil list
        std::map<log_val_t, vertex_t*> valToLeaf 
        { 
            {0, new vertex_t {nextId++, leafLevel}}
          , {1, new vertex_t {nextId++, leafLevel}} 
        };
        std::map<const vertex_t*, log_val_t> leafToVal 
        { 
            {valToLeaf[0], 0}
          , {valToLeaf[1], 1} 
        };

        size_t inputIndex {0};
        while (inputIndex < inputsCount)
        {
            const log_val_t currInputVal {input[inputIndex]};
            const log_val_t nextInputVal {input[inputIndex + 1]};

            vertex_t* son {nullptr};

            if (currInputVal == nextInputVal)
            {
                son = valToLeaf[currInputVal];
            }
            else
            {
                // TODO 01 alebo 10 je to to isté ?
                vertex_t* const negativeTarget {valToLeaf[currInputVal]};
                vertex_t* const positiveTarget {valToLeaf[nextInputVal]};

                son = this->try_insert(vertex_key {negativeTarget, positiveTarget}, leafLevel - 1);
            }
            
            stack.push(stack_frame {son, leafLevel - 1});
            
            while (stack.size() > 1)
            {
                if (stack.top().level != stack.under_top().level)
                {
                    break;
                }

                const size_t stackLevel {stack.top().level};

                if (stack.top().vertexPtr == stack.under_top().vertexPtr)
                {
                    vertex_t* const v {stack.top().vertexPtr};

                    stack.pop();
                    stack.pop();

                    stack.push(stack_frame {v, stackLevel - 1});
                }
                else
                {
                    vertex_t* const negativeTarget {stack.under_top().vertexPtr};
                    vertex_t* const positiveTarget {stack.top().vertexPtr};
                    
                    stack.pop();
                    stack.pop();

                    son = this->try_insert(vertex_key {negativeTarget, positiveTarget}, stackLevel - 1);

                    stack.push(stack_frame {son, stackLevel - 1});
                }
            }

            inputIndex += 2;
        }

        vertex_t* root {stack.top().vertexPtr};
        this->reset();

        return bdd<VertexData, ArcData> {
            root
          , static_cast<index_t>(input.variable_count())
          , std::move(leafToVal)
        };
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::try_insert 
        (const vertex_key key, const index_t level) -> vertex_t*
    {
        // https://www.boost.org/doc/libs/1_71_0/libs/pool/doc/html/boost/object_pool.html
        // dalo by sa vyskúšať na alokovanie vrcholov

        auto inGraphIt {levels[level].find(key)};
        if (inGraphIt != levels[level].end())
        {
            return (*inGraphIt).second;            
        }

        auto newVertex {new vertex_t {
            this->nextId++
          , level
          , {arc {key.negative}, arc {key.positive}}
        }};
        levels[level][key] = newVertex;
        
        return newVertex;
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::reset
        () -> void
    {
        this->levels.clear();
        this->stack.clear();
        this->nextId = 1;
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::vertex_key::operator== 
        (const vertex_key& other) const -> bool
    {
        return this->negative == other.negative
            && this->positive == other.positive;
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::vertex_key_hash::operator() 
        (const vertex_key& key) const -> size_t
    {
        size_t seed {0};

        // utils::boost::hash_combine<vertex_t*, utils::pointer_hash<vertex>>(seed, key.negative);
        // utils::boost::hash_combine<vertex_t*, utils::pointer_hash<vertex>>(seed, key.positive);

        utils::boost::hash_combine<vertex_t*, std::hash<vertex_t*>>(seed, key.negative);
        utils::boost::hash_combine<vertex_t*, std::hash<vertex_t*>>(seed, key.positive);

        return seed;
    }
}

#endif