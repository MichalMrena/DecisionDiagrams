#ifndef MIX_DD_BIN_DD_CREATOR
#define MIX_DD_BIN_DD_CREATOR

#include <vector>
#include "typedefs.hpp"
#include "utils/math_utils.hpp"
#include "utils/double_top_stack.hpp"
#include "utils/boost.hpp"
#include "graph.hpp"
#include "bin_decision_diagram.hpp"

namespace mix::dd
{
    template<class InputFunction>
    class bin_dd_creator
    {
    private:
        template<class VertexData, class ArcData>
        struct stack_frame
        {
            typename graph<VertexData, ArcData>::vertex* vertex;
            int level;
        };

        template<class vertex_t>
        struct vertex_key
        {
            int level;
            vertex_t* left;
            vertex_t* right;

            auto operator== (const vertex_key& rhs) const -> bool;
        };
        // TODO hash for vertex_key

    private:
        const std::vector<var_name_t> variableNames;
        const size_t  inputCount;
        InputFunction inputFunction;

    public:
        template<class InFuncRef>
        bin_dd_creator(InFuncRef&& pInputFunction);

        // možno template na Vertex a Arc data
        auto create_diagram () -> bin_decision_diagram;
    };

    template<class InputFunction>
    template<class InFuncRef>
    bin_dd_creator<InputFunction>::bin_dd_creator(InFuncRef&& pInputFunction) :
        variableNames {pInputFunction.begin(), pInputFunction.end()}
      , inputCount    {utils::pow(static_cast<size_t>(2), variableNames.size())}
      , inputFunction {std::forward<InFuncRef>(pInputFunction)}
    {
    }

    template<class InputFunction>
    auto bin_dd_creator<InputFunction>::create_diagram () -> bin_decision_diagram
    {
        using vertex  = typename graph<int, int>::vertex;
        using arc     = typename graph<int, int>::arc;
        using s_frame = stack_frame<int, int>;

        utils::double_top_stack<s_frame> stack;
        
        size_t inputIndex {0};
        while (inputIndex < inputCount)
        {
            const log_val_t currInputVal {this->inputFunction[inputIndex]};
            const log_val_t nextInputVal {this->inputFunction[inputIndex + 1]};

            // TODO do some magic here

            inputIndex += 2;
        }

        return bin_decision_diagram {};
    }

    template<class InputFunction>
    template<class vertex_t>
    auto bin_dd_creator<InputFunction>::vertex_key<vertex_t>::operator== (const vertex_key& other) const -> bool
    {
        return this->level == other.level
            && this->left  == other.left
            && this->right == other.right;
    }
}


#endif