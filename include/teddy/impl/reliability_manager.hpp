#ifndef TEDDY_RELIABILITY_MANAGER_HPP
#define TEDDY_RELIABILITY_MANAGER_HPP

#include "diagram_manager.hpp"
#include <array>
#include <type_traits>
#include <vector>

namespace teddy
{
    template<class Degree>
    concept is_bss = std::same_as<degrees::fixed<2>, Degree>;

    template<class Probabilities>
    concept component_probabilities = requires(Probabilities ps)
    {
        { ps[index_t()][index_t()] } -> std::convertible_to<double>;
    };

    struct value_change
    {
        uint_t from;
        uint_t to;
    };

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
        template<component_probabilities Ps>
        auto calculate_probabilities
            (Ps const& ps, diagram_t& f) -> void;

        /**
         *  Calculates and returns probability that the system represented
         *  by @p f is in state greater or equal to @p j
         *  given probabilities @p ps .
         */
        template<component_probabilities Ps>
        auto probability ( uint_t     j
                         , Ps const&  ps
                         , diagram_t& f ) -> probability_t;

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
        template< component_probabilities Ps
                , class                   Foo = void> requires (is_bss<Domain>)
        auto availability
            ( Ps const&  ps
            , diagram_t& f ) -> second_t<Foo, probability_t>;

        /**
         *  Calculates and returns probability that system represetned by @p f
         *  is in state greater or equal to @p j given probabilities @p ps .
         */
        template<component_probabilities Ps>
        auto availability
            ( uint_t     j
            , Ps const&  ps
            , diagram_t& f ) -> probability_t;

        /**
         *  Returns probability that system is in state 1.
         *  after call to @c calculate_probabilities .
         *  If there was no call to @c calculate_probabilities then
         *  the result is undefined.
         */
        template<class Foo = void>
        auto get_availability () const -> second_t<Foo, probability_t>;

        /**
         *  Returns probability that system is in state
         *  greater or equal to @p j after call to @c calculate_probabilities .
         *  If there was no call to @c calculate_probabilities then
         *  the result is undefined.
         */
        auto get_availability (uint_t j) const -> probability_t;

        /**
         *  Calculates and returns probability that system represented by @p f
         *  is in state worse than @p j given probabilities @p ps .
         */
        template< component_probabilities Ps
                , class Foo = void > requires(is_bss<Degree>)
        auto unavailability
            ( Ps const&  ps
            , diagram_t& f) -> second_t<Foo, probability_t>;

        /**
         *  Calculates and returns probability that system represented by @p f
         *  is in state worse than @p j given probabilities @p ps .
         */
        template<component_probabilities Ps>
        auto unavailability
            ( uint_t     j
            , Ps const&  ps
            , diagram_t& f) -> probability_t;

        /**
         *  Returns probability that system is in state
         *  0 after call to @c calculate_probabilities .
         *  If there was no call to @c calculate_probabilities then
         *  the result is undefined.
         */
        template<class Foo = void>
        auto get_unavailability () -> second_t<Foo, probability_t>;

        /**
         *  Returns probability that system is in state
         *  worse than @p j after call to @c calculate_probabilities .
         *  If there was no call to @c calculate_probabilities then
         *  the result is undefined.
         */
        auto get_unavailability (uint_t j) -> probability_t;

        /**
         *  Calculates Direct Partial Boolean Derivative for @p i - th variable
         *  where @p var describes change in value of @p i - th variable and
         *  @p f describes change in value of the function represented
         *  by @p sf .
         */
        auto dpbd ( value_change     var
                  , value_change     f
                  , diagram_t const& sf
                  , index_t          i ) -> diagram_t;

        /**
         *  Calculate Direct Partial Boolean Derivative of type 3 for
         *  @p i - th variable where @p var describes change in value of
         *  @p i - th variable and @p f describes value of the function
         *  represented by @p sf .
         */
        auto dpbd_i_3 ( value_change     var
                      , uint_t           f
                      , diagram_t const& sf
                      , index_t          i ) -> diagram_t;

        /**
         *  Calculates Direc Partial Boolean Derivatives ( @c dpbd ) for
         *  each variable.
         */
        auto dpbds ( value_change     var
                   , value_change     f
                   , diagram_t const& sf ) -> std::vector<diagram_t>;

        /**
         *  Calculate Direct Partial Boolean Derivative of
         *  type 3 ( @c dpbd_i_3 ) for each variable.
         */
        auto dpbds_i_3 ( value_change     var
                       , uint_t           f
                       , diagram_t const& sf ) -> std::vector<diagram_t>;

        /**
         *  Calculates structural importace of @p i - th component
         *  using @p dpbd derivative.
         */
        auto structural_importance (diagram_t& dpbd) -> probability_t;

        /**
         *  Calculates structural importaces for all components using
         *  their derivatives @p dpbds .
         */
        auto structural_importances
            (std::vector<diagram_t>& dpbds) -> std::vector<probability_t>;

        /**
         *  Finds all Minimal Cut Vectors of the system described by @p sf
         *  with respect to state @p j . MCVs are stored and returned
         *  in a vector.
         */
        template<out_var_values Vars>
        auto mcvs ( diagram_t const& sf
                  , uint_t           j ) -> std::vector<Vars>;

        /**
         *  Finds all Minimal Cut Vectors of the system described by @p sf
         *  with respect to state @p j . MCVs are outputed
         *  via output iterator @p out .
         */
        template<out_var_values Vars, std::output_iterator<Vars> Out>
        auto mcvs_g ( diagram_t const& sf
                    , uint_t           j
                    , Out              out ) -> void;

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

    private:
        auto to_dpbd_e ( uint_t           varFrom
                       , index_t          i
                       , diagram_t const& dpbd ) -> diagram_t;
    };

    template<degree Degree, domain Domain>
    template<component_probabilities Ps>
    auto reliability_manager<Degree, Domain>::calculate_probabilities
        (Ps const& ps, diagram_t& f) -> void
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
                this->nodes_.for_each_son(node, [this, node, nodeIndex, &ps, &k]
                    (auto const son)
                {
                    son->data() += node->data() * ps[nodeIndex][k];
                    ++k;
                });
            }
        });
    }

    template<degree Degree, domain Domain>
    template<component_probabilities Ps>
    auto reliability_manager<Degree, Domain>::probability
        ( uint_t const j
        , Ps const&    ps
        , diagram_t&   f ) -> probability_t
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
    template<component_probabilities Ps, class Foo> requires (is_bss<Domain>)
    auto reliability_manager<Degree, Domain>::availability
        ( Ps const&  ps
        , diagram_t& f ) -> second_t<Foo, probability_t>
    {
        return this->availability(1, ps, f);
    }

    template<degree Degree, domain Domain>
    template<component_probabilities Ps>
    auto reliability_manager<Degree, Domain>::availability
        ( uint_t const j
        , Ps const&    ps
        , diagram_t&   f ) -> probability_t
    {
        this->calculate_probabilities(ps, f);
        return this->get_availability(j);
    }

    template<degree Degree, domain Domain>
    template<class Foo>
    auto reliability_manager<Degree, Domain>::get_availability
        () const -> second_t<Foo, probability_t>
    {
        auto const node = this->nodes_.get_terminal_node(1);
        return node ? node->data() : 0;
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
    template<component_probabilities Ps, class Foo> requires(is_bss<Degree>)
    auto reliability_manager<Degree, Domain>::unavailability
        ( Ps const&  ps
        , diagram_t& f ) -> second_t<Foo, probability_t>
    {
        return this->unavailability(1, ps, f);
    }

    template<degree Degree, domain Domain>
    template<component_probabilities Ps>
    auto reliability_manager<Degree, Domain>::unavailability
        ( uint_t const j
        , Ps const&    ps
        , diagram_t&   f ) -> probability_t
    {
        this->calculate_probabilities(ps, f);
        return this->get_unavailability(j);
    }

    template<degree Degree, domain Domain>
    template<class Foo>
    auto reliability_manager<Degree, Domain>::get_unavailability
        () -> second_t<Foo, probability_t>
    {
        return this->get_unavailability(1);
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::get_unavailability
        (uint_t const j) -> probability_t
    {
        auto U = .0;
        this->nodes_.for_each_terminal_node([j, &U](auto const node)
        {
            if (node->get_value() < j)
            {
                U += node->data();
            }
        });
        return U;
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::dpbd
        ( value_change const var
        , value_change const f
        , diagram_t const&   sf
        , index_t const      i ) -> diagram_t
    {
        auto const lhs = this->template apply<ops::EQUAL_TO>
            ( this->cofactor(sf, i, var.from)
            , this->constant(f.from) );
        auto const rhs = this->template apply<ops::EQUAL_TO>
            ( this->cofactor(sf, i, var.to)
            , this->constant(f.to) );
        return this->template apply<ops::AND>(lhs, rhs);
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::dpbd_i_3
        ( value_change const var
        , uint_t const       f
        , diagram_t const&   sf
        , index_t const      i ) -> diagram_t
    {
        auto const fVal = this->constant(f);
        auto const cofactorFrom = this->cofactor(sf, i, var.from);
        auto const cofactorTo = this->cofactor(sf, i, var.to);
        if (var.from < var.to)
        {
            return this->template apply<ops::AND>
                ( this->template apply<ops::LESS>(cofactorFrom, fVal)
                , this->template apply<ops::GREATER_EQUAL>(cofactorTo, fVal) );
        }
        else
        {
            return this->template apply<ops::AND>
                ( this->template apply<ops::GREATER_EQUAL>(cofactorFrom, fVal)
                , this->template apply<ops::LESS>(cofactorTo, fVal) );
        }
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::dpbds
        ( value_change const var
        , value_change const f
        , diagram_t const&   sf ) -> std::vector<diagram_t>
    {
        return utils::fill_vector(this->get_var_count(), [&](auto const i)
        {
            return this->dpbd(var, f, sf, i);
        });
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::dpbds_i_3
        ( value_change const var
        , uint_t const       f
        , diagram_t const&   sf ) -> std::vector<diagram_t>
    {
        return utils::fill_vector(this->get_var_count(), [&](auto const i)
        {
            return this->dpbd_i_3(var, f, sf, i);
        });
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::structural_importance
        (diagram_t& dpbd) -> probability_t
    {
        auto const from = level_t(0);
        auto const to   = static_cast<level_t>(this->get_var_count() - 1);
        auto const domainSize = this->nodes_.domain_product(from, to);
        return static_cast<probability_t>(this->satisfy_count(1, dpbd))
             / static_cast<probability_t>(domainSize);
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::structural_importances
        (std::vector<diagram_t>& dpbds) -> std::vector<probability_t>
    {
        auto const from = 0;
        auto const to   = this->get_var_count() - 1;
        auto const domainSize = this->nodes_.domain_product(from, to);
        return utils::fill_vector(this->get_var_count(), [&](auto const i)
        {
            return static_cast<probability_t>(this->satisfy_count(dpbd, 1))
                 / static_cast<probability_t>(domainSize);
        });
    }

    template<degree Degree, domain Domain>
    template<out_var_values Vars>
    auto reliability_manager<Degree, Domain>::mcvs
        (diagram_t const& sf, uint_t const j) -> std::vector<Vars>
    {
        auto cuts = std::vector<Vars>();
        this->mcvs_g<Vars>(sf, j, std::back_inserter(cuts));
        return cuts;
    }

    template<degree Degree, domain Domain>
    template<out_var_values Vars, std::output_iterator<Vars> Out>
    auto reliability_manager<Degree, Domain>::mcvs_g
        (diagram_t const& sf, uint_t const j, Out out) -> void
    {
        // auto const varCount = manager_.get_var_count();
        // auto dpbdes = std::vector<diagram_t>();

        // for (auto varIndex = 0u; varIndex < varCount; ++varIndex)
        // {
        //     auto const varDomain = this->nodes_.get_domain(varIndex);
        //     for (auto varFrom = 0u; varFrom < varDomain - 1; ++varFrom)
        //     {
        //         auto const varChange = {varFrom, varFrom + 1};
        //         auto const dpbd = this->dpbd_i_3(varChange, j, sf, varIndex);
        //         dpbdes.emplace_back(this->to_dpbd_e(varFrom, varIndex, dpbd));
        //     }
        // }

        // auto const conj = this->template tree_fold<ops::PI_CONJ>(dpbdes);
        // this->template satisfy_all_g<Vars, Out>(1, conj, out);
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