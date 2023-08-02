#ifndef LIBTEDDY_DETAILS_RELIABILITY_MANAGER_HPP
#define LIBTEDDY_DETAILS_RELIABILITY_MANAGER_HPP

#include <libteddy/details/diagram_manager.hpp>

#include <array>
#include <concepts>
#include <iostream>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace teddy
{
template<class Degree>
concept is_bss = std::same_as<degrees::fixed<2>, Degree>;

template<class Probabilities>
concept component_probabilities
    = requires(Probabilities probs, int32 index, int32 value) {
          {
              probs[index][value]
          } -> std::convertible_to<double>;
      };

namespace dpld
{
/**
 *  \brief Returns lambda that can be used in basic \c dpld
 */
inline static auto constexpr basic = [] (int32 const fFrom, int32 const fTo)
{
    return [=] (int32 const lhs, int32 const rhs)
    {
        return lhs == fFrom && rhs == fTo;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 1
 */
inline static auto constexpr type_1_decrease = [] (int32 const state)
{
    return [state] (int32 const lhs, int32 const rhs)
    {
        return lhs == state && rhs < state;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 1
 */
inline static auto constexpr type_1_increase = [] (int32 const state)
{
    return [state] (int32 const lhs, int32 const rhs)
    {
        return lhs == state && rhs > state;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 2
 */
inline static auto constexpr type_2_decrease = [] ()
{
    return [] (int32 const lhs, int32 const rhs)
    {
        return lhs > rhs;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 2
 */
inline static auto constexpr type_2_increase = [] ()
{
    return [] (int32 const lhs, int32 const rhs)
    {
        return lhs < rhs;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 3
 */
inline static auto constexpr type_3_decrease = [] (int32 const state)
{
    return [state] (int32 const lhs, int32 const rhs)
    {
        return lhs >= state && rhs < state;
    };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 3
 */
inline static auto constexpr type_3_increase = [] (int32 const state)
{
    return [state] (int32 const lhs, int32 const rhs)
    {
        return lhs < state && rhs >= state;
    };
};
} // namespace dpld

/**
 *  \struct var_change
 *  \brief Describes change in a value of a variable
 */
struct var_change
{
    int32 index_;
    int32 from_;
    int32 to_;
};

/**
 *  \class reliability_manager
 *  \brief Base class for reliability managers.
 *
 *  Base class for reliability managers. Defines all functions for
 *  reliability analysis.
 */
template<class Degree, class Domain>
class reliability_manager : public diagram_manager<double, Degree, Domain>
{
public:
    using diagram_t =
        typename diagram_manager<double, Degree, Domain>::diagram_t;

public:
    /**
     *  \brief Calculates probabilities of all system states
     *
     *  When used as \c probs[i][k] parameter \p probs must return probability
     *  that i-th component is in state k.
     *  After a call to this method you can acces individual system
     *  state probabilities using \c get_probability method.
     *
     *  \tparam Type that holds component state probabilities
     *  \param probs Componenet state probabilities
     *  \param diagram Structure function
     */
    template<component_probabilities Ps>
    auto calculate_probabilities (Ps const& probs, diagram_t const& diagram)
        -> void;

    /**
     *  \brief Calculates and returns probability of a system state \p state
     *
     *  When used as \c probs[i][k] parameter \p probs must return probability
     *  that i-th component is in state k.
     *
     *  \tparam Type that holds component state probabilities
     *  \param state System state
     *  \param probs Component state probabilities
     *  \param diagram Structure function
     *  \return Probability that system described by \p diagran is in
     *  state \p state given probabilities \p probs
     */
    template<component_probabilities Ps>
    auto calculate_probability (
        int32 state,
        Ps const& probs,
        diagram_t const& diagram
    ) -> double;

    /**
     *  \brief Returns probability of given system state.
     *
     *  Call to \c calculate_probabilities must proceed call to this
     *  funtion otherwise the result is undefined. This is a bit
     *  unfortunate but the idea is that probabilities are calculated
     *  once using \c calculate_probabilities and later accessed using
     *  \c get_probability .
     *
     *  \param state System state.
     *  \return Probability of a system state \p state
     */
    [[nodiscard]] auto get_probability (int32 state) const -> double;

    /**
     *  \brief Calculates and returns availability of a BSS.
     *
     *  When used as \c ps[i][k] parameter \p ps must return probability
     *  that i-th component is in state k.
     *
     *  \tparam Component state probabilities
     *  \tparam Foo Dummy parameter to enable SFINE
     *  \param probs Component state probabilities
     *  \param diagram Structure function
     *  \return System availability
     */
    template<component_probabilities Ps, class Foo = void>
    requires(is_bss<Degree>)
    auto calculate_availability (Ps const& probs, diagram_t const& diagram)
        -> utils::second_t<Foo, double>;

    /**
     *  \brief Calculates and returns system availability with
     *  respect to the system state \p state
     *
     *  When used as \c probs[i][k] parameter \p probs must return probability
     *  that i-th component is in state k.
     *
     *  \tparam Component state probabilities
     *  \param state System state
     *  \param probs Component state probabilities
     *  \param diagram Structure function
     *  \return System availability with respect to the system state \p state
     */
    template<component_probabilities Ps>
    auto calculate_availability (
        int32 state,
        Ps const& probs,
        diagram_t const& diagram
    ) -> double;

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
    [[nodiscard]] auto get_availability () const -> utils::second_t<Foo, double>;

    /**
     *  \brief Returns system availability with
     *  respect to the system state \p state
     *
     *  Call to \c calculate_probabilities must proceed call to this
     *  funtion otherwise the result is undefined. This is a bit
     *  unfortunate but the idea is that probabilities are calculated
     *  once using \c calculate_probabilities and availability and
     *  unavailability are later accessed using \c get_availability
     *  and \c get_unavailability
     *
     *  \param state System state
     *  \return System availability with respect to the system state \p state
     */
    [[nodiscard]] auto get_availability (int32 state) const -> double;

    /**
     *  \brief Calculates and returns unavailability of a BSS.
     *
     *  When used as \c probs[i][k] parameter \p probs must return probability
     *  that i-th component is in state k
     *
     *  \tparam Component state probabilities
     *  \tparam Foo Dummy parameter to enable SFINE
     *  \param probs Component state probabilities
     *  \param diagran Structure function
     *  \return System unavailtability
     */
    template<component_probabilities Ps, class Foo = void>
    requires(is_bss<Degree>)
    auto calculate_unavailability (Ps const& probs, diagram_t const& diagram)
        -> utils::second_t<Foo, double>;

    /**
     *  \brief Calculates and returns system availability with
     *  respect to the system state \p state
     *
     *  When used as \c probs[i][k] parameter \p probs must return probability
     *  that i-th component is in state k
     *
     *  \tparam Component state probabilities
     *  \param state System state
     *  \param probs Component state probabilities
     *  \param diagram Structure function
     *  \return System availability with respect to the system state \p state
     */
    template<component_probabilities Ps>
    auto calculate_unavailability (
        int32 state,
        Ps const& probs,
        diagram_t const& diagram
    ) -> double;

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
     *  \return System availability
     */
    template<class Foo = void>
    auto get_unavailability () -> utils::second_t<Foo, double>;

    /**
     *  \brief Returns system unavailability with
     *  respect to the system state \p state
     *
     *  Call to \c calculate_probabilities must proceed call to this
     *  funtion otherwise the result is undefined. This is a bit
     *  unfortunate but the idea is that probabilities are calculated
     *  once using \c calculate_probabilities and availability and
     *  unavailability are later accessed using \c get_availability
     *  and \c get_unavailability
     *
     *  \param state System state
     *  \return System unavailability with respect to
     *  the system state \p state
     */
    [[nodiscard]] auto get_unavailability (int32 state) -> double;

    /**
     *  \brief Returns system state frequency of state \p state
     *  \param diagram Structure function
     *  \param state System state
     *  \return Frequency of system state \p state
     */
    auto state_frequency (diagram_t const& diagram, int32 state) -> double;

    /**
     *  \brief Calculates Direct Partial Boolean Derivative
     *  \param varChange Change of the value of a variable
     *  \param fChange Change of the value of the system
     *  \param diagram Structure function
     *  \return Diagram representing Direct Partial Boolean Derivative
     */
    template<class FChange>
    auto dpld (var_change varChange, FChange fChange, diagram_t const& diagram)
        -> diagram_t;

    /**
     * \brief Transforms \p dpld into Extended DPLD
     * \param varFrom Value from whitch the variable changes
     * \param varIndex Index of the variable
     * \param varFrom Derivative
     * \return Diagram representing Extended DPLD
     */
    auto to_dpld_e (int32 varFrom, int32 varIndex, diagram_t const& dpld)
        -> diagram_t;

    /**
     *  \brief Calculates Structural Importace (SI) of a component
     *
     *  Different types of DPLDs can be used for the calculation.
     *  It is up to the user to pick the one that suits his needs.
     *
     *  \param dpld Direct Partial Boolean Derivative of any type
     *  \return Structural importance of the component
     */
    auto structural_importance (diagram_t const& dpld) -> double;

    /**
     *  \brief Calculates Birnbaum importance (BI) of a component
     *
     *  Different types of DPLDs can be used for the calculation.
     *  It is up to the user to pick the one that suits his needs.
     *
     *  \param probs Component state probabilities
     *  \param dpld Direct Partial Boolean Derivative of any type
     *  \return Birnbaum importance of the component
     */
    template<component_probabilities Ps>
    auto birnbaum_importance (Ps const& probs, diagram_t const& dpld) -> double;

    /**
     *  \brief Calculates Fussell-Vesely importance (FVI) of a component
     *
     *  Different types of DPLDs can be used for the calculation.
     *  It is up to the user to pick the one that suits his needs.
     *
     *  \param probs Component state probabilities
     *  \param dpld Direct Partial Boolean Derivative of any type
     *  \param unavailability System unavailability
     *  \param componentState State of the component
     *  \param componentIndex Component index
     *  \return Fussell-Vesely of the component
     */
    template<component_probabilities Ps>
    auto fussell_vesely_importance (
        Ps const& probs,
        diagram_t const& dpld,
        double unavailability,
        int32 componentState,
        int32 componentIndex
    ) -> double;

    /**
     *  \brief Finds all Minimal Cut Vector (MCVs) of the system with
     *  respect to the system state \p State
     *
     *  This function uses \c satisfy_all function. Please keep in mind
     *  that for bigger systems, there can be a huge number of MCVs.
     *
     *  \tparam Vars Container type that defines \c operator[] and allows
     *  assigning integers e.g std::vector or std::array
     *  \param diagram Structure function
     *  \param state System state
     *  \returns Vector of Minimal Cut Vectors
     */
    template<out_var_values Vars>
    auto mcvs (diagram_t const& diagram, int32 state) -> std::vector<Vars>;

    /**
     *  \brief Finds all Minimal Path Vector (MPVs) of the system with
     *  respect to the system state \p state
     *
     *  This function uses \c satisfy_all function. Please keep in mind
     *  that for bigger systems, there can be a huge number of MCVs
     *
     *  \tparam Vars Container type that defines \c operator[] and allows
     *  assigning integers e.g std::vector or std::array
     *  \param diagram Structure function
     *  \param state System state
     *  \returns Vector of Minimal Cut Vectors.
     */
    template<out_var_values Vars>
    auto mpvs (diagram_t const& diagram, int32 state) -> std::vector<Vars>;

    /**
     *  \brief Finds all Minimal Cut Vector of the system with respect
     *  to the system state \p state
     *
     *  This function uses \c satisfy_all_g function. Please keep in mind
     *  that for bigger systems, there can be a huge number of MCVs.
     *
     *  \tparam Vars Container type that defines \c operator[] and allows
     *  assigning integers e.g std::vector or std::array
     *  \param diagram Structure function
     *  \param state System state
     *  \param out Output iterator that is used to output instances
     *  of \p Vars
     */
    template<out_var_values Vars, std::output_iterator<Vars> Out>
    auto mcvs_g (diagram_t const& diagram, int32 state, Out out) -> void;

    /**
     *  \brief Finds all Minimal Path Vector (MPVs) of the system with
     *  respect to the system state \p state
     *
     *  This function uses \c satisfy_all function. Please keep in mind
     *  that for bigger systems, there can be a huge number of MCVs
     *
     *  \tparam Vars Container type that defines \c operator[] and allows
     *  assigning integers e.g std::vector or std::array
     *  \param diagram Structure function
     *  \param state System state
     *  \param out Output iterator that is used to output instances
     *  of \p Vars .
     */
    template<out_var_values Vars, std::output_iterator<Vars> Out>
    auto mpvs_g (diagram_t const& diagram, int32 state, Out out) -> void;

protected:
    reliability_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        std::vector<int32> order
    )
    requires(domains::is_fixed<Domain>::value);

    reliability_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        domains::mixed domain,
        std::vector<int32> order
    )
    requires(domains::is_mixed<Domain>::value);

private:
    using node_t = typename diagram_manager<double, Degree, Domain>::node_t;

    template<class F>
    auto apply_dpld_new (
        diagram_t const& diagram,
        var_change varChange,
        F fChange
    ) -> diagram_t;

    template<component_probabilities Ps>
    auto calculate_ntp (
        std::vector<int32> const& selected,
        Ps const& probs,
        diagram_t const& diagram
    ) -> double;

    auto to_mnf (diagram_t const& diagram) -> diagram_t;
};

template<class Degree, class Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::calculate_probabilities(
    Ps const& probs,
    diagram_t const& diagram
) -> void
{
    auto const root = diagram.unsafe_get_root();

    this->nodes_.traverse_pre(
        root,
        [] (auto const n)
        {
            n->get_data() = 0.0;
        }
    );
    this->nodes_.for_each_terminal_node(
        [] (auto const n)
        {
            n->get_data() = 0.0;
        }
    );
    root->get_data() = 1.0;

    this->nodes_.traverse_level(
        root,
        [this, &probs] (auto const node)
        {
            if (node->is_internal())
            {
                auto const nodeIndex = node->get_index();
                auto sonOrder        = 0;
                this->nodes_.for_each_son(
                    node,
                    [node, nodeIndex, &probs, &sonOrder] (auto const son)
                    {
                        son->get_data()
                            += node->get_data()
                             * probs[as_uindex(nodeIndex)][as_uindex(sonOrder)];
                        ++sonOrder;
                    }
                );
            }
        }
    );
}

template<class Degree, class Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::calculate_probability(
    int32 const state,
    Ps const& probs,
    diagram_t const& diagram
) -> double
{
    return this->calculate_ntp({state}, probs, diagram);
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::get_probability(int32 const state
) const -> double
{
    auto const terminalNode = this->nodes_.get_terminal_node(state);
    return terminalNode ? terminalNode->get_data() : 0.0;
}

template<class Degree, class Domain>
template<component_probabilities Ps, class Foo>
requires(is_bss<Degree>)
auto reliability_manager<Degree, Domain>::calculate_availability(
    Ps const& probs,
    diagram_t const& diagram
) -> utils::second_t<Foo, double>
{
    return this->calculate_availability(1, probs, diagram);
}

template<class Degree, class Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::calculate_availability(
    int32 const state,
    Ps const& probs,
    diagram_t const& diagram
) -> double
{
    auto states = std::vector<int32>();
    this->nodes_.for_each_terminal_node(
        [state, &states] (node_t* const n)
        {
            if (n->get_value() >= state)
            {
                states.emplace_back(n->get_value());
            }
        }
    );
    return this->calculate_ntp(states, probs, diagram);
}

template<class Degree, class Domain>
template<class Foo>
auto reliability_manager<Degree, Domain>::get_availability() const
    -> utils::second_t<Foo, double>
{
    auto const terminalNode = this->nodes_.get_terminal_node(1);
    return terminalNode ? terminalNode->get_data() : 0;
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::get_availability(int32 const state
) const -> double
{
    auto result = .0;
    this->nodes_.for_each_terminal_node(
        [state, &result] (auto const node)
        {
            if (node->get_value() >= state)
            {
                result += node->get_data();
            }
        }
    );
    return result;
}

template<class Degree, class Domain>
template<component_probabilities Ps, class Foo>
requires(is_bss<Degree>)
auto reliability_manager<Degree, Domain>::calculate_unavailability(
    Ps const& probs,
    diagram_t const& diagram
) -> utils::second_t<Foo, double>
{
    return this->calculate_unavailability(1, probs, diagram);
}

template<class Degree, class Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::calculate_unavailability(
    int32 const state,
    Ps const& probs,
    diagram_t const& diagram
) -> double
{
    auto states = std::vector<int32>();
    this->nodes_.for_each_terminal_node(
        [state, &states] (auto const n)
        {
            if (n->get_value() < state)
            {
                states.emplace_back(n->get_value());
            }
        }
    );
    return this->calculate_ntp(states, probs, diagram);
}

template<class Degree, class Domain>
template<class Foo>
auto reliability_manager<Degree, Domain>::get_unavailability()
    -> utils::second_t<Foo, double>
{
    return this->get_unavailability(1);
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::get_unavailability(int32 const state)
    -> double
{
    double result = .0;
    this->nodes_.for_each_terminal_node(
        [state, &result] (node_t* const node)
        {
            if (node->get_value() < state)
            {
                result += node->get_data();
            }
        }
    );
    return result;
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::state_frequency(
    diagram_t const& diagram,
    int32 state
) -> double
{
    auto const indexFrom  = int32(0);
    auto const indexTo    = this->get_var_count();
    auto const domainSize = this->nodes_.domain_product(indexFrom, indexTo);
    return static_cast<double>(this->satisfy_count(state, diagram))
         / static_cast<double>(domainSize);
}

template<class Degree, class Domain>
template<class FChange>
auto reliability_manager<Degree, Domain>::dpld(
    var_change varChange,
    FChange fChange,
    diagram_t const& diagram
) -> diagram_t
{
    return this->apply_dpld_new(diagram, varChange, fChange);
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::to_dpld_e(
    int32 const varFrom,
    int32 const varIndex,
    diagram_t const& dpld
) -> diagram_t
{
    auto const root      = dpld.unsafe_get_root();
    auto const rootLevel = this->nodes_.get_level(root);
    auto const varLevel  = this->nodes_.get_level(varIndex);

    if (varLevel < rootLevel)
    {
        auto sons = this->nodes_.make_sons(
            varIndex,
            [this, varFrom, root] (int32 const sonOrder)
            {
                return sonOrder == varFrom
                         ? root
                         : this->nodes_.make_terminal_node(Undefined);
            }
        );
        auto const newRoot
            = this->nodes_.make_internal_node(varIndex, std::move(sons));
        return diagram_t(newRoot);
    }

    auto memo       = std::unordered_map<node_t*, node_t*>();
    auto const step = [=, this, &memo] (auto const& self, node_t* const n)
    {
        if (n->is_terminal())
        {
            return n;
        }

        auto const memoIt = memo.find(n);
        if (memoIt != end(memo))
        {
            return memoIt->second;
        }

        auto const nodeLevel = this->nodes_.get_level(n);
        auto const nodeIndex = n->get_index();
        auto sons            = this->nodes_.make_sons(
            nodeIndex,
            [=, this, &self] (int32 const sonOrder)
            {
                auto const son      = n->get_son(sonOrder);
                auto const sonLevel = this->nodes_.get_level(son);
                if (varLevel > nodeLevel && varLevel < sonLevel)
                {
                    // A new node goes in between the current node
                    // and its k th son.
                    // Transformation does not need to continue.
                    auto newNodeSons = this->nodes_.make_sons(
                        varIndex,
                        [this, varFrom, son] (int32 const newSonOrder)
                        {
                            return newSonOrder == varFrom
                                                ? son
                                                : this->nodes_.make_terminal_node(Undefined
                                     );
                        }
                    );
                    return this->nodes_.make_internal_node(
                        varIndex,
                        std::move(newNodeSons)
                    );
                }

                // A new node will be inserted somewhere deeper.
                return self(self, son);
            }
        );
        auto const transformedNode
            = this->nodes_.make_internal_node(nodeIndex, std::move(sons));
        memo.emplace(n, transformedNode);
        return transformedNode;
    };
    return diagram_t(step(step, root));
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::structural_importance(
    diagram_t const& dpld
) -> double
{
    auto const indexFrom  = int32(0);
    auto const indexTo    = static_cast<int32>(this->get_var_count());
    auto const domainSize = this->nodes_.domain_product(indexFrom, indexTo);
    return static_cast<double>(this->satisfy_count(1, dpld))
         / static_cast<double>(domainSize);
}

template<class Degree, class Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::birnbaum_importance(
    Ps const& probs,
    diagram_t const& dpld
) -> double
{
    return this->calculate_probability(1, probs, dpld);
}

template<class Degree, class Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::fussell_vesely_importance(
    Ps const& probs,
    diagram_t const& dpld,
    double const unavailability,
    int32 const componentState,
    int32 const componentIndex
) -> double
{
    auto const mnf            = this->to_mnf(dpld);
    auto const mnfProbability = this->calculate_probability(1, probs, mnf);
    auto nominator            = .0;
    for (auto lowerState = 0; lowerState < componentState; ++lowerState)
    {
        nominator += probs[as_uindex(componentIndex)][as_uindex(lowerState)];
    }
    nominator *= mnfProbability;
    return nominator / unavailability;
}

template<class Degree, class Domain>
template<out_var_values Vars>
auto reliability_manager<Degree, Domain>::mcvs(
    diagram_t const& diagram,
    int32 const state
) -> std::vector<Vars>
{
    auto cuts = std::vector<Vars>();
    this->mcvs_g<Vars>(diagram, state, std::back_inserter(cuts));
    return cuts;
}

template<class Degree, class Domain>
template<out_var_values Vars>
auto reliability_manager<Degree, Domain>::mpvs(
    diagram_t const& diagram,
    int32 state
) -> std::vector<Vars>
{
    auto cuts = std::vector<Vars>();
    this->mcvs_g<Vars>(diagram, state, std::back_inserter(cuts));
    return cuts;
}

template<class Degree, class Domain>
template<out_var_values Vars, std::output_iterator<Vars> Out>
auto reliability_manager<Degree, Domain>::mcvs_g(
    diagram_t const& diagram,
    int32 const state,
    Out out
) -> void
{
    auto const varCount = this->get_var_count();
    auto dpldes         = std::vector<diagram_t>();

    for (auto varIndex = 0; varIndex < varCount; ++varIndex)
    {
        auto const varDomain = this->nodes_.get_domain(varIndex);
        for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom)
        {
            auto const varChange = var_change {varIndex, varFrom, varFrom + 1};
            auto const dpld
                = this->dpld(varChange, dpld::type_3_increase(state), diagram);
            dpldes.emplace_back(this->to_dpld_e(varFrom, varIndex, dpld));
        }
    }

    auto const conj = this->template tree_fold<ops::PI_CONJ>(dpldes);
    this->template satisfy_all_g<Vars, Out>(1, conj, out);
}

template<class Degree, class Domain>
template<out_var_values Vars, std::output_iterator<Vars> Out>
auto reliability_manager<Degree, Domain>::mpvs_g(
    diagram_t const& diagram,
    int32 const state,
    Out out
) -> void
{
    auto const varCount = this->get_var_count();
    auto dpldes         = std::vector<diagram_t>();

    for (auto varIndex = 0; varIndex < varCount; ++varIndex)
    {
        auto const varDomain = this->nodes_.get_domain(varIndex);
        for (auto varFrom = 1; varFrom < varDomain; ++varFrom)
        {
            auto const varChange = var_change {varIndex, varFrom, varFrom - 1};
            auto const dpld
                = this->dpld(varChange, dpld::type_3_decrease(state), diagram);
            dpldes.emplace_back(this->to_dpld_e(varFrom, varIndex, dpld));
        }
    }

    auto const conj = this->template tree_fold<ops::PI_CONJ>(dpldes);
    this->template satisfy_all_g<Vars, Out>(1, conj, out);
}

template<class Degree, class Domain>
template<class F>
auto reliability_manager<Degree, Domain>::apply_dpld_new(
    diagram_t const& diagram,
    var_change varChange,
    F fChange
) -> diagram_t
{
    auto cache = std::unordered_map<
        std::tuple<node_t*, node_t*>,
        node_t*,
        utils::tuple_hash
    >();

    auto const get_node_value = [] (node_t* const node)
    {
        return node->is_terminal() ? node->get_value() : Nondetermined;
    };

    auto const get_lhs_son = [varChange](node_t* const node, int32 const sonOrder)
    {
        auto const son = node->get_son(sonOrder);
        return son->is_internal() && son->get_index() == varChange.index_
            ? son->get_son(varChange.from_)
            : son;
    };

    auto const get_rhs_son = [varChange](node_t* const node, int32 const sonOrder)
    {
        auto const son = node->get_son(sonOrder);
        return son->is_internal() && son->get_index() == varChange.index_
            ? son->get_son(varChange.to_)
            : son;
    };

    auto const step = [
        this,
        &cache,
        fChange,
        get_node_value,
        get_lhs_son,
        get_rhs_son
    ] (
        auto const& self,
        node_t* const lhs,
        node_t* const rhs
    ) -> node_t*
    {
        auto const cached = cache.find(std::make_tuple(lhs, rhs));
        if (cached != end(cache))
        {
            return cached->second;
        }

        auto const lhsVal = get_node_value(lhs);
        auto const rhsVal = get_node_value(rhs);
        auto const opVal = lhsVal == Nondetermined || rhsVal == Nondetermined
            ? Nondetermined
            : static_cast<int32>(fChange(lhsVal, rhsVal));
        auto result = static_cast<node_t*>(nullptr);

        if (opVal != Nondetermined)
        {
            result = this->nodes_.make_terminal_node(opVal);
        }
        else
        {
            auto const lhsLevel = this->nodes_.get_level(lhs);
            auto const rhsLevel = this->nodes_.get_level(rhs);
            auto const topLevel = std::min(lhsLevel, rhsLevel);
            auto const topNode  = topLevel == lhsLevel ? lhs : rhs;
            auto const topIndex = topNode->get_index();
            auto sons           = this->nodes_.make_sons(
                topIndex,
                [=, &self] (int32 const sonOrder)
                {
                    auto const fst = lhsLevel == topLevel
                        ? get_lhs_son(lhs, sonOrder)
                        : lhs;

                    auto const snd = rhsLevel == topLevel
                        ? get_rhs_son(rhs, sonOrder)
                        : rhs;

                    return self(self, fst, snd);
                }
            );

            result = this->nodes_.make_internal_node(topIndex, std::move(sons));
        }

        cache.emplace(
            std::piecewise_construct,
            std::make_tuple(lhs, rhs),
            std::make_tuple(result)
        );
        return result;
    };

    auto const oldRoot = diagram.unsafe_get_root();
    auto const lhsRoot = oldRoot->get_index() == varChange.index_
        ? oldRoot->get_son(varChange.from_)
        : oldRoot;
    auto const rhsRoot = oldRoot->get_index() == varChange.index_
        ? oldRoot->get_son(varChange.to_)
        : oldRoot;
    auto const newRoot = step(step, lhsRoot, rhsRoot);
    return diagram_t(newRoot);
}

template<class Degree, class Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::calculate_ntp(
    std::vector<int32> const& selectedValues,
    Ps const& probs,
    diagram_t const& diagram
) -> double
{
    this->nodes_.for_each_terminal_node(
        [] (node_t* const n)
        {
            n->get_data() = 0.0;
        }
    );

    for (auto const selectedValue : selectedValues)
    {
        auto const terminalNode = this->nodes_.get_terminal_node(selectedValue);
        if (terminalNode)
        {
            terminalNode->get_data() = 1.0;
        }
    }

    this->nodes_.traverse_post(
        diagram.unsafe_get_root(),
        [this, &probs] (node_t* const node) mutable
        {
            if (not node->is_terminal())
            {
                node->get_data()     = 0.0;
                auto const nodeIndex = node->get_index();
                this->nodes_.for_each_son(
                    node,
                    [=, sonState = 0, &probs] (node_t* const son) mutable
                    {
                        node->get_data()
                            += son->get_data()
                             * probs[as_uindex(nodeIndex)][as_uindex(sonState)];
                        ++sonState;
                    }
                );
            }
        }
    );
    return diagram.unsafe_get_root()->get_data();
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::to_mnf(diagram_t const& diagram)
    -> diagram_t
{
    auto const step = [this] (auto& self, node_t* const node)
    {
        if (node->is_terminal())
        {
            return node;
        }

        auto const index  = node->get_index();
        auto const domain = this->nodes_.get_domain(index);
        auto newSons      = this->nodes_.make_sons(
            index,
            [&self, node] (int32 const sonOrder)
            {
                return self(self, node->get_son(sonOrder));
            }
        );

        for (auto sonOrder = domain - 1; sonOrder > 0; --sonOrder)
        {
            auto const son = newSons[as_uindex(sonOrder)];
            if (son->is_terminal() && son->get_value() == 1)
            {
                for (auto k = 0; k < sonOrder; ++k)
                {
                    newSons[as_uindex(k)] = son;
                }
                break;
            }
        }

        for (auto sonOrder = domain - 2; sonOrder >= 0; --sonOrder)
        {
            auto const son = newSons[as_uindex(sonOrder)];
            if (son->is_terminal() && son->get_value() == 0)
            {
                newSons[as_uindex(sonOrder)] = newSons[as_uindex(sonOrder + 1)];
            }
        }

        return this->nodes_.make_internal_node(index, std::move(newSons));
    };

    return diagram_t(step(step, diagram.unsafe_get_root()));
}

template<class Degree, class Domain>
reliability_manager<Degree, Domain>::reliability_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order
)
requires(domains::is_fixed<Domain>::value)
    :
    diagram_manager<double, Degree, Domain>(
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        std::move(order)
    )
{
}

template<class Degree, class Domain>
reliability_manager<Degree, Domain>::reliability_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    domains::mixed domain,
    std::vector<int32> order
)
requires(domains::is_mixed<Domain>::value)
    :
    diagram_manager<double, Degree, Domain>(
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        std::move(domain),
        std::move(order)
    )
{
}
} // namespace teddy

#endif
