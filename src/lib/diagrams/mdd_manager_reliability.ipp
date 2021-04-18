#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

namespace teddy
{
    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::calculate_probabilities
        (prob_table const& ps, mdd_t& f) -> void
    {
        manager_.for_each_terminal_vertex([](auto const v)
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
        auto const v = manager_.terminal_vertex(level);
        return v ? v->data : 0.0;
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
        return this->apply<AND>( this->apply<EQUAL_TO>(this->cofactor(sf, i, var.from), this->constant(f.from))
                               , this->apply<EQUAL_TO>(this->cofactor(sf, i, var.to), this->constant(f.to)) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbd_integrated_1
        (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i) -> mdd_t
    {
        return this->apply<AND>( this->apply<EQUAL_TO>(this->cofactor(sf, i, var.from), this->constant(fVal))
                               , this->apply<LESS>(this->cofactor(sf, i, var.to), this->constant(fVal)) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbd_integrated_2
        (val_change<P> const var, mdd_t const& sf, index_t const i) -> mdd_t
    {
        return this->apply<GREATER>( this->cofactor(sf, i, var.from)
                                   , this->cofactor(sf, i, var.to) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbd_integrated_3
        (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i) -> mdd_t
    {
        auto const fValConstant = this->constant(fVal);
        if (var.from < var.to)
        {
            return this->apply<AND>( this->apply<LESS>(this->cofactor(sf, i, var.from), fValConstant)
                                   , this->apply<GREATER_EQUAL>(this->cofactor(sf, i, var.to), fValConstant) );
        }
        else
        {
            return this->apply<AND>( this->apply<GREATER_EQUAL>(this->cofactor(sf, i, var.from), fValConstant)
                                   , this->apply<LESS>(this->cofactor(sf, i, var.to), fValConstant) );
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds
        (val_change<P> const var, val_change<P> const f, mdd_t const& sf) -> std::vector<mdd_t>
    {
        return utils::fill_vector( manager_.get_var_count()
                                 , std::bind_front(&mdd_manager::dpbd, this, var, f, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds_integrated_1
        (val_change<P> const var, log_t const fVal, mdd_t const& sf) -> std::vector<mdd_t>
    {
        return utils::fill_vector( manager_.get_var_count()
                                 , std::bind_front(&mdd_manager::dpbd_integrated_1, this, var, fVal, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds_integrated_2
        (val_change<P> const var, mdd_t const& sf) -> std::vector<mdd_t>
    {
        return utils::fill_vector( manager_.get_var_count()
                                 , std::bind_front(&mdd_manager::dpbd_integrated_2, this, var, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::dpbds_integrated_3
        (val_change<P> const var, log_t const fVal, mdd_t const& sf) -> std::vector<mdd_t>
    {
        return utils::fill_vector( manager_.get_var_count()
                                 , std::bind_front(&mdd_manager::dpbd_integrated_3, this, var, fVal, sf) );
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::structural_importance
        (mdd_t& dpbd, index_t const i) -> double
    {
        auto const domProduct = this->domain_product(0, manager_.get_var_count() - 1);
        return this->structural_importance(domProduct / manager_.get_domain(i), dpbd);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::structural_importances
        (std::vector<mdd_t>& dpbds) -> std::vector<double>
    {
        auto const domProduct = this->domain_product(0, manager_.get_var_count() - 1);
        return utils::fmap_i(dpbds, [=, this](auto const i, auto& d)
        {
            return this->structural_importance(domProduct / manager_.get_domain(i), d);
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
        (prob_table const& ps, std::vector<mdd_t>& dpbds) -> std::vector<double>
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
        (prob_table const& ps, double const U, std::vector<mdd_t> const& dpbds) -> std::vector<double>
    {
        return utils::fmap(dpbds, std::bind_front(&mdd_manager::fussell_vesely_importance, this, ps, U));
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class SetIthVar>
    auto mdd_manager<VertexData, ArcData, P>::mcvs
        (mdd_t const& sf, log_t const logLevel) -> std::vector<VariableValues>
    {
        auto cs = std::vector<VariableValues>();
        this->template mcvs_g<VariableValues>(sf, logLevel, std::back_inserter(cs));
        return cs;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VariableValues, class OutputIt, class SetIthVar>
    auto mdd_manager<VertexData, ArcData, P>::mcvs_g
        (mdd_t const& sf, log_t const logLevel, OutputIt out) -> void
    {
        auto const varCount = manager_.get_var_count();
        auto dpbdes = std::vector<mdd_t>();

        for (auto varIndex = 0u; varIndex < varCount; ++varIndex)
        {
            auto const varDomain = manager_.get_domain(varIndex);
            for (auto varFrom = 0u; varFrom < varDomain - 1; ++varFrom)
            {
                auto const dpbd = this->dpbd_integrated_3({varFrom, varFrom + 1}, logLevel, sf, varIndex);
                dpbdes.emplace_back(this->to_dpbd_e(varFrom, varIndex, dpbd));
            }
        }
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
    auto mdd_manager<VertexData, ArcData, P>::to_dpbd_e
        (log_t const varFrom, index_t const varIndex, mdd_t const& dpbd) -> mdd_t
    {
        auto constexpr U     = log_val_traits<P>::undefined;
        auto const root      = dpbd.get_root();
        auto const rootLevel = manager_.get_vertex_level(root);
        auto const varLevel  = manager_.get_level(varIndex);
        auto const varDomain = manager_.get_domain(varIndex);

        // Special case when the new vertex for the i-th variable is inserted above the root.
        if (varLevel < rootLevel)
        {
            auto const sons = utils::fill_array_n<P>(varDomain, [=, this](auto const val)
            {
                return val == varFrom ? root : manager_.terminal_vertex(U);
            });
            return mdd_t {manager_.internal_vertex(varIndex, sons)};
        }

        // Normal case for all internal vertices.
        return this->transform(dpbd, [=, this](auto&& l_this, auto const v)
        {
            auto const vertexLevel  = manager_.get_vertex_level(v);
            auto const vertexDomain = manager_.get_domain(v->get_index());
            return utils::fill_array_n<P>(vertexDomain, [=, this, &l_this](auto const val)
            {
                auto const son      = v->get_son(val);
                auto const sonLevel = manager_.get_vertex_level(son);

                // This means that the new vertex goes in between current vertex and its val-th son.
                if (varLevel > vertexLevel && varLevel < sonLevel)
                {
                    auto const newSons = utils::fill_array_n<P>(varDomain, [=, this](auto const j)
                    {
                        return j == varFrom ? son : manager_.terminal_vertex(U);
                    });
                    return manager_.internal_vertex(varIndex, newSons);
                }
                else
                {
                    // New vertex will be inserted somewhere deeper.
                    return this->transform_step(l_this, son);
                }
            });
        });
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::to_mnf
        (mdd_t const& dpbd) -> mdd_t
    {
        auto const leaf0 = manager_.terminal_vertex(0);
        auto const leaf1 = manager_.terminal_vertex(1);
        return this->transform(dpbd, [=, this](auto const v, auto&& l_this)
        {
            auto sons = utils::fill_array<P>([=, this](auto const i)
            {
                return this->transform_step(v->get_son(i), l_this);
            });

            for (auto r = P; r > 0;)
            {
                --r;
                if (r > 0 && sons[r] == leaf1)
                {
                    std::fill_n(std::begin(sons), r, leaf1);
                    break;
                }

                if (r < P - 1 && sons[r] == leaf0)
                {
                    sons[r] = sons[r + 1];
                }
            }

            // TODO check 0 0 0 1
            //              ^
            // for (auto r = 0u; r < P - 1; ++r)
            // {
            //     if (sons[r] == leaf0)
            //     {
            //         sons[r] = sons[r + 1];
            //     }
            // }

            return sons;
        });
    }
}