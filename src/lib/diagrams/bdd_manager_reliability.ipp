#ifndef MIX_DD_BDD_MANAGER_HPP
#include "../bdd_manager.hpp"
#endif

#include "../utils/more_vector.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::calculate_probabilities
        (bdd_t& f, prob_v const& ps) -> void
    {
        base::calculate_probabilities(f, this->to_prob_table(ps));
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::get_availability
        () const -> double
    {
        return base::get_probability(1);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::get_unavailability
        () const -> double
    {
        return 1 - this->get_availability();
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::availability
        (bdd_t& f, prob_v const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_probability(1);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::unavailability
        (bdd_t& f, prob_v const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_probability(0);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::to_prob_table
        (prob_v const& ps) -> prob_table
    {
        return utils::map(ps, [](auto const p)
        {
            return std::array<double, 2> {1 - p, p};
        });
    }
}