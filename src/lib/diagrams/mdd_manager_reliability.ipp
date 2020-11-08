#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include <numeric>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::calculate_probabilities
        (prob_table const& ps, mdd_t& f) -> void
    {
        vertexManager_.for_each_terminal([](auto const v)
        {
            v->data = 0.0;
        });

        this->traverse_pre(f, [](auto const v)
        {
            v->data = 0.0;
        });
        f.get_root()->data = 1.0;

        this->traverse_level(f, [this, &ps](auto const v)
        {
            if (!this->vertexManager_.is_leaf(v))
            {
                for (auto i = 0u; i < P; ++i)
                {
                    v->get_son(i)->data += v->data * ps[v->get_index()][i];
                }
            }
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_probability
        (log_t const level) const -> double
    {
        auto const leaf = vertexManager_.get_terminal_vertex(level);
        return leaf ? leaf->data : 0;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_availability
        (log_t const level) const -> double
    {
        return this->sum_terminals(level, P);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_unavailability
        (log_t const level) const -> double
    {
        return this->sum_terminals(0, level);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::availability
        (log_t const level, prob_table const& ps, mdd_t& f) -> double
    {
        this->calculate_probabilities(ps, f);
        return this->get_availability(level);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::unavailability
        (log_t const level, prob_table const& ps, mdd_t& f) -> double
    {
        this->calculate_probabilities(ps, f);
        return this->get_unavailability(level);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbd
        (val_change<P> const var, val_change<P> const f, mdd_t const& sf, index_t const i) -> mdd_t
    {
        return this->apply( this->apply(this->restrict_var(sf, i, var.from), EQUAL_TO<P>(), this->just_val(f.from))
                          , AND<P, domain_e::nonhomogenous>()
                          , this->apply(this->restrict_var(sf, i, var.to), EQUAL_TO<P>(), this->just_val(f.to)) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbd_integrated_1
        (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i) -> mdd_t
    {
        return this->apply( this->apply(this->restrict_var(sf, i, var.from), EQUAL_TO<P>(), this->just_val(fVal))
                          , AND<P, domain_e::nonhomogenous>()
                          , this->apply(this->restrict_var(sf, i, var.to), LESS<P>(), this->just_val(fVal)) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbd_integrated_2
        (val_change<P> const var, mdd_t const& sf, index_t const i) -> mdd_t
    {
        return this->apply( this->restrict_var(sf, i, var.from)
                          , GREATER<P>()
                          , this->restrict_var(sf, i, var.to) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbd_integrated_3
        (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i) -> mdd_t
    {
        return this->apply( this->apply(this->restrict_var(sf, i, var.from), GREATER_EQUAL<P>(), this->just_val(fVal)) 
                          , AND<P, domain_e::nonhomogenous>()
                          , this->apply(this->restrict_var(sf, i, var.to), LESS<P>(), this->just_val(fVal)) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds
        (val_change<P> const var, val_change<P> const f, mdd_t const& sf) -> mdd_v
    {
        return utils::fill_vector( this->get_var_count()
                                 , std::bind_front(&mdd_manager::dpbd, this, var, f, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds_integrated_1
        (val_change<P> const var, log_t const fVal, mdd_t const& sf) -> mdd_v
    {
        return utils::fill_vector( this->get_var_count()
                                 , std::bind_front(&mdd_manager::dpbd_integrated_1, this, var, fVal, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds_integrated_2
        (val_change<P> const var, mdd_t const& sf) -> mdd_v
    {
        return utils::fill_vector( this->get_var_count()
                                 , std::bind_front(&mdd_manager::dpbd_integrated_2, this, var, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds_integrated_3
        (val_change<P> const var, log_t const fVal, mdd_t const& sf) -> mdd_v
    {
        return utils::fill_vector( this->get_var_count()
                                 , std::bind_front(&mdd_manager::dpbd_integrated_3, this, var, fVal, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::structural_importance
        (std::size_t const domainSize, mdd_t& dpbd) -> double
    {
        auto const onesCount = static_cast<double>(this->satisfy_count(1, dpbd) / P);
        return domainSize ? onesCount / domainSize : 0;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::structural_importance
        (log_v const& domains, mdd_t& dpbd, index_t const i) -> double
    {
        auto const domProduct = std::reduce( std::begin(domains), std::end(domains)
                                           , std::size_t {1}, std::multiplies() );
        return domains[i] ? this->structural_importance(domProduct / domains[i], dpbd) : 0;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::structural_importances
        (log_v const& domains, mdd_v& dpbds) -> double_v
    {
        auto const domProduct = std::reduce( std::begin(domains), std::end(domains)
                                           , std::size_t {1}, std::multiplies() );
        auto zs = utils::zip(utils::range(0u, dpbds.size()), dpbds);
        return utils::map(zs, dpbds.size(), [this, domProduct, &domains](auto&& pair)
        {
            auto&& [i, d] = pair;
            return domains[i] ? this->structural_importance(domProduct / domains[i], d) : 0;
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::birnbaum_importance
        (prob_table const& ps, mdd_t& dpbd)  -> double
    {
        this->calculate_probabilities(ps, dpbd);
        return this->get_probability(1);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::birnbaum_importances
        (prob_table const& ps, mdd_v& dpbds) -> double_v
    {
        return utils::map(dpbds, std::bind_front(&mdd_manager::birnbaum_importance, this, ps));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::sum_terminals
        (log_t const from, log_t const to) const -> double
    {
        auto sumval = 0.0;
        for (auto i = from; i < to; ++i)
        {
            sumval += this->get_probability(i);
        }
        return sumval;
    }
}