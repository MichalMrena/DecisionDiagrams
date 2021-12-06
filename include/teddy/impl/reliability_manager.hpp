#ifndef TEDDY_RELIABILITY_MANAGER_HPP
#define TEDDY_RELIABILITY_MANAGER_HPP

#include "diagram_manager.hpp"
#include <array>
#include <type_traits>
#include <vector>

namespace teddy
{
    template<class...>
    struct is_bss : public std::false_type
    {
    };

    template<>
    struct is_bss<domains::fixed<2>> : public std::true_type
    {
    };

    template<class Domain>
    inline constexpr auto is_bss_v = is_bss<Domain>()();

    template<degree Degree, domain Domain>
    class reliability_manager : public diagram_manager<double, Degree, Domain>
    {
    public:
        using probability_t = double;

        template<uint_t N>
        static auto probabilities
            ( degrees::fixed<N>
            , domains::fixed<N> ) -> std::vector<std::array<probability_t, N>>;

        static auto probabilities
            ( degrees::mixed
            , domains::mixed ) -> std::vector<std::vector<probability_t>>;

        template<uint_t N>
        static auto probabilities
            ( degrees::fixed<N>
            , domains::mixed ) -> std::vector<std::array<probability_t, N>>;


        using probabilities_t = decltype(probabilities(Degree(), Domain({})));
        using diagram_t = typename diagram_manager<double, Degree, Domain>::diagram_t; // TODO moc dlhe

    public:
        /**
         *  Calculates probability of reaching each terminal node
         *  of @p f given state probabilities @p ps .
         */
        auto calculate_probabilities
            (probabilities_t ps, diagram_t& f) -> void;

        /**
         *  Calculates and returns probability that system is in state
         *  greater or equal to @p j .
         */
        auto probability
            (uint_t j, probabilities_t const&, diagram_t&) -> probability_t;

        /**
         *  Returns probability that system is in state @p j .
         *  Call to @c calculate_probabilities must precede this call.
         */
        auto get_probability (uint_t) const -> probability_t;

        /**
         *  Calculates and returns probability that system is in state 1.
         *  Available only for BSS.
         */
        template<class Foo = void> requires (is_bss_v<Domain>)
        auto availability
            (probabilities_t, diagram_t&) -> second_t<Foo, probability_t>;

        /**
         *  Calculates and returns probability that system is in state
         *  greater or equal to @p j .
         */
        auto availability
            (uint_t j, probabilities_t const&, diagram_t&) -> probability_t;

        /**
         *  Returns probability that system is in state
         *  greater or equal to @p j .
         *  Call to @c calculate_probabilities must precede this call.
         */
        auto get_availability (uint_t j) const -> probability_t;

    protected:
        reliability_manager ( std::size_t vars
                            , std::size_t nodes
                            , std::vector<index_t> order )
                            requires(domains::is_fixed<Domain>()());

        reliability_manager ( std::size_t vars
                            , std::size_t nodes
                            , domains::mixed
                            , std::vector<index_t> order )
                            requires(domains::is_mixed<Domain>()());
    };

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::calculate_probabilities
        (probabilities_t ps, diagram_t& f) -> void
    {
        auto const root = f.get_root();

        this->nodes_.traverse_pre(root, [](auto const n)
        {
            n->data() = 0.0;
        });
        this->nodes_.for_each_terminal_node([](auto const n)
        {
            n->data() = 0.0;
        });
        root->data() = 1.0;

        this->nodes_.traverse_level(root, [this, &ps](auto const n)
        {
            if (n->is_internal())
            {
                auto const nIndex = n->get_index();
                auto k = 0u;
                this->nodes_.for_each_son(n, [this, n, nIndex, &ps, &k]
                    (auto const son)
                {
                    son->data() += n->data() * ps[nIndex][k];
                    ++k;
                });
            }
        });
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::probability
        ( uint_t const           j
        , probabilities_t const& ps
        , diagram_t&             f ) -> probability_t
    {
        this->calculate_probabilities(ps, f);
        return this->get_probability(j);
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::get_probability
        (uint_t const j) const -> probability_t
    {
        auto const n = this->nodes_.get_terminal_node(j);
        return n ? n->data() : 0.0;
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::availability
        ( uint_t const           j
        , probabilities_t const& ps
        , diagram_t&             f ) -> probability_t
    {
        this->calculate_probabilities(ps, f);
        return this->get_availability(j);
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::get_availability
        (uint_t const j) const -> probability_t
    {
        auto A = .0;
        this->nodes_.for_each_terminal_node([j, &A](auto const n)
        {
            if (n->get_value() >= j)
            {
                A += n->data();
            }
        });
        return A;
    }

    template<degree Degree, domain Domain>
    reliability_manager<Degree, Domain>::reliability_manager
        ( std::size_t          vars
        , std::size_t          nodes
        , std::vector<index_t> order )
        requires(domains::is_fixed<Domain>()()) :
        diagram_manager<double, Degree, Domain>
            (vars, nodes, std::move(order))
    {
    }

    template<degree Degree, domain Domain>
    reliability_manager<Degree, Domain>::reliability_manager
        ( std::size_t          vars
        , std::size_t          nodes
        , domains::mixed       ds
        , std::vector<index_t> order )
        requires(domains::is_mixed<Domain>()()) :
        diagram_manager<double, Degree, Domain>
            (vars, nodes, std::move(ds), std::move(order))
    {
    }
}

#endif