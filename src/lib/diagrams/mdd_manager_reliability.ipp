#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::calculate_probabilities
        (prob_table const& ps, mdd_t& f) -> void
    {
        vertexManager_.for_each_terminal_vertex([](auto const v)
        {
            v->data = 0.0;
        });

        this->traverse_pre(f, [](auto const v)
        {
            v->data = 0.0;
        });
        f.get_root()->data = 1.0;

        this->traverse_level(f, [&ps](auto const v)
        {
            v->for_each_son_i([v, &ps](auto const i, auto const son) mutable
            {
                son->data += v->data * ps[v->get_index()][i];
            });
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
        return this->apply<AND>( this->apply<EQUAL_TO>(this->restrict_var(sf, i, var.from), this->constant(f.from))
                               , this->apply<EQUAL_TO>(this->restrict_var(sf, i, var.to), this->constant(f.to)) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbd_integrated_1
        (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i) -> mdd_t
    {
        return this->apply<AND>( this->apply<EQUAL_TO>(this->restrict_var(sf, i, var.from), this->constant(fVal))
                               , this->apply<LESS>(this->restrict_var(sf, i, var.to), this->constant(fVal)) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbd_integrated_2
        (val_change<P> const var, mdd_t const& sf, index_t const i) -> mdd_t
    {
        return this->apply<GREATER>( this->restrict_var(sf, i, var.from)
                                   , this->restrict_var(sf, i, var.to) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbd_integrated_3
        (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i) -> mdd_t
    {
        return this->apply<AND>( this->apply<LESS>(this->restrict_var(sf, i, var.from), this->constant(fVal))
                               , this->apply<GREATER_EQUAL>(this->restrict_var(sf, i, var.to), this->constant(fVal)) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds
        (val_change<P> const var, val_change<P> const f, mdd_t const& sf) -> mdd_v
    {
        return utils::fill_vector( this->var_count()
                                 , std::bind_front(&mdd_manager::dpbd, this, var, f, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds_integrated_1
        (val_change<P> const var, log_t const fVal, mdd_t const& sf) -> mdd_v
    {
        return utils::fill_vector( this->var_count()
                                 , std::bind_front(&mdd_manager::dpbd_integrated_1, this, var, fVal, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds_integrated_2
        (val_change<P> const var, mdd_t const& sf) -> mdd_v
    {
        return utils::fill_vector( this->var_count()
                                 , std::bind_front(&mdd_manager::dpbd_integrated_2, this, var, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds_integrated_3
        (val_change<P> const var, log_t const fVal, mdd_t const& sf) -> mdd_v
    {
        return utils::fill_vector( this->var_count()
                                 , std::bind_front(&mdd_manager::dpbd_integrated_3, this, var, fVal, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::structural_importance
        (mdd_t& dpbd, index_t const i) -> double
    {
        auto const domProduct = this->get_domain_product();
        return this->structural_importance(domProduct / this->get_domain(i), dpbd);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::structural_importances
        (mdd_v& dpbds) -> double_v
    {
        auto const domProduct = this->get_domain_product();
        auto zs = utils::zip(utils::range(0u, static_cast<index_t>(dpbds.size())), dpbds);
        return utils::fmap(zs, dpbds.size(), [=, this](auto&& pair)
        {
            auto&& [i, d] = pair;
            return this->structural_importance(domProduct / this->get_domain(i), d);
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
        return utils::fmap(dpbds, std::bind_front(&mdd_manager::birnbaum_importance, this, ps));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::fussell_vesely_importance
        (prob_table const& ps, double const U, mdd_t const& dpbd) -> double
    {
        auto mnf = this->to_mnf(dpbd);
        this->calculate_probabilities(ps, mnf);
        auto const prmnf = this->get_probability(1);
        return prmnf / static_cast<double>(U);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::fussell_vesely_importances
        (prob_table const& ps, double const U, mdd_v const& dpbds) -> double_v
    {
        return utils::fmap(dpbds, std::bind_front(&mdd_manager::fussell_vesely_importance, this, ps, U));
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class SetIthVar>
    auto mdd_manager<VertexData, ArcData, P>::mcvs
        (mdd_v const& dpbds, log_t const level) -> std::vector<VariableValues>
    {
        auto cs = std::vector<VariableValues>();
        this->template mcvs_g<VariableValues>(dpbds, level, std::back_inserter(cs));
        return cs;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class OutputIt, class SetIthVar>
    auto mdd_manager<VertexData, ArcData, P>::mcvs_g
        (mdd_v const& dpbds, log_t const level, OutputIt out) -> void
    {
        auto const is = utils::range(0u, static_cast<index_t>(dpbds.size())); // TODO fmap_i
        auto dpbdes   = utils::fmap(utils::zip(is, dpbds), dpbds.size(), [=, this](auto const& pair)
        {
            auto const& [i, dpbd] = pair;
            return this->to_dpbde(dpbd, level, i);
        });
        auto const conj = this->tree_fold<PI_CONJ>(dpbdes);
        this->template satisfy_all_g<VariableValues, OutputIt, SetIthVar>(1, conj, out);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::sum_terminals
        (log_t const from, log_t const to) const -> double
    {
        auto const is = utils::range(from, to);
        return std::transform_reduce( std::begin(is), std::end(is), 0.0, std::plus<>()
                                    , std::bind_front(&mdd_manager::get_probability, this) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::structural_importance
        (std::size_t const domainSize, mdd_t& dpbd) -> double
    {
        auto const onesCount = static_cast<double>(this->satisfy_count(1, dpbd) / P);
        return domainSize ? onesCount / static_cast<double>(domainSize) : 0;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::to_dpbde
        (mdd_t const& dpbd, log_t const level, index_t const i) -> mdd_t
    {
        auto constexpr U   = log_val_traits<P>::undefined;
        auto constexpr ND  = log_val_traits<P>::nodomain;
        auto const root    = dpbd.get_root();
        auto const rLevel  = vertexManager_.get_vertex_level(root);
        auto const iLevel  = vertexManager_.get_level(i);
        auto const iDomain = this->get_domain(i);

        // Special case when the new vertex for the i-th variable is inserted above the root.
        if (iLevel < rLevel)
        {
            auto const sons = utils::fill_array<P>([=, this](auto const val)
            {
                return val == (level - 1) ? root                              :
                       val < iDomain      ? vertexManager_.terminal_vertex(U) :
                                            vertexManager_.terminal_vertex(ND);
            });
            return mdd_t {vertexManager_.internal_vertex(i, sons)};
        }

        // Normal case for all internal vertices.
        return this->transform(dpbd, [=, this](auto const v, auto&& l_this)
        {
            auto const vLevel = vertexManager_.get_vertex_level(v);
            return utils::fill_array<P>([=, this, &l_this](auto const val) // TODO fill_array_n podla domain
            {
                auto const son    = v->get_son(val);
                auto const sLevel = vertexManager_.get_vertex_level(son);

                if (ND == vertexManager_.get_vertex_value(son)) // TODO nope ND už netreba
                {
                    return son;
                }
                else if (iLevel > vLevel && iLevel < sLevel)
                {
                    return vertexManager_.internal_vertex(i, utils::fill_array<P>([=, this](auto const j) // TODO fill_array_n podla domain
                    {
                        return j == (level - 1) ? son :
                               j < iDomain      ? vertexManager_.terminal_vertex(U) :
                                                  vertexManager_.terminal_vertex(ND);
                    }));
                }
                else
                {
                    return this->transform_step(son, l_this);
                }
            });
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::to_mnf
        (mdd_t const& dpbd) -> mdd_t
    {
        auto const leaf0 = vertexManager_.get_terminal_vertex(0);
        auto const leaf1 = vertexManager_.get_terminal_vertex(1);
        return this->transform(dpbd, [=, this](auto const v, auto&& l_this)
        {
            auto sons = utils::fill_array<P>([=, this](auto const i)
            {
                return this->transform_step(v->get_son(i), l_this);
            });

            for (auto r = 1u; r < P; ++r)
            {
                if (sons[r] == leaf1)
                {
                    std::fill_n(std::begin(sons), r, leaf1);
                }
            }

            // TODO check 0 0 0 1
            //              ^
            for (auto r = 0u; r < P - 1; ++r)
            {
                if (sons[r] == leaf0)
                {
                    sons[r] = sons[r + 1];
                }
            }

            return sons;
        });
    }
}