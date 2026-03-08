#ifndef LIBTEDDY_DETAILS_ZDD_MANAGER_HPP
#define LIBTEDDY_DETAILS_ZDD_MANAGER_HPP

#include <libteddy/impl/diagram_manager.hpp>
#include <libteddy/impl/node_manager.hpp>
#include "libteddy/impl/node.hpp"
#include <stack>
#include <algorithm>
#include <stdio.h>

/*
    TODO:
    - first zdd rule which returns terminal can break shrink
*/

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

    auto from_vector(const std::vector<int> &vector) -> node_t* {
        if (vector.empty() || vector.size() % 2 != 0) {
            return nullptr;
        }

        diagram_t result;

        std::stack<node_t*> s;
        int p = 2;
        int i = static_cast<int>(std::log2(vector.size()));
        std::vector<node_t*> terminals = {m_nodes.make_terminal_node(0), m_nodes.make_terminal_node(1)};
        size_t pos = 0;

        while (pos < vector.size()) {
            auto sons = node_t::make_son_container(p);
            for (int x = 0; x < p; ++x) {
                if (vector[pos] == 1) {
                    sons[x] = terminals[1];
                }
                else {
                    sons[x] = terminals[0];
                }
                ++pos;
            }

            node_t* u = m_nodes.make_internal_node_zdd(i, sons);
            s.push(u);

            shrink(s);
        }

        return s.top();
    }



private:
    node_manager<degrees::fixed<2>, domains::fixed<2>> m_nodes;

    void shrink(std::stack<node_t*>& s) {
        while(true) {
            if (s.empty()) {
                return;
            }

            if (s.top()->is_terminal()) {
                return;
            }

            int i = s.top()->get_index();
            if (i == 1) {
                return;
            }
            
            int d = 2;
            if (!can_shrink(s, i, d)) {
                return;
            }

            std::vector<node_t*> children;
            for (int x = 0; x < d; ++x) {
                if (s.top()->get_index() != i) {
                    return;
                }
                children.push_back(s.top());
                s.pop();
            }

            std::ranges::reverse(children);
            
            auto sons = node_t::make_son_container(d);
            for (size_t x = 0; x < children.size(); ++x) {
                sons[static_cast<int64>(x)] = children[x];
            }

            node_t* u = m_nodes.make_internal_node_zdd(i - 1, sons);
            s.push(u);
        }
    }

    auto static can_shrink(std::stack<node_t*>& s, int i, int d) -> bool {
        if (s.size() < static_cast<size_t>(d)) {
            return false;
        }

        std::stack<node_t*> temp = s;

        for (int x = 0; x < d; ++x) {
            if (temp.top()->get_index() != i) {
                return false;
            }
            temp.pop();
        }
        return true;
    }
};

} // namespace teddy

#endif