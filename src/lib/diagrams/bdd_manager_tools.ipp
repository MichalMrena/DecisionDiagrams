#ifndef MIX_DD_BDD_MANAGER_HPP
#include "../bdd_manager.hpp"
#endif

namespace mix::dd
{
    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::satisfy_count
        (bdd_t& d) -> std::size_t
    {
        return base::satisfy_count(1, d);
    }

    template<class VertexData, class ArcData>
    template<class VariableValues, class OutputIt, class SetVarVal>
    auto bdd_manager<VertexData, ArcData>::satisfy_all
        (bdd_t const& d, OutputIt out) const -> void
    {
        auto xs = VariableValues {};
        this->satisfy_all_step(0, d.get_root(), xs, out);
    }

    template<class VertexData, class ArcData>
    template<class VariableValues, class OutputIt, class SetVarVal>
    auto bdd_manager<VertexData, ArcData>::satisfy_all_step
        ( index_t   const i
        , vertex_t* const v
        , VariableValues& xs
        , OutputIt&       out ) const -> void
    {
        auto const set_var = SetVarVal {};
        auto const val = base::vertexManager_.get_terminal_value(v);

        if (base::vertexManager_.is_leaf(v) && 1 != val)
        {
            return;
        }
        else if (base::vertexManager_.is_leaf(v) && 1 == val)
        {
            *out++ = xs;
            return;
        }
        else if (v->get_index() > i) // TODO check levels
        {
            set_var(xs, i, 0);
            satisfy_all_step(i + 1, v, xs, out);
            set_var(xs, i, 1);
            satisfy_all_step(i + 1, v, xs, out);
        }
        else
        {
            set_var(xs, i, 0);
            satisfy_all_step(i + 1, v->get_son(0), xs, out);
            set_var(xs, i, 1);
            satisfy_all_step(i + 1, v->get_son(1), xs, out);
        }
    }
}