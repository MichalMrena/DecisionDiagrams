#ifndef MIX_DD_BDD_MANAGER_HPP
#include "../bdd_manager.hpp"
#endif

#include "../utils/more_vector.hpp"
#include "../utils/more_iterator.hpp"

#include <functional>

namespace mix::dd
{
    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::calculate_probabilities
        (bdd_t& f, double_v const& ps) -> void
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
        (bdd_t& f, double_v const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_probability(1);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::unavailability
        (bdd_t& f, double_v const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_probability(0);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::dpbd
        (bdd_t const& f, index_t const i) -> bdd_t
    {
        return this->apply( this->negate(this->restrict_var(f, i, 0)) // TODO project?
                          , AND()
                          , this->restrict_var(f, i, 1) );
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::birnbaum_importance
        (bdd_t& dpbd, double_v const& ps) -> double
    {
        return this->availability(dpbd, ps);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::birnbaum_importances
        (bdd_v& dpbds, double_v const& ps) -> double_v
    {
        using namespace std::placeholders;
        return utils::map(dpbds, std::bind(&bdd_manager::birnbaum_importance, this, _1, ps));
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::criticality_importance
        (double const BI, double const qi, double const U) -> double
    {
        return (BI * qi) / U;
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::criticality_importances
        (double_v const& BIs, double_v const& ps, double const U) -> double_v
    {
        return utils::map(utils::zip(BIs, ps), ps.size(), [this, U](auto pair)
        {
            auto const [BI, pi] = pair;
            return this->criticality_importance(BI, 1 - pi, U);
        });
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::dpbds
        (bdd_t const& f) -> bdd_v
    {
        using namespace std::placeholders;
        auto const is = utils::range(0u, base::vertexManager_.get_var_count());
        return utils::map(is, std::bind(&bdd_manager::dpbd, this, f, _1));
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::structural_importance
        (bdd_t& dpbd) -> double
    {
        auto const sc = static_cast<double>(this->satisfy_count(dpbd) / 2);
        return sc / utils::two_pow(base::vertexManager_.get_var_count() - 1);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::structural_importances
        (bdd_v& dpbds) -> double_v
    {
        using namespace std::placeholders;
        return utils::map(dpbds, std::bind(&bdd_manager::structural_importance, this, _1));
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::to_prob_table
        (double_v const& ps) -> prob_table
    {
        return utils::map(ps, [](auto const p)
        {
            return std::array<double, 2> {1 - p, p};
        });
    }
}