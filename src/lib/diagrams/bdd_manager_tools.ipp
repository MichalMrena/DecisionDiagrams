#ifndef MIX_DD_BDD_MANAGER_HPP
#include "../bdd_manager.hpp"
#endif

#include "../utils/more_math.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::satisfy_count
        (bdd_t& d) -> std::size_t
    {
        return base::satisfy_count(1, d);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::truth_density
        (bdd_t& d) -> double
    {
        return static_cast<double>(this->satisfy_count(d))
             / utils::two_pow(this->get_var_count());
    }

    template<class VertexData, class ArcData>
    template<class VariableValues, class OutputIt, class SetVarVal>
    auto bdd_manager<VertexData, ArcData>::satisfy_all
        (bdd_t const& d, OutputIt out) const -> void
    {
        base::template satisfy_all<VariableValues>(1, d, out);
    }
}