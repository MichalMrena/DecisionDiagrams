#ifndef MIX_DD_MDD_RELIABILITY_HPP
#define MIX_DD_MDD_RELIABILITY_HPP

#include "bdd.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    class mdd_reliability
    {
    public:
        using mdd_t       = std::conditional_t< 2 == P
                                              , bdd<VertexData, ArcData, Allocator>
                                              , mdd<VertexData, ArcData, P, Allocator> >;
        using vertex_t   = typename mdd_t::vertex_t;
        using arc_t      = typename mdd_t::arc_t;
        using log_t      = typename log_val_traits<P>::type;
        using prob_table = std::vector<std::array<double, P>>;

    public:
        explicit mdd_reliability (Allocator const& alloc = Allocator {});

        auto calculate_probabilities (mdd_t& f, prob_table const& ps)                     -> void;
        auto get_probability         (mdd_t const& f, log_t const level)                  -> double;
        auto get_availability        (mdd_t const& f, log_t const level)                  -> double;
        auto get_unavailability      (mdd_t const& f, log_t const level)                  -> double;
        auto availability            (mdd_t&  f, log_t const level, prob_table const& ps) -> double;
        auto availability            (mdd_t&& f, log_t const level, prob_table const& ps) -> double;
        auto unavailability          (mdd_t&  f, log_t const level, prob_table const& ps) -> double;
        auto unavailability          (mdd_t&& f, log_t const level, prob_table const& ps) -> double;

    private:
        auto sum_terminals (mdd_t const& f, log_t const from, log_t const to) -> double;

    protected:
        using manager_t = utils::alloc_manager<Allocator>;

    protected:
        manager_t manager_;
    };

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    mdd_reliability<VertexData, ArcData, P, Allocator>::mdd_reliability
        (Allocator const& alloc) :
        manager_ {alloc}
    {
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::calculate_probabilities
        (mdd_t& f, prob_table const& ps) -> void
    {
        for (auto& vertex : f)
        {
            vertex.data = 0.0;
        }
        f.get_root()->data = 1.0;

        for (auto& vertex : f)
        {
            if (!f.is_leaf(&vertex))
            {
                for (auto i = 0u; i < P; ++i)
                {
                    vertex.son(i)->data += vertex.data * ps[vertex.index][i];
                }
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::get_probability
        (mdd_t const& f, log_t const level) -> double
    {
        return f.get_leaf(level)->data;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::get_availability
        (mdd_t const& f, log_t const level) -> double
    {
        return this->sum_terminals(f, level, P);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::get_unavailability
        (mdd_t const& f, log_t const level) -> double
    {
        return this->sum_terminals(f, 0, level);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::availability
        (mdd_t& f, log_t const level, prob_table const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_availability(f, level);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::availability
        (mdd_t&& f, log_t const level, prob_table const& ps) -> double
    {
        return this->get_availability(f, level, ps);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::unavailability
        (mdd_t& f, log_t const level, prob_table const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_unavailability(f, level);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::unavailability
        (mdd_t&& f, log_t const level, prob_table const& ps) -> double
    {
        return this->unavailability(f, level, ps);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::sum_terminals
        (mdd_t const& f, log_t const from, log_t const to) -> double
    {
        auto sumval = 0.0;
        for (auto i = from; i < to; ++i)
        {
            sumval += this->get_probability(f, i);
        }
        return sumval;
    }
}

#endif