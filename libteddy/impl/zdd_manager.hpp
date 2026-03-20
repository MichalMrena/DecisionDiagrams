#ifndef LIBTEDDY_DETAILS_ZDD_MANAGER_HPP
#define LIBTEDDY_DETAILS_ZDD_MANAGER_HPP

#include <libteddy/impl/diagram_manager.hpp>
#include <libteddy/impl/node_manager.hpp>
#include "libteddy/impl/node.hpp"
#include <libteddy/inc/io.hpp>
#include <iostream>

namespace teddy
{

#define DOMAIN_SIZE 2

class zdd_manager
{
public:
    using node_t = node_manager<degrees::fixed<DOMAIN_SIZE>, domains::fixed<DOMAIN_SIZE>>::node_t;
    using diagram_t = diagram<degrees::fixed<DOMAIN_SIZE>>;

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

    auto from_vector(const std::vector<int>& vector) -> node_t* {
        if (vector.empty() || (vector.size() & (vector.size() - 1)) != 0) {
            return nullptr;
        }

        std::vector<stack_frame> s;
        size_t pos = 0;
        int const terminalLevel = m_nodes.get_var_count();

        while (pos < vector.size()) {
            node_t* u = m_nodes.make_terminal_node(vector[pos++]);
            s.push_back({u, terminalLevel});
            shrink(s);
        }

        return s.back().node;
    }

    auto evaluate(diagram_t const& diagram, const std::vector<int>& values) -> int32 {
        node_t* node = diagram.unsafe_get_root();

        while (not node->is_terminal()) {
            int32 const index = node->get_index();
            assert(m_nodes.is_valid_var_value(index, values[static_cast<uint32>(index)]));
            node = node->get_son(values[static_cast<uint32>(index)]);
         }

        return node->get_value();
    }

    auto to_dot(diagram_t const& diagram) -> void {
        io::to_dot(m_nodes, std::cout, diagram);
    }

    auto to_dot(node_t* node) -> void {
        io::to_dot(m_nodes, std::cout, diagram_t(node));
    }


private:
    node_manager<degrees::fixed<DOMAIN_SIZE>, domains::fixed<DOMAIN_SIZE>> m_nodes;

    using stack_frame   = struct {
        node_t* node;
        int32 level;
    };

    auto shrink(std::vector<stack_frame>& s) -> void {
        while(true) {
            if (s.size() < DOMAIN_SIZE) {
                return;
            }

            int32 const currentLevel = s.back().level;
            auto sons = node_t::make_son_container(DOMAIN_SIZE);

            for (int i = 0; i < DOMAIN_SIZE; ++i) {
                auto const& frame = s[s.size() - DOMAIN_SIZE + static_cast<size_t>(i)];

                if (frame.level != currentLevel) {
                    return;
                }

                sons[i] = frame.node;
            }

            s.resize(s.size() - DOMAIN_SIZE);

            int32 newIndex = m_nodes.get_index(currentLevel - 1);
            node_t* u = m_nodes.make_internal_node_zdd(newIndex, sons);
            s.push_back({u, currentLevel - 1});
        }
    }
};

} // namespace teddy

#endif