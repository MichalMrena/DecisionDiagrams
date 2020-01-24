#ifndef _MIX_DD_BDD_CREATOR_
#define _MIX_DD_BDD_CREATOR_

#include <vector>
#include <unordered_map>
#include <array>
#include <cmath>
#include <utility>
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
    class bdd_creator
    {
    private:
        using bdd_t          = bdd<VertexData, ArcData>;
        using vertex_t       = typename bdd_t::vertex_t;
        using arc_t          = typename bdd_t::arc_t;
        using leaf_val_map_t = typename bdd_t::leaf_val_map;
        
        struct stack_frame
        {
            vertex_t* vertexPtr;
            size_t    level;

            stack_frame( vertex_t* const pVertexPtr
                       , const size_t pLevel);
        };

        using vertex_key = std::pair<vertex_t*, vertex_t*>;

        struct vertex_key_hash
        {
            auto operator() (const vertex_key& key) const -> size_t;
        };

    private:
        using level_map = std::unordered_map<vertex_key, vertex_t*, vertex_key_hash>;

        utils::double_top_stack<stack_frame> stack;
        std::vector<level_map> levels;
        id_t nextId {0};

    public:
        template< class BoolFunction
                , class GetFVal  = get_f_val_r<BoolFunction>
                , class VarCount = var_count<BoolFunction> >
        auto create_from (const BoolFunction& in) -> bdd_t;

        template<class InputIt>
        auto create_product ( InputIt varValsBegin
                            , InputIt varValsEnd
                            , const bool_t fVal) -> bdd_t;

    private:
        auto try_insert ( const vertex_key key
                        , const index_t level) -> vertex_t*; 

        auto reset () -> void;
    };

    template<class VertexData, class ArcData>
    template<class BoolFunction, class GetFVal, class VarCount>
    auto bdd_creator<VertexData, ArcData>::create_from
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
            const bool_t currInputVal {GetFVal {} (in, varVals)};
            const bool_t nextInputVal {GetFVal {} (in, varVals + 1)};

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
    template<class InputIt>
    auto bdd_creator<VertexData, ArcData>::create_product
        (InputIt varValsBegin, InputIt varValsEnd, const bool_t fVal) -> bdd_t
    {
        if (0 == fVal)
        {
            return bdd_t::just_false();
        }

        const auto varCount  {varValsEnd - varValsBegin};
        const auto leafIndex {static_cast<index_t>(varCount)};
        index_t index  {0};
        id_t    nextId {0};

        std::vector<vertex_t*> relevantVariables;
        relevantVariables.reserve(varCount);

        auto varValsIt {varValsBegin};
        while (varValsIt != varValsEnd)
        {
            if (*varValsIt != X)
            {
                relevantVariables.push_back(new vertex_t {nextId++, index});
            }

            ++index;
            ++varValsIt;
        }

        if (relevantVariables.empty())
        {
            return bdd_t::just_true();
        }

        auto trueLeaf  {new vertex_t {nextId++, leafIndex}};
        auto falseLeaf {new vertex_t {nextId++, leafIndex}};

        auto b {relevantVariables.begin()};
        auto e {relevantVariables.end()};
        --e;

        while (b != e)
        {
            const auto v      {*b};
            const auto varVal {*(varValsBegin + v->index)};
            
            ++b;
            
            v->son(varVal)  = *b;
            v->son(!varVal) = falseLeaf;
        }

        const auto v      {*b};
        const auto varVal {*(varValsBegin + v->index)};

        v->son(varVal)  = trueLeaf;
        v->son(!varVal) = falseLeaf;
        
        leaf_val_map_t leafToVal 
        { 
            {trueLeaf,  1}
          , {falseLeaf, 0} 
        };

        return bdd_t 
        {
            relevantVariables.front()
          , static_cast<index_t>(varCount)
          , std::move(leafToVal)
        };
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::try_insert 
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
        // levels[level][key] = newVertex;
        levels[level].emplace(key, newVertex);
        
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
    bdd_creator<VertexData, ArcData>::stack_frame::stack_frame
        ( vertex_t* const pVertexPtr
        , const size_t pLevel) :
        vertexPtr {pVertexPtr}
      , level     {pLevel} 
    {
    }

    template<class VertexData, class ArcData>
    auto bdd_creator<VertexData, ArcData>::vertex_key_hash::operator() 
        (const vertex_key& key) const -> size_t
    {
        size_t seed {0};

        utils::boost::hash_combine<vertex_t*, utils::pointer_hash<vertex_t>>(seed, key.first);
        utils::boost::hash_combine<vertex_t*, utils::pointer_hash<vertex_t>>(seed, key.second);

        // utils::boost::hash_combine<vertex_t*, std::hash<vertex_t*>>(seed, key.first);
        // utils::boost::hash_combine<vertex_t*, std::hash<vertex_t*>>(seed, key.second);

        return seed;
    }
}

#endif