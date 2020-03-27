#ifndef MIX_DD_MDD_CREATOR_
#define MIX_DD_MDD_CREATOR_

#include "mdd.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData, size_t P>
    class mdd_creator : public dd_manipulator_base<VertexData, ArcData, P>
    {
    public:
        using mdd_t    = mdd<VertexData, ArcData, P>;
        using vertex_t = typename mdd_t::vertex_t;
        using log_t    = typename mdd_t::log_t;

    public:
        auto just_val (const log_t   val)   -> mdd_t;
        auto just_var (const index_t index) -> mdd_t;

    private:
        using leaf_val_map = typename mdd_t::leaf_val_map;
    };    
    
    template<class VertexData, class ArcData, size_t P>
    auto mdd_creator<VertexData, ArcData, P>::just_var
        (const index_t index) -> mdd_t
    {
        auto id   (0);
        auto root (this->create_vertex(id++, index));
        
        leaf_val_map leafToVal;

        for (auto val (0u); val < P; ++val)
        {
            auto leaf {this->create_vertex(id++, index + 1)};
            leafToVal.emplace(leaf, val);
            root->son(val) = leaf;
        }

        return mdd_t (root, index + 1, std::move(leafToVal));
    }
}

#endif