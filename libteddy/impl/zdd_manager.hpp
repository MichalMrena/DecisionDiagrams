#ifndef LIBTEDDY_DETAILS_ZDD_MANAGER_HPP
#define LIBTEDDY_DETAILS_ZDD_MANAGER_HPP

#include <libteddy/impl/diagram_manager.hpp>
#include <libteddy/impl/node_manager.hpp>
#include "libteddy/impl/node.hpp"
#include <stack>
#include <algorithm>

//TODO
// fix can_shrink doesn't work well if terminal node is on stack
// with only two levels, shrink works but with more doesnt

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

        std::stack<node_t*> s;
        int p = 2;
        int i = static_cast<int>(std::log2(vector.size())) - 1;
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

    auto evaluate(diagram_t const& diagram, const std::vector<int>& values) -> int32 {
        node_t* node = diagram.unsafe_get_root();

        while (not node->is_terminal()) {
            int32 const index = node->get_index();
            assert(m_nodes.is_valid_var_value(index, values[static_cast<uint32>(index)]));
            node = node->get_son(values[static_cast<uint32>(index)]);
         }

        return node->get_value();
    }


private:
    node_manager<degrees::fixed<2>, domains::fixed<2>> m_nodes;

    auto shrink(std::stack<node_t*>& s) -> void {
        while(true) {
            if (s.empty()) {
                return;
            }

            if (s.top()->is_terminal()) {
                return;
            }

            int i = s.top()->get_index();
            if (i == 0) {
                return;
            }

            int d = 2;
            if (!can_shrink(s, i, d)) {
                return;
            }

            std::vector<node_t*> children;
            for (int x = 0; x < d; ++x) {
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

        std::vector<node_t*> temp;
        bool has_terminal = false;
        bool has_the_level = false;

        for (int x = 0; x < d; ++x){
            auto* n = s.top();
            s.pop();

            temp.push_back(n);

            if (n->is_terminal()) {
                has_terminal = true;
            }
            else if (n->get_index() == i) {
                has_the_level = true;
            }
            else {
                for (auto it = temp.rbegin(); it != temp.rend(); ++it) {
                    s.push(*it);
                }
                return false;
            }
        }

        for (auto it = temp.rbegin(); it != temp.rend(); ++it) {
            s.push(*it);
        }

        return true;
    }
};

} // namespace teddy

#endif