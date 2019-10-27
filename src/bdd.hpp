#ifndef MIX_DD_BDD
#define MIX_DD_BDD

#include <string>
#include <vector>
#include <map>
#include "graph.hpp"
#include "typedefs.hpp"

namespace mix::dd
{   
    // TODO asi template s dátami
    class bdd
    {
    private:
        using vertex = typename graph<int, int>::vertex;
        using arc    = typename graph<int, int>::arc;

        vertex* root;

        // TODO map interface ano ale za tým by stačil list
        // TODO valToLeaf tu ani nebude treba
        std::map<log_val_t, vertex*> valToLeaf;
        std::map<vertex*, log_val_t> leafToVal;

    public:
        template<typename U>
        friend class bdd_creator;

    public:
        static auto TRUE  () -> bdd;
        static auto FALSE () -> bdd;

    public:
        bdd(const bdd& other); // not yet implemented
        ~bdd();

        auto to_dot_graph () const -> std::string;
        
        auto get_value (const input_t input) const -> log_val_t;

        template<class BinaryFunction> // TODO enableIf is boolen function
        auto apply (const bdd& other) const -> bdd; // not yet implemented

    private:
        bdd(vertex* pRoot
          , std::map<log_val_t, vertex*>&& pValToLeaf
          , std::map<vertex*, log_val_t>&& pLeafToVal);

        auto reduce () -> void;  // not yet implemented
    };
}

#endif