#ifndef _MIX_DD_BDD_CREATOR_ALT_
#define _MIX_DD_BDD_CREATOR_ALT_

#include <vector>
#include <unordered_map>
#include <array>
#include <cmath>
#include <utility>
#include <iterator>
#include "bdd.hpp"
#include "bool_function.hpp"
#include "../dd/typedefs.hpp"
#include "../dd/graph.hpp"
#include "../utils/math_utils.hpp"
#include "../utils/hash.hpp"
#include "../data_structures/double_top_stack.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData>
    class bdd_creator_alt
    {
    private:
        using bdd_t           = bdd<VertexData, ArcData>;
        using vertex_t        = typename bdd_t::vertex_t;
        using arc_t           = typename bdd_t::arc_t;
        using leaf_val_map_t  = typename bdd_t::leaf_val_map;
        using vertex_key      = std::pair<vertex_t*, vertex_t*>;
        using level_map       = std::unordered_map<const vertex_key, vertex_t*, utils::tuple_hash_t<const vertex_key>>;

        struct stack_frame
        {
            vertex_t* vertexPtr;
            size_t    level;
        };

    private:

        utils::double_top_stack<stack_frame> stack;
        std::vector<level_map> levels;
        id_t nextId {0};

    public:
        template< class BoolFunction
                , class GetFunctionValue  = get_f_val_r<BoolFunction>
                , class VarCount = var_count<BoolFunction> >
        auto create_from (const BoolFunction& in) -> bdd_t;

    private:
        auto try_insert ( const vertex_key key
                        , const index_t level ) -> vertex_t*; 

        auto reset () -> void;
    };
    
    template<class VertexData, class ArcData>
    template<class BoolFunction, class GetFunctionValue, class VarCount>
    auto bdd_creator_alt<VertexData, ArcData>::create_from
        (const BoolFunction& in) -> bdd_t
    {
        const index_t leafLevel  {VarCount {} (in)};
        const var_vals_t maxInput 
        {
            utils::two_pow(static_cast<var_vals_t>(VarCount {} (in)))
        };

        this->levels.resize(leafLevel + 1);

        std::array<vertex_t*, 2> valToLeaf 
        {
            new vertex_t {nextId++, leafLevel}
          , new vertex_t {nextId++, leafLevel}
        };

        leaf_val_map_t leafToVal 
        { 
            {valToLeaf[0], 0}
          , {valToLeaf[1], 1} 
        };

        var_vals_t varVals {0};
        while (varVals < maxInput)
        {
            const bool_t currInputVal {GetFunctionValue {} (in, varVals)};
            const bool_t nextInputVal {GetFunctionValue {} (in, varVals + 1)};

            vertex_t* son {nullptr};

            if (currInputVal == nextInputVal)
            {
                son = valToLeaf[currInputVal];
            }
            else
            {
                vertex_t* const negativeTarget {valToLeaf[currInputVal]};
                vertex_t* const positiveTarget {valToLeaf[nextInputVal]};

                son = this->try_insert(vertex_key {negativeTarget, positiveTarget}, leafLevel - 1);
            }
            
            stack.emplace(son, leafLevel - 1);
            
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

                    stack.emplace(v, stackLevel - 1);
                }
                else
                {
                    vertex_t* const negativeTarget {stack.under_top().vertexPtr};
                    vertex_t* const positiveTarget {stack.top().vertexPtr};
                    
                    stack.pop();
                    stack.pop();

                    son = this->try_insert(vertex_key {negativeTarget, positiveTarget}, stackLevel - 1);

                    stack.emplace(son, stackLevel - 1);
                }
            }

            varVals += 2;
        }

        vertex_t* root {stack.top().vertexPtr};
        this->reset();

        return bdd_t {
            root
          , VarCount {} (in)
          , std::move(leafToVal)
        };
    }
 
    template<class VertexData, class ArcData>
    auto bdd_creator_alt<VertexData, ArcData>::try_insert 
        (const vertex_key key, const index_t level) -> vertex_t*
    {
        auto inGraphIt {levels[level].find(key)};
        if (inGraphIt != levels[level].end())
        {
            return (*inGraphIt).second;            
        }

        auto newVertex 
        {
            new vertex_t {
                this->nextId++
                , level
                , {arc_t {key.first}, arc_t {key.second}}
            }
        };
        levels[level].emplace(key, newVertex);
        
        return newVertex;
    }

    template<class VertexData, class ArcData>
    auto bdd_creator_alt<VertexData, ArcData>::reset
        () -> void
    {
        this->levels.clear();
        this->stack.clear();
        this->nextId = 0;
    }
}

#endif