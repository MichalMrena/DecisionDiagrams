#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::calculate_probabilities
        (mdd_t& f, prob_table const& ps) -> void
    {
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
        (mdd_t& f, log_t const level, prob_table const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_availability(level);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::unavailability
        (mdd_t& f, log_t const level, prob_table const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_unavailability(level);
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