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
        return 1 - this->get_availability();
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::availability
        (double_v const& ps, bdd_t& f) -> double
    {
        this->calculate_probabilities(ps, f);
        return this->get_probability(1);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::unavailability
        (double_v const& ps, bdd_t& f) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_probability(0);
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::dpbd
        (bdd_t const& f, index_t const i) -> bdd_t
    {
        return this->apply( this->negate(this->restrict_var(f, i, 0))
                          , AND()
                          , this->restrict_var(f, i, 1) );
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::dpbds
        (bdd_t const& f) -> bdd_v
    {
        // using namespace std::placeholders;
        // return utils::fill_vector( this->vertexManager_.get_var_count()
        //                          , std::bind(&bdd_manager::dpbd, this, f, _1) );
        return utils::fill_vector( this->vertexManager_.get_var_count()
                                 , std::bind_front(&bdd_manager::dpbd, this, f) );
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
        // using namespace std::placeholders;
        // return utils::map(dpbds, std::bind(&bdd_manager::structural_importance, this, _1));
        return utils::map(dpbds, std::bind_front(&bdd_manager::structural_importance, this));
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
        // using namespace std::placeholders;
        // return utils::map(dpbds, std::bind(&bdd_manager::birnbaum_importance, this, _1, ps));
        return utils::map(dpbds, std::bind_front(&bdd_manager::birnbaum_importance, this, ps));
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
        auto const is = utils::range(0u, dpbds.size());
        return utils::map(utils::zip(is, dpbds), dpbds.size(), [this, &ps, U](auto&& pair)
        {
            auto&& [i, dpbd] = pair;
            return this->fussell_vesely_importance(dpbd, 1 - ps[i], ps, U);
        });
    }

    template<class VertexData, class ArcData>
    template<class VectorType>
    auto bdd_manager<VertexData, ArcData>::mcvs
        (std::vector<bdd_t> dpbds) -> std::vector<VectorType>
    {
        auto is     = utils::range(0u, dpbds.size());
        auto dpbdes = utils::map(utils::zip(is, dpbds), dpbds.size(), [this](auto const& pair)
        {
            auto const& [i, dpbd] = pair;
            return this->to_dpbd_e(dpbd, i);
        });
        auto const conj = this->tree_fold(std::move(dpbdes), PI_CONJ {});
        auto cutVectors = std::vector<VectorType>();
        this->satisfy_all<VectorType>(conj, std::back_inserter(cutVectors));
        return cutVectors;
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

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::to_mnf
        (bdd_t const& dpbd) -> bdd_t
    {
        return this->transform(dpbd, [this](auto const v, auto&& l_this)
        {
            // If 0-th son is the false leaf we set 0-th son to 1-th son.
            // Otherwise we continue down to the 0-th son.
            // We always continue to 1-th son.
            auto sons  = son_a {};
            auto son0  = v->get_son(0);
            auto son1  = v->get_son(1);
            auto son1t = this->transform_step(son1, l_this);
            auto const leaf0 = this->vertexManager_.terminal_vertex(0);
            sons[0] = son0 == leaf0 ? son1t : this->transform_step(son0, l_this);
            sons[1] = son1t;
            return sons;
        });
    }

    template<class VertexData, class ArcData>
    auto bdd_manager<VertexData, ArcData>::to_dpbd_e
        (bdd_t const& dpbd, index_t const i) -> bdd_t
    {
        auto const root   = dpbd.get_root();
        auto const rLevel = this->vertexManager_.get_level(root);
        auto const iLevel = this->vertexManager_.get_level(i);

        // Special case when the new vertex for the i-th variable is inserted above the root.
        if (iLevel < rLevel)
        {
            auto constexpr U   = log_val_traits<2>::undefined;
            auto const uLeaf   = this->vertexManager_.terminal_vertex(U);
            auto const newRoot = this->vertexManager_.internal_vertex(i, son_a {root, uLeaf});
            return bdd_t {newRoot};
        }

        // Normal case for all internal vertices.
        return this->transform(dpbd, [this, iLevel, i](auto const v, auto&& l_this)
        {
            auto constexpr U  = log_val_traits<2>::undefined;
            auto const vLevel = this->vertexManager_.get_level(v);
            return utils::fill_array<2>([this, &l_this, i, v, vLevel, iLevel](auto const val)
            {
                auto const son    = v->get_son(val);
                auto const sLevel = this->vertexManager_.get_level(son);
                if (iLevel > vLevel && iLevel < sLevel)
                {
                    // New vertex for the i-th variable is inserted between vertex v and his son.
                    // No need to go deeper.
                    auto const uLeaf = this->vertexManager_.terminal_vertex(U);
                    return this->vertexManager_.internal_vertex(i, son_a {son, uLeaf});
                }
                else
                {
                    // No insertion point here, we need to go deeper.
                    return this->transform_step(son, l_this);
                }
            });
        });
    }
}