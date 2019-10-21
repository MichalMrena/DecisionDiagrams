#ifndef MIX_DD_BIN_DD_CREATOR
#define MIX_DD_BIN_DD_CREATOR

#include <vector>
#include <unordered_map>
#include <cmath>
#include "typedefs.hpp"
#include "utils/math_utils.hpp"
#include "utils/double_top_stack.hpp"
#include "utils/hash.hpp"
#include "graph.hpp"
#include "bin_decision_diagram.hpp"

namespace mix::dd
{
    template<class InputFunction>
    class bin_dd_creator
    {
    private:
        using vertex = typename graph<int, int>::vertex;
        using arc    = typename graph<int, int>::arc;
        
        struct stack_frame
        {
            vertex* vertexPtr;
            size_t level;
        };

        struct vertex_key
        {
            vertex* negative;
            vertex* positive;

            auto operator== (const vertex_key& rhs) const -> bool;
        };
        
        struct vertex_key_hash
        {
            auto operator() (const vertex_key& key) const -> size_t;
        };

    private:
        using level_map = std::unordered_map<vertex_key, vertex*, vertex_key_hash>;

        const std::vector<var_name_t> variableNames;
        const size_t  inputCount;
        InputFunction inputFunction;

        utils::double_top_stack<stack_frame> stack;
        std::vector<level_map> levels;
        size_t currentLevel;

    public:
        template<class InFuncRef>
        bin_dd_creator(InFuncRef&& pInputFunction);

        // možno template na Vertex a Arc data
        auto create_diagram () -> bin_decision_diagram;
        // TODO keď vytvorí jeden sú atribúty v neznámom stave, resetovať?

    private:
        auto try_insert_vertex (const vertex_key key) -> vertex*; 
        auto variable_name     (const size_t level) const -> std::string;
    };

    template<class InputFunction>
    template<class InFuncRef>
    bin_dd_creator<InputFunction>::bin_dd_creator(InFuncRef&& pInputFunction) :
        variableNames {pInputFunction.begin(), pInputFunction.end()}
      , inputCount    {utils::pow(static_cast<size_t>(2), variableNames.size())}
      , inputFunction {std::forward<InFuncRef>(pInputFunction)}
      , currentLevel  {this->variableNames.size() - 1}
    {
        levels.resize(this->variableNames.size());
    }

    template<class InputFunction>
    auto bin_dd_creator<InputFunction>::create_diagram () -> bin_decision_diagram
    {
    // TODO map interface ano ale za tým by stačil list
        std::map<log_val_t, vertex*> valToLeaf { {0, new vertex {"0", this->currentLevel + 1}}, {1, new vertex {"1", this->currentLevel + 1}} };
        std::map<vertex*, log_val_t> leafToVal { {valToLeaf[0], 0}, {valToLeaf[1], 1} };

        size_t inputIndex {0};
        while (inputIndex < inputCount)
        {
            const log_val_t currInputVal {this->inputFunction[inputIndex]};
            const log_val_t nextInputVal {this->inputFunction[inputIndex + 1]};

            vertex* son {nullptr};

            if (currInputVal == nextInputVal)
            {
                son = valToLeaf[currInputVal];
            }
            else
            {
                // TODO 01 alebo 10 ale je to to isté asi ich bude treba swapnut?
                vertex* const negativeTarget {valToLeaf[currInputVal]};
                vertex* const positiveTarget {valToLeaf[nextInputVal]};

                son = this->try_insert_vertex(vertex_key {negativeTarget, positiveTarget});
            }
            
            stack.push(stack_frame {son, currentLevel});
            
            while (stack.size() > 1)
            {
                if (stack.top().level != stack.under_top().level)
                {
                    break;
                }

                if (stack.top().vertexPtr == stack.under_top().vertexPtr)
                {
                    vertex* v {stack.top().vertexPtr};

                    stack.pop();
                    stack.pop();

                    stack.push(stack_frame {v, this->currentLevel - 1});
                    continue;
                }
                
                // TODO tu treba asi checknúť, ktorý ma byť ľavý a pravý
                vertex* const negativeTarget {stack.under_top().vertexPtr};
                vertex* const positiveTarget {stack.top().vertexPtr};

                stack.pop();
                stack.pop();

                --this->currentLevel;
                son = this->try_insert_vertex(vertex_key {negativeTarget, positiveTarget});

                stack.push(stack_frame {son, this->currentLevel});
            }

            inputIndex += 2;
        }

        return bin_decision_diagram {stack.top().vertexPtr, std::move(valToLeaf), std::move(leafToVal)};
    }

    template<class InputFunction>
    auto bin_dd_creator<InputFunction>::try_insert_vertex (const vertex_key key) -> vertex*
    {
        // https://www.boost.org/doc/libs/1_71_0/libs/pool/doc/html/boost/object_pool.html
        // dalo by sa vyskúšať na alokovanie vrcholov

        auto inGraphIt {levels[currentLevel].find(key)};
        if (inGraphIt != levels[currentLevel].end())
        {
            return (*inGraphIt).second;            
        }

        auto newVertex {new vertex {
            this->variable_name(this->currentLevel)
          , this->currentLevel
          , {arc {key.negative}, arc {key.positive}}
        }};
        levels[this->currentLevel][key] = newVertex;
        
        return newVertex;
    }

    template<class InputFunction>
    auto bin_dd_creator<InputFunction>::variable_name (const size_t level) const -> std::string
    {
        return this->variableNames[level]
             + "_" 
             + std::to_string(this->levels[level].size());
    }

    template<class InputFunction>
    auto bin_dd_creator<InputFunction>::vertex_key::operator== (const vertex_key& other) const -> bool
    {
        return this->negative == other.negative
            && this->positive == other.positive;
    }

    template<class InputFunction>
    auto bin_dd_creator<InputFunction>::vertex_key_hash::operator() (const vertex_key& key) const -> size_t
    {
        size_t seed {0};

        // utils::boost::hash_combine<vertex*, utils::pointer_hash<vertex>>(seed, key.negative);
        // utils::boost::hash_combine<vertex*, utils::pointer_hash<vertex>>(seed, key.positive);

        utils::boost::hash_combine<vertex*, std::hash<vertex*>>(seed, key.negative);
        utils::boost::hash_combine<vertex*, std::hash<vertex*>>(seed, key.positive);

        return seed;
    }
}

#endif