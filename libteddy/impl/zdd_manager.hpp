#ifndef LIBTEDDY_DETAILS_ZDD_MANAGER_HPP
#define LIBTEDDY_DETAILS_ZDD_MANAGER_HPP

#include <libteddy/impl/diagram_manager.hpp>
#include <libteddy/impl/node_manager.hpp>

namespace teddy
{

class zdd_manager
{
public:
    using node_t = node_manager<degrees::fixed<2>, domains::fixed<2>>::node_t;
    using diagram_t = diagram<degrees::fixed<2>>;

    zdd_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 extraNodePoolSize,
        std::vector<int32> order = {}
    ) :
    m_nodes(
        varCount,
        nodePoolSize,
        extraNodePoolSize,
        detail::default_or_fwd(varCount, TEDDY_MOVE(order))
    ){
    }

    auto from_vector(const std::vector<int> &vector) -> diagram_t {
        
        return diagram_t(nullptr);
    }



private:
    node_manager<degrees::fixed<2>, domains::fixed<2>> m_nodes;
};

} // namespace teddy

#endif