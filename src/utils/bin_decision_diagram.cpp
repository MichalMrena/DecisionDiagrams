#include "bin_decision_diagram.hpp"

#include <sstream>
#include <set>
#include <queue>

namespace mix::dd
{
    bin_decision_diagram::bin_decision_diagram(vertex* pRoot
                      , std::map<log_val_t, vertex*>&& pValToLeaf
                      , std::map<vertex*, log_val_t>&& pLeafToVal) :
        root {pRoot}
      , valToLeaf {std::move(pValToLeaf)}
      , leafToVal {std::move(pLeafToVal)}
    {
    }

    auto bin_decision_diagram::to_dot_graph () -> std::string
    {
        std::ostringstream ost;

        std::set<vertex*> processed;
        std::queue<vertex*> toProcess;
        toProcess.push(this->root);

        ost << "digraph D {" << '\n'
            << "    node [shape = square] 0 1;" << '\n'
            << "    node [shape = circle];"     << "\n\n";

        while (! toProcess.empty())
        {
            vertex* v {toProcess.front()};
            toProcess.pop();

            if (v->is_leaf() || processed.find(v) != processed.end())
            {
                continue;
            }
            
            vertex* negativeTarget {v->forwardStar[0]->target};
            vertex* positiveTarget {v->forwardStar[1]->target};

            ost << "    " << v->label << " -> " << negativeTarget->label << " [style = dashed];" << '\n';
            ost << "    " << v->label << " -> " << positiveTarget->label << " [style = solid];"  << '\n';

            processed.insert(v);
            toProcess.push(negativeTarget);
            toProcess.push(positiveTarget);
        }

        ost << '}' << '\n';

        return ost.str();
    }
}