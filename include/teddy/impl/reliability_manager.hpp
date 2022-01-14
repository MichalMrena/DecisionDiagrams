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

        /**
         *  Homogenous system.
         *  Probabilities are stored in vector of arrays.
         */
        template<uint_t N>
        static auto probabilities
            ( degrees::fixed<N>
            , domains::fixed<N> ) -> std::vector<std::array<probability_t, N>>;

        /**
         *  Nonhomogenous system optimized for memory usage.
         *  Probabilities are stored in vector of vectors.
         */
        static auto probabilities
            ( degrees::mixed
            , domains::mixed ) -> std::vector<std::vector<probability_t>>;

        /**
         *  Nonhomogenous system optimized for speed.
         *  Probabilities are stored in vector of arrays.
         */
        template<uint_t N>
        static auto probabilities
            ( degrees::fixed<N>
            , domains::mixed ) -> std::vector<std::array<probability_t, N>>;


        using probabilities_t = decltype(probabilities(Degree(), Domain({})));
        using diagram_t
            = typename diagram_manager<double, Degree, Domain>::diagram_t;

    public:
        /**
         *  Calculates probability of reaching each terminal node
         *  of @p f given state probabilities @p ps .
         */
        auto calculate_probabilities
            (probabilities_t ps, diagram_t& f) -> void;

        /**
         *  Calculates and returns probability that the system represented
         *  by @p f is in state greater or equal to @p j
         *  given probabilities @p ps .
         */
        auto probability ( uint_t                 j
                         , probabilities_t const& ps
                         , diagram_t&             f ) -> probability_t;

        /**
         *  Returns probability stored in terminal node representing value @p j
         *  after call to @c calculate_probabilities .
         *  Zero is returned if there is no such terminal node.
         *  If there was no call to @c calculate_probabilities then
         *  the result is undefined.
         */
        auto get_probability (uint_t j) const -> probability_t;

        /**
         *  Calculates and returns probability that system represented
         *  by @p f is in state 1 given probabilities @p ps .
         *  Available only for BSS.
         */
        template<class Foo = void> requires (is_bss_v<Domain>)
        auto availability
            ( probabilities_t const& ps
            , diagram_t&             f) -> second_t<Foo, probability_t>;

        /**
         *  Calculates and returns probability that system represetned by @p f
         *  is in state greater or equal to @p j given probabilities @p ps .
         */
        auto availability
            ( uint_t                 j
            , probabilities_t const& ps
            , diagram_t&             f) -> probability_t;

        /**
         *  Returns probability that system is in state
         *  greater or equal to @p j after call to @c calculate_probabilities .
         *  If there was no call to @c calculate_probabilities then
         *  the result is undefined.
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

        this->nodes_.traverse_level(root, [this, &ps](auto const node)
        {
            if (node->is_internal())
            {
                auto const nodeIndex = node->get_index();
                auto k = 0u;
                this->nodes_.for_each_son(n, [this, node, nIndex, &ps, &k]
                    (auto const son)
                {
                    son->data() += n->data() * ps[nodeIndex][k];
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
        this->nodes_.for_each_terminal_node([j, &A](auto const node)
        {
            if (node->get_value() >= j)
            {
                A += node->data();
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