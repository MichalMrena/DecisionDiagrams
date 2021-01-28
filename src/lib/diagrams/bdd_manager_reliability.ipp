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
        (double_v const& ps, bdd_t& f) -> void
    {
        base::calculate_probabilities(this->to_prob_table(ps), f);
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
        return base::get_probability(0);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::availability
        (double_v const& ps, bdd_t& f) -> double
    {
        this->calculate_probabilities(ps, f);
        return base::get_probability(1);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::unavailability
        (double_v const& ps, bdd_t& f) -> double
    {
        this->calculate_probabilities(ps, f);
        return base::get_probability(0);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::dpbd
        (bdd_t const& f, index_t const i) -> bdd_t
    {
        return this->template apply<AND>( this->negate(this->restrict_var(f, i, 0))
                                        , this->restrict_var(f, i, 1) );
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::dpbds
        (bdd_t const& f) -> bdd_v
    {
        return utils::fill_vector( base::vertexManager_.var_count()
                                 , std::bind_front(&bdd_manager::dpbd, this, f) );
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::structural_importance
        (bdd_t& dpbd) -> double
    {
        auto const sc = static_cast<double>(this->satisfy_count(dpbd) / 2);
        auto const domainSize = utils::two_pow(base::vertexManager_.var_count() - 1);
        return sc / static_cast<double>(domainSize);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::structural_importances
        (bdd_v& dpbds) -> double_v
    {
        return utils::fmap(dpbds, std::bind_front(&bdd_manager::structural_importance, this));
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::birnbaum_importance
        (double_v const& ps, bdd_t& dpbd) -> double
    {
        return this->availability(ps, dpbd);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::birnbaum_importances
        (double_v const& ps, bdd_v& dpbds) -> double_v
    {
        return utils::fmap(dpbds, std::bind_front(&bdd_manager::birnbaum_importance, this, ps));
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
        return utils::fmap_i(BIs, [this, U, &ps](auto const i, auto const BI)
        {
            return this->criticality_importance(BI, 1 - ps[i], U);
        });
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::fussell_vesely_importance
        (bdd_t& dpbd, double const qi, double_v const& ps, double const U) -> double
    {
        auto mnf = this->to_mnf(dpbd);
        return (qi * this->availability(ps, mnf)) / U;
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::fussell_vesely_importances
        (bdd_v& dpbds, double_v const& ps, double const U) -> double_v
    {
        return utils::fmap_i(dpbds, [this, &ps, U](auto const i, auto&& dpbd)
        {
            return this->fussell_vesely_importance(dpbd, 1 - ps[i], ps, U);
        });
    }

    template<class VertexData, class ArcData>
    template<class VectorType>
    auto bdd_manager<VertexData, ArcData>::mcvs
        (std::vector<bdd_t> const& dpbds) -> std::vector<VectorType>
    {
        auto dpbdes = utils::fmap_i(dpbds, std::bind_front(&bdd_manager::to_dpbd_e, this));
        auto const conj = this->template tree_fold<PI_CONJ>(dpbdes);
        return this->satisfy_all<VectorType>(conj);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::to_prob_table
        (double_v const& ps) -> prob_table
    {
        return utils::fmap(ps, [](auto const p)
        {
            return std::array<double, 2> {1 - p, p};
        });
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::to_mnf
        (bdd_t const& dpbd) -> bdd_t
    {
        return this->transform(dpbd, [this](auto const v, auto&& l_this)
        {
            // If 0-th son is the false leaf we set 0-th son to 1-th son.
            // Otherwise we continue down to the 0-th son.
            // We always continue to 1-th son.
            auto sons  = vertex_a {};
            auto son0  = v->get_son(0);
            auto son1  = v->get_son(1);
            auto son1t = this->transform_step(son1, l_this); // TODO premyslieť si tú rekurziu, aj keď asi ak son0 nie je 0 pred zanorením asi nebude ani potom
            auto const leaf0 = this->vertexManager_.terminal_vertex(0);
            sons[0] = son0 == leaf0 ? son1t : this->transform_step(son0, l_this);
            sons[1] = son1t;
            return sons;
        });
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::to_dpbd_e
        (index_t const i, bdd_t const& dpbd) -> bdd_t
    {
        return base::to_dpbd_e(0, i, dpbd);
    }
}