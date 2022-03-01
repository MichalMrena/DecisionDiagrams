#ifndef TEDDY_RELIABILITY_MANAGER_HPP
#define TEDDY_RELIABILITY_MANAGER_HPP

#include "diagram_manager.hpp"
#include <array>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace teddy
{
    template<class Degree>
    concept is_bss = std::same_as<degrees::fixed<2>, Degree>;

    template<class Probabilities>
    concept component_probabilities =
        requires(Probabilities ps, index_t index, uint_t val)
    {
        { ps[index][val] } -> std::convertible_to<double>;
    };

    template<class F>
    concept f_val_change = requires(F f, uint_t l, uint_t r)
    {
        { f(l, r) } -> std::convertible_to<bool>;
    };

    /**
     *  \struct value_change
     *  \brief Describes change of a value of a variable or a function.
     */
    struct value_change
    {
        uint_t from;
        uint_t to;
    };

    /**
     *  \class reliability_manager
     *  \brief Base class for reliability managers.
     *
     *  Base class for reliability managers. Defines all functions for
     *  reliability analysis.
     */
    template<degree Degree, domain Domain>
    class reliability_manager
        : public diagram_manager<double, Degree, Domain>
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
            , domains::fixed<N> ) -> std::vector<std::array<double, N>>;

        /**
         *  Nonhomogenous system optimized for memory usage.
         *  Probabilities are stored in vector of vectors.
         */
        static auto probabilities
            ( degrees::mixed
            , domains::mixed ) -> std::vector<std::vector<double>>;

        /**
         *  Nonhomogenous system optimized for speed.
         *  Probabilities are stored in vector of arrays.
         */
        template<uint_t N>
        static auto probabilities
            ( degrees::fixed<N>
            , domains::mixed ) -> std::vector<std::array<double, N>>;


        using probabilities_t = decltype(probabilities(Degree(), Domain({})));
        using diagram_t
            = typename diagram_manager<double, Degree, Domain>::diagram_t;

    public:
        /**
         *  \brief Calculates probabilities of all system states.
         *
         *  When used as \c ps[i][k] parameter \p ps must return probability
         *  that i-th component is in state k.
         *  After a call to this method you can acces individual system
         *  state probabilities using \c get_probability method.
         *
         *  \tparam Type that holds component state probabilities.
         *  \param ps Componenet state probabilities.
         *  \param sf Structure function.
         */
        template<component_probabilities Ps>
        auto calculate_probabilities
            (Ps const& ps, diagram_t& sf) -> void;

        /**
         *  \brief Calculates and returns probability of a system state \p j .
         *
         *  When used as \c ps[i][k] parameter \p ps must return probability
         *  that i-th component is in state k.
         *
         *  \tparam Type that holds component state probabilities.
         *  \param j System state.
         *  \param ps Component state probabilities.
         *  \param sf Structure function.
         *  \return Probability that system described by \p sf is in
         *  state \p j given probabilities \p ps .
         */
        template<component_probabilities Ps>
        auto probability ( uint_t     j
                         , Ps const&  ps
                         , diagram_t& sf ) -> double;

        /**
         *  \brief Returns probability of given system state.
         *
         *  Call to \c calculate_probabilities must proceed call to this
         *  funtion otherwise the result is undefined. This is a bit
         *  unfortunate but the idea is that probabilities are calculated
         *  once using \c calculate_probabilities and later accessed using
         *  \c get_probability .
         *
         *  \param j System state.
         *  \return Probability of a system state \p j .
         */
        auto get_probability (uint_t j) const -> double;

        /**
         *  \brief Calculates and returns availability of a BSS.
         *
         *  When used as \c ps[i][k] parameter \p ps must return probability
         *  that i-th component is in state k.
         *
         *  \tparam Component state probabilities.
         *  \tparam Foo Dummy parameter to enable SFINE.
         *  \param ps Component state probabilities.
         *  \param sf Structure function.
         *  \return System availability.
         */
        template< component_probabilities Ps
                , class                   Foo = void> requires (is_bss<Domain>)
        auto availability
            ( Ps const&  ps
            , diagram_t& sf ) -> second_t<Foo, double>;

        /**
         *  \brief Calculates and returns system availability with
         *  respect to the system state \p j .
         *
         *  When used as \c ps[i][k] parameter \p ps must return probability
         *  that i-th component is in state k.
         *
         *  \tparam Component state probabilities.
         *  \param j System state.
         *  \param ps Component state probabilities.
         *  \param sf Structure function.
         *  \return System availability with respect to the system state \p j .
         */
        template<component_probabilities Ps>
        auto availability
            ( uint_t     j
            , Ps const&  ps
            , diagram_t& f ) -> double;

        /**
         *  \brief Returns availability of a BSS.
         *
         *  Call to \c calculate_probabilities must proceed call to this
         *  funtion otherwise the result is undefined. This is a bit
         *  unfortunate but the idea is that probabilities are calculated
         *  once using \c calculate_probabilities and availability and
         *  unavailability are later accessed using \c get_availability
         *  and \c get_unavailability .
         *
         *  \return System availability.
         */
        template<class Foo = void>
        auto get_availability () const -> second_t<Foo, double>;

        /**
         *  \brief Returns system availability with
         *  respect to the system state \p j .
         *
         *  Call to \c calculate_probabilities must proceed call to this
         *  funtion otherwise the result is undefined. This is a bit
         *  unfortunate but the idea is that probabilities are calculated
         *  once using \c calculate_probabilities and availability and
         *  unavailability are later accessed using \c get_availability
         *  and \c get_unavailability .
         *
         *  \param j System state.
         *  \return System availability with respect to the system state \p j .
         */
        auto get_availability (uint_t j) const -> double;

        /**
         *  \brief Calculates and returns unavailability of a BSS.
         *
         *  When used as \c ps[i][k] parameter \p ps must return probability
         *  that i-th component is in state k.
         *
         *  \tparam Component state probabilities.
         *  \tparam Foo Dummy parameter to enable SFINE.
         *  \param ps Component state probabilities.
         *  \param sf Structure function.
         *  \return System unavailtability.
         */
        template< component_probabilities Ps
                , class Foo = void > requires(is_bss<Degree>)
        auto unavailability
            ( Ps const&  ps
            , diagram_t& f) -> second_t<Foo, double>;

        /**
         *  \brief Calculates and returns system availability with
         *  respect to the system state \p j .
         *
         *  When used as \c ps[i][k] parameter \p ps must return probability
         *  that i-th component is in state k.
         *
         *  \tparam Component state probabilities.
         *  \param j System state.
         *  \param ps Component state probabilities.
         *  \param sf Structure function.
         *  \return System availability with respect to the system state \p j .
         */
        template<component_probabilities Ps>
        auto unavailability
            ( uint_t     j
            , Ps const&  ps
            , diagram_t& f) -> double;

        /**
         *  \brief Returns system unavailability of a BSS.
         *
         *  Call to \c calculate_probabilities must proceed call to this
         *  funtion otherwise the result is undefined. This is a bit
         *  unfortunate but the idea is that probabilities are calculated
         *  once using \c calculate_probabilities and availability and
         *  unavailability are later accessed using \c get_availability
         *  and \c get_unavailability .
         *
         *  \param j System state.
         *  \return System availability with respect to the system state \p j .
         */
        template<class Foo = void>
        auto get_unavailability () -> second_t<Foo, double>;

        /**
         *  \brief Returns system unavailability with
         *  respect to the system state \p j .
         *
         *  Call to \c calculate_probabilities must proceed call to this
         *  funtion otherwise the result is undefined. This is a bit
         *  unfortunate but the idea is that probabilities are calculated
         *  once using \c calculate_probabilities and availability and
         *  unavailability are later accessed using \c get_availability
         *  and \c get_unavailability .
         *
         *  \param j System state.
         *  \return System unavailability with respect to
         *  the system state \p j .
         */
        auto get_unavailability (uint_t j) -> double;

        /**
         *  \brief Calculates Direct Partial Boolean Derivative.
         *
         *  \param var Change of the value of \p i th component.
         *  \param f Change of the value of the system.
         *  \param sf Structure function.
         *  \param i Index of the component.
         *  \return Diagram representing Direct Partial Boolean Derivative.
         */
        auto dpbd ( value_change     var
                  , value_change     f
                  , diagram_t const& sf
                  , index_t          i ) -> diagram_t;

        /**
         *  \brief Calculates Direct Partial Boolean Derivative of type 1.
         *
         *  Identifies situation in which change of the state of \p i th
         *  component causes change of the system state from the state \p j
         *  to a worse state.
         *
         *  \param var Change of the value of \p i th component.
         *  \param j System state.
         *  \param sf Structure function.
         *  \param i Index of the component.
         *  \return Diagram representing Direct Partial Boolean Derivative
         *  of type 1.
         */
        auto idpbd_type_1_decrease ( value_change     var
                                   , uint_t           j
                                   , diagram_t const& sf
                                   , index_t          i ) -> diagram_t;

        /**
         *  \brief Calculates Direct Partial Boolean Derivative of type 1.
         *
         *  Identifies situation in which change of the state of \p i th
         *  component causes change of the system state from the state \p j
         *  to a better state.
         *
         *  \param var Change of the value of \p i th component.
         *  \param j System state.
         *  \param sf Structure function.
         *  \param i Index of the component.
         *  \return Diagram representing Direct Partial Boolean Derivative
         *  of type 1.
         */
        auto idpbd_type_1_increase ( value_change     var
                                   , uint_t           j
                                   , diagram_t const& sf
                                   , index_t          i ) -> diagram_t;

        /**
         *  \brief Calculates Direct Partial Boolean Derivative of type 2.
         *
         *  Identifies situation in which change of the state of \p i th
         *  component causes degradation of the system state.
         *
         *  \param var Change of the value of \p i th component.
         *  \param sf Structure function.
         *  \param i Index of the component.
         *  \return Diagram representing Direct Partial Boolean Derivative
         *  of type 2.
         */
        auto idpbd_type_2_decrease ( value_change     var
                                   , diagram_t const& sf
                                   , index_t          i ) -> diagram_t;

        /**
         *  \brief Calculates Direct Partial Boolean Derivative of type 2.
         *
         *  Identifies situation in which change of the state of \p i th
         *  component causes improvement of the system state.
         *
         *  \param var Change of the value of \p i th component.
         *  \param sf Structure function.
         *  \param i Index of the component.
         *  \return Diagram representing Direct Partial Boolean Derivative
         *  of type 2.
         */
        auto idpbd_type_2_increase ( value_change     var
                                   , diagram_t const& sf
                                   , index_t          i ) -> diagram_t;

        /**
         *  Calculate Direct Partial Boolean Derivative of type 3 for
         *  @p i - th variable where @p var describes change in value of
         *  @p i - th variable and @p f describes value of the function
         *  represented by @p sf .
         */
        auto idpbd_type_3_decrease
            ( value_change     var
            , uint_t           f
            , diagram_t const& sf
            , index_t          i ) -> diagram_t;

        /**
         *  Calculate Direct Partial Boolean Derivative of type 3 for
         *  @p i - th variable where @p var describes change in value of
         *  @p i - th variable and @p f describes value of the function
         *  represented by @p sf .
         */
        auto idpbd_type_3_increase
            ( value_change     var
            , uint_t           f
            , diagram_t const& sf
            , index_t          i ) -> diagram_t;

        /**
         *  Calculates structural importace of @p i - th component
         *  using @p dpbd derivative.
         */
        auto structural_importance (diagram_t& dpbd) -> double;

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
        using node_t = typename diagram_manager<double, Degree, Domain>::node_t;

    private:
        auto to_dpbd_e ( uint_t           varFrom
                       , index_t          i
                       , diagram_t const& dpbd ) -> diagram_t;

        // TODO this will be merged with apply_dpbd in the future
        template<f_val_change F>
        auto dpbd_g ( diagram_t const& sf
                    , value_change     var
                    , index_t          i
                    , F                change ) -> diagram_t;

        // TODO toto by mohlo preberat aj zmenu premennej
        // potom by to nebralo dva diagramy ale iba jeden - priamo
        // strukturnu funkciu. Prehladavanie v apply by sa modifikovalo
        // podla zmien premennych.
        template<f_val_change F>
        auto apply_dpbd (diagram_t const&, diagram_t const&, F) -> diagram_t;
    };

    template<degree Degree, domain Domain>
    template<component_probabilities Ps>
    auto reliability_manager<Degree, Domain>::calculate_probabilities
        (Ps const& ps, diagram_t& sf) -> void
    {
        auto const root = sf.get_root();

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
        , diagram_t&   f ) -> double
    {
        this->calculate_probabilities(ps, f);
        return this->get_probability(j);
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::get_probability
        (uint_t const j) const -> double
    {
        auto const n = this->nodes_.get_terminal_node(j);
        return n ? n->data() : 0.0;
    }

    template<degree Degree, domain Domain>
    template<component_probabilities Ps, class Foo> requires (is_bss<Domain>)
    auto reliability_manager<Degree, Domain>::availability
        ( Ps const&  ps
        , diagram_t& f ) -> second_t<Foo, double>
    {
        return this->availability(1, ps, f);
    }

    template<degree Degree, domain Domain>
    template<component_probabilities Ps>
    auto reliability_manager<Degree, Domain>::availability
        ( uint_t const j
        , Ps const&    ps
        , diagram_t&   f ) -> double
    {
        this->calculate_probabilities(ps, f);
        return this->get_availability(j);
    }

    template<degree Degree, domain Domain>
    template<class Foo>
    auto reliability_manager<Degree, Domain>::get_availability
        () const -> second_t<Foo, double>
    {
        auto const node = this->nodes_.get_terminal_node(1);
        return node ? node->data() : 0;
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::get_availability
        (uint_t const j) const -> double
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
        , diagram_t& f ) -> second_t<Foo, double>
    {
        return this->unavailability(1, ps, f);
    }

    template<degree Degree, domain Domain>
    template<component_probabilities Ps>
    auto reliability_manager<Degree, Domain>::unavailability
        ( uint_t const j
        , Ps const&    ps
        , diagram_t&   f ) -> double
    {
        this->calculate_probabilities(ps, f);
        return this->get_unavailability(j);
    }

    template<degree Degree, domain Domain>
    template<class Foo>
    auto reliability_manager<Degree, Domain>::get_unavailability
        () -> second_t<Foo, double>
    {
        return this->get_unavailability(1);
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::get_unavailability
        (uint_t const j) -> double
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
        return this->dpbd_g(sf, var, i, [f](auto const l, auto const r)
        {
            return l == f.from && r == f.to;
        });
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::idpbd_type_1_decrease
        ( value_change     var
        , uint_t           j
        , diagram_t const& sf
        , index_t          i ) -> diagram_t
    {
        return this->dpbd_g(sf, var, i, [j](auto const l, auto const r)
        {
            return l == j && r < j;
        });
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::idpbd_type_1_increase
        ( value_change     var
        , uint_t           j
        , diagram_t const& sf
        , index_t          i ) -> diagram_t
    {
        return this->dpbd_g(sf, var, i, [j](auto const l, auto const r)
        {
            return l > j && r == j;
        });
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::idpbd_type_2_decrease
        ( value_change     var
        , diagram_t const& sf
        , index_t          i ) -> diagram_t
    {
        return this->dpbd_g(sf, var, i, [](auto const l, auto const r)
        {
            return l < r;
        });
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::idpbd_type_2_increase
        ( value_change     var
        , diagram_t const& sf
        , index_t          i ) -> diagram_t
    {
        return this->dpbd_g(sf, var, i, [](auto const l, auto const r)
        {
            return l > r;
        });
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::idpbd_type_3_decrease
        ( value_change const var
        , uint_t const       f
        , diagram_t const&   sf
        , index_t const      i ) -> diagram_t
    {
        return this->dpbd_g(sf, var, i, [f](auto const l, auto const r)
        {
            return l >= f && r < f;
        });
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::idpbd_type_3_increase
        ( value_change const var
        , uint_t const       f
        , diagram_t const&   sf
        , index_t const      i ) -> diagram_t
    {
        return this->dpbd_g(sf, var, i, [f](auto const l, auto const r)
        {
            return l < f && r >= f;
        });
    }

    template<degree Degree, domain Domain>
    auto reliability_manager<Degree, Domain>::structural_importance
        (diagram_t& dpbd) -> double
    {
        auto const from = level_t(0);
        auto const to   = static_cast<level_t>(this->get_var_count());
        auto const domainSize = this->nodes_.domain_product(from, to);
        return static_cast<double>(this->satisfy_count(1, dpbd))
             / static_cast<double>(domainSize);
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
    template<f_val_change F>
    auto reliability_manager<Degree, Domain>::dpbd_g
        ( diagram_t const& sf
        , value_change     var
        , index_t          i
        , F                change ) -> diagram_t
    {
        auto const lhs = this->cofactor(sf, i, var.from);
        auto const rhs = this->cofactor(sf, i, var.to);
        return this->apply_dpbd(lhs, rhs, change);
    }

    template<degree Degree, domain Domain>
    template<f_val_change F>
    auto reliability_manager<Degree, Domain>::apply_dpbd
        (diagram_t const& lhs, diagram_t const& rhs, F change) -> diagram_t
    {
        using cache_pair = struct { node_t* left; node_t* right; };
        auto constexpr cache_pair_hash = [](auto const p)
        {
            auto const hash1 = std::hash<node_t*>()(p.left);
            auto const hash2 = std::hash<node_t*>()(p.right);
            auto result = 0ul;
            result ^= hash1 + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= hash2 + 0x9e3779b9 + (result << 6) + (result >> 2);
            return result;
        };
        auto constexpr cache_pair_equals = [](auto const l, auto const r)
        {
            return l.left == r.left && l.right == r.right;
        };
        auto cache = std::unordered_map< cache_pair
                                       , node_t*
                                       , decltype(cache_pair_hash)
                                       , decltype(cache_pair_equals) >();

        auto const go
            = [this, &cache, change](auto&& self, auto const l, auto const r)
        {
            auto const cached = cache.find(cache_pair(l, r));
            if (cached != std::end(cache))
            {
                return cached->second;
            }

            auto const lhsVal = node_value(l);
            auto const rhsVal = node_value(r);
            auto const opVal // TODO toto by sa dalo lepsie
                = lhsVal == Nondetermined || rhsVal == Nondetermined
                    ? Nondetermined
                    : static_cast<uint_t>(change(lhsVal, rhsVal));
            auto u = static_cast<node_t*>(nullptr);

            if (opVal != Nondetermined)
            {
                u = this->nodes_.terminal_node(opVal);
            }
            else
            {
                auto const lhsLevel = this->nodes_.get_level(l);
                auto const rhsLevel = this->nodes_.get_level(r);
                auto const topLevel = std::min(lhsLevel, rhsLevel);
                auto const topNode  = topLevel == lhsLevel ? l : r;
                auto const topIndex = topNode->get_index();
                auto sons
                    = this->nodes_.make_sons(topIndex, [=, &self](auto const k)
                {
                    auto const fst = lhsLevel == topLevel ? l->get_son(k) : l;
                    auto const snd = rhsLevel == topLevel ? r->get_son(k) : r;
                    return self(self, fst, snd);
                });

                u = this->nodes_.internal_node(topIndex, std::move(sons));
            }

            // TODO in place
            cache.emplace(std::make_pair(cache_pair(l, r), u));
            return u;
        };

        auto const newRoot = go(go, lhs.get_root(), rhs.get_root());
        return diagram_t(newRoot);
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