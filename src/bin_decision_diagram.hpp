#ifndef MIX_DD_BIN_DECISION_DIAGRAM
#define MIX_DD_BIN_DECISION_DIAGRAM

#include <string>
#include <vector>
#include <map>
// graf možno ani nebude treba, structy by sa nestli sem a template by sa tiež presunul sem
// záleží ako to bude s dátami v grafe
#include "graph.hpp"
#include "typedefs.hpp"

namespace mix::dd
{   
    // TODO asi template s dátami
    class bin_decision_diagram
    {
    private:
        using vertex = typename graph<int, int>::vertex;
        using arc    = typename graph<int, int>::arc;

        vertex* root;

        // TODO map interface ano ale za tým by stačil list
        std::map<log_val_t, vertex*> valToLeaf;
        std::map<vertex*, log_val_t> leafToVal;

    public:
        template<typename U>
        friend class bin_dd_creator;

    public:
        ~bin_decision_diagram();

        auto to_dot_graph () const -> std::string;

        auto complement   () const -> bin_decision_diagram;  // not yet implemented
        auto complement   ()       -> bin_decision_diagram&; // not yet implemented

        auto get_value (const input_t input) const -> log_val_t;

        template<class SomeFunction> // TODO funkcia asi bude závisieť od dát, čo su vo vrcholoch
        auto traverse (SomeFunction f) -> void; // not yet implemented

    private:
        bin_decision_diagram(vertex* pRoot
                           , std::map<log_val_t, vertex*>&& pValToLeaf
                           , std::map<vertex*, log_val_t>&& pLeafToVal);
    };
}

#endif