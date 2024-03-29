#ifndef LIBTEDDY_DETAILS_RELIABILITY_MANAGER_HPP
#define LIBTEDDY_DETAILS_RELIABILITY_MANAGER_HPP

#include <libteddy/details/diagram_manager.hpp>
#include <libteddy/details/dplds.hpp>
#include <libteddy/details/probabilities.hpp>

#include <concepts>
#include <iterator>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace teddy
{
namespace details
{
template<class Degree>
concept is_bss = std::same_as<degrees::fixed<2>, Degree>;
}

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
    // TODO                          ^^^^^^
    //                          details::bytes<max(sizeof(double),
    //                          sizeof(expr))>

public:
    /**
     *  \brief Calculates probabilities of system states 0 and 1
     *
     *  \p probs[i] must return probability that i-th component is in state 1
     *  After a call to this method you can acces individual system
     *  state probabilities using \c get_probability method.
     *
     *  \tparam Type that holds component state probabilities
     *  \param probs vector of component state probabilities
     *  \param diagram Structure function
     */
    template<probs::prob_vector Ps>
    requires(details::is_bss<Degree>)
    auto calculate_probabilities (Ps const& probs, diagram_t const& diagram)
        -> void;

    /**
     *  \brief Calculates probabilities of all system states
     *
     *  \p probs[i][k] must return probability that i-th component is in state k
     *  After a call to this method you can acces individual system
     *  state probabilities using \c get_probability method.
     *
     *  \tparam Type that holds component state probabilities
     *  \param probs matrix of component state probabilities
     *  \param diagram Structure function
     */
    template<probs::prob_matrix Ps>
    auto calculate_probabilities (Ps const& probs, diagram_t const& diagram)
        -> void;

    /**
     *  \brief Calculates and returns probability of system state 1
     *
     *  \p probs[i] must return probability that i-th component is in state 1
     *
     *  \tparam Type that holds component state probabilities
     *  \param probs vector of component state probabilities
     *  \param diagram Structure function
     *  \return Probability that system described by \p diagram is in
     *  state 1 given probabilities \p probs
     */
    template<probs::prob_vector Ps>
    requires(details::is_bss<Degree>)
    auto calculate_probability (Ps const& probs, diagram_t const& diagram)
        -> double;

    /**
     *  \brief Calculates and returns probability of a system state \p state
     *
     *  \p probs[i][k] must return probability that i-th component is in state k
     *
     *  \tparam Type that holds component state probabilities
     *  \param state System state
     *  \param probs matrix of component state probabilities
     *  \param diagram Structure function
     *  \return Probability that system described by \p diagran is in
     *  state \p state given probabilities \p probs
     */
    template<probs::prob_matrix Ps>
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
     *  \c get_probability
     *
     *  \param state System state
     *  \return Probability of a system state \p state
     */
    [[nodiscard]] auto get_probability (int32 state) const -> double;

    /**
     *  \brief Calculates and returns availability of a BSS.
     *
     *  \p probs[i] must return probability that i-th component is in state 1
     *
     *  \tparam Component state probabilities
     *  \tparam Foo Dummy parameter to enable SFINE
     *  \param probs vector of component state probabilities
     *  \param diagram Structure function
     *  \return System availability
     */
    template<probs::prob_vector Ps, class Foo = void>
    requires(details::is_bss<Degree>)
    auto calculate_availability (Ps const& probs, diagram_t const& diagram)
        -> utils::second_t<Foo, double>;

    /**
     *  \brief Calculates and returns system availability with
     *  respect to the system state \p state
     *
     *  \p probs[i][k] must return probability that i-th component is in state k
     *
     *  \tparam Component state probabilities
     *  \param state System state
     *  \param probs matrix of component state probabilities
     *  \param diagram Structure function
     *  \return System availability with respect to the system state \p state
     */
    template<probs::prob_matrix Ps>
    auto calculate_availability (
        int32 state,
        Ps const& probs,
        diagram_t const& diagram
    ) -> double;

    /**
     *  \brief Returns availability of a BSS
     *
     *  Call to \c calculate_probabilities must proceed call to this
     *  funtion otherwise the result is undefined. This is a bit
     *  unfortunate but the idea is that probabilities are calculated
     *  once using \c calculate_probabilities and availability and
     *  unavailability are later accessed using \c get_availability
     *  and \c get_unavailability
     *
     *  \return System availability
     */
    template<class Foo = void>
    requires(details::is_bss<Degree>)
    [[nodiscard]] auto get_availability () const
        -> utils::second_t<Foo, double>;

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
     *  \brief Calculates and returns unavailability of a BSS
     *
     *  \p probs[i] must return probability that i-th component is in state 1
     *
     *  \tparam Component state probabilities
     *  \tparam Foo Dummy parameter to enable SFINE
     *  \param probs vector of component state probabilities
     *  \param diagran Structure function
     *  \return System unavailtability
     */
    template<probs::prob_matrix Ps, class Foo = void>
    requires(details::is_bss<Degree>)
    auto calculate_unavailability (Ps const& probs, diagram_t const& diagram)
        -> utils::second_t<Foo, double>;

    /**
     *  \brief Calculates and returns system availability with
     *  respect to the system state \p state
     *
     *  \p probs[i][k] must return probability that i-th component is in state k
     *
     *  \tparam Component state probabilities
     *  \param state System state
     *  \param probs matrix of component state probabilities
     *  \param diagram Structure function
     *  \return System availability with respect to the system state \p state
     */
    template<probs::prob_matrix Ps>
    auto calculate_unavailability (
        int32 state,
        Ps const& probs,
        diagram_t const& diagram
    ) -> double;

    /**
     *  \brief Returns system unavailability of a BSS
     *
     *  Call to \c calculate_probabilities must proceed call to this
     *  funtion otherwise the result is undefined. This is a bit
     *  unfortunate but the idea is that probabilities are calculated
     *  once using \c calculate_probabilities and availability and
     *  unavailability are later accessed using \c get_availability
     *  and \c get_unavailability
     *
     *  \return System availability
     */
    template<class Foo = void>
    requires(details::is_bss<Degree>)
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

#ifdef LIBTEDDY_SYMBOLIC_RELIABILITY
    // TODO cmp post a level verzie

    /**
     *  \brief TODO
     */
    template<class Ps>
    auto symbolic_probability (
        int32 state,
        Ps const& probs,
        diagram_t const& diagram
    ) -> symprobs::expression;

    /**
     *  \brief TODO
     */
    template<class Ps>
    auto symbolic_availability (
        int32 state,
        Ps const& probs,
        diagram_t const& diagram
    ) -> symprobs::expression;

    /**
     *  \brief TODO
     */
    template<class Ps>
    auto unsymbolic_availability (
        int32 state,
        Ps const& probs,
        diagram_t const& diagram
    ) -> symprobs::expression;
#endif
    // TODO calculate_state_frequency
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
    template<probs::prob_matrix Ps>
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
    template<probs::prob_matrix Ps>
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
    using son_conainer = typename node_t::son_container;

    // TODO not nice
    // same problem as n-ary apply, we will see...

    struct dpld_cache_entry
    {
        node_t* lhs_;
        node_t* rhs_;
    };

    struct cache_entry_hash
    {
        auto operator() (dpld_cache_entry const& entry) const
        {
            return utils::pack_hash(entry.lhs_, entry.rhs_);
        }
    };

    struct cache_entry_equals
    {
        auto operator() (
            dpld_cache_entry const& lhs,
            dpld_cache_entry const& rhs
        ) const
        {
            return lhs.lhs_ == rhs.lhs_ && lhs.rhs_ == rhs.rhs_;
        }
    };

    using dpld_cache = std::unordered_map<
        dpld_cache_entry,
        node_t*,
        cache_entry_hash,
        cache_entry_equals>;

private:
    auto to_mnf (diagram_t const& diagram) -> diagram_t;

    auto to_mnf_impl (std::unordered_map<node_t*, node_t*>& memo, node_t* node)
        -> node_t*;

    template<class FChange>
    auto dpld_impl (
        dpld_cache& cache,
        var_change varChange,
        FChange fChange,
        node_t* lhs,
        node_t* rhs
    ) -> node_t*;

    auto to_dpld_e_impl (
        std::unordered_map<node_t*, node_t*>& memo,
        int32 varFrom,
        int32 varIndex,
        node_t* node
    ) -> node_t*;

    template<probs::prob_matrix Ps>
    auto calculate_ntps_post_impl (
        std::vector<int32> const& selected,
        Ps const& probs,
        node_t* root
    ) -> double;

    template<probs::prob_matrix Ps>
    auto calculate_ntps_level_impl (Ps const& probs, node_t* root) -> void;
};

template<class Degree, class Domain>
template<probs::prob_vector Ps>
requires(details::is_bss<Degree>)
auto reliability_manager<Degree, Domain>::calculate_probabilities(
    Ps const& probs,
    diagram_t const& diagram
) -> void
{
    this->calculate_probabilities(
        probs::details::prob_vector_wrap(probs),
        diagram
    );
}

template<class Degree, class Domain>
template<probs::prob_matrix Ps>
auto reliability_manager<Degree, Domain>::calculate_probabilities(
    Ps const& probs,
    diagram_t const& diagram
) -> void
{
    this->calculate_ntps_level_impl(probs, diagram.unsafe_get_root());
}

template<class Degree, class Domain>
template<probs::prob_vector Ps>
requires(details::is_bss<Degree>)
auto reliability_manager<Degree, Domain>::calculate_probability(
    Ps const& probs,
    diagram_t const& diagram
) -> double
{
    return this->calculate_probability(
        1,
        probs::details::prob_vector_wrap(probs),
        diagram
    );
}

template<class Degree, class Domain>
template<probs::prob_matrix Ps>
auto reliability_manager<Degree, Domain>::calculate_probability(
    int32 const state,
    Ps const& probs,
    diagram_t const& diagram
) -> double
{
    return this
        ->calculate_ntps_post_impl({state}, probs, diagram.unsafe_get_root());
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::get_probability(int32 const state
) const -> double
{
    node_t* const node = this->nodes_.get_terminal_node(state);
    return node ? node->get_data() : 0.0;
}

template<class Degree, class Domain>
template<probs::prob_vector Ps, class Foo>
requires(details::is_bss<Degree>)
auto reliability_manager<Degree, Domain>::calculate_availability(
    Ps const& probs,
    diagram_t const& diagram
) -> utils::second_t<Foo, double>
{
    return this->calculate_availability(
        1,
        probs::details::prob_vector_wrap(probs),
        diagram
    );
}

template<class Degree, class Domain>
template<probs::prob_matrix Ps>
auto reliability_manager<Degree, Domain>::calculate_availability(
    int32 const state,
    Ps const& probs,
    diagram_t const& diagram
) -> double
{
    std::vector<int32> states;
    this->nodes_.for_each_terminal_node(
        [state, &states] (node_t* const node)
        {
            if (node->get_value() >= state)
            {
                states.push_back(node->get_value());
            }
        }
    );
    return this
        ->calculate_ntps_post_impl(states, probs, diagram.unsafe_get_root());
}

template<class Degree, class Domain>
template<class Foo>
requires(details::is_bss<Degree>)
auto reliability_manager<Degree, Domain>::get_availability() const
    -> utils::second_t<Foo, double>
{
    node_t* const node = this->nodes_.get_terminal_node(1);
    return node ? node->get_data() : 0;
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::get_availability(int32 const state
) const -> double
{
    double result = 0;
    this->nodes_.for_each_terminal_node(
        [state, &result] (node_t* const node)
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
template<probs::prob_matrix Ps, class Foo>
requires(details::is_bss<Degree>)
auto reliability_manager<Degree, Domain>::calculate_unavailability(
    Ps const& probs,
    diagram_t const& diagram
) -> utils::second_t<Foo, double>
{
    return this->calculate_unavailability(1, probs, diagram);
}

template<class Degree, class Domain>
template<probs::prob_matrix Ps>
auto reliability_manager<Degree, Domain>::calculate_unavailability(
    int32 const state,
    Ps const& probs,
    diagram_t const& diagram
) -> double
{
    std::vector<int32> states;
    this->nodes_.for_each_terminal_node(
        [state, &states] (node_t* const node)
        {
            if (node->get_value() < state)
            {
                states.emplace_back(node->get_value());
            }
        }
    );
    return this
        ->calculate_ntps_post_impl(states, probs, diagram.unsafe_get_root());
}

template<class Degree, class Domain>
template<class Foo>
requires(details::is_bss<Degree>)
auto reliability_manager<Degree, Domain>::get_unavailability()
    -> utils::second_t<Foo, double>
{
    return this->get_unavailability(1);
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::get_unavailability(int32 const state)
    -> double
{
    double result = 0;
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

#ifdef LIBTEDDY_SYMBOLIC_RELIABILITY

/**
 *  \brief TODO
 */
template<class Degree, class Domain>
template<class Ps>
auto reliability_manager<Degree, Domain>::symbolic_availability(
    int32 state,
    Ps const& probs,
    diagram_t const& diagram
) -> symprobs::expression
{
    // TODO store it in the nodes
    std::unordered_map<node_t*, GiNaC::ex> exprMap;

    this->nodes_.for_each_terminal_node(
        [&exprMap, state] (node_t* const node)
        {
            GiNaC::ex val(static_cast<double>(
                static_cast<int32>(node->get_value() >= state)
            ));
            exprMap.emplace(std::make_pair(node, val));
        }
    );

    node_t* const root = diagram.unsafe_get_root();
    this->nodes_.traverse_post(
        root,
        [this, &probs, &exprMap] (node_t* const node) mutable
        {
            if (not node->is_terminal())
            {
                auto [it, isIn]
                    = exprMap.emplace(std::make_pair(node, GiNaC::ex(0.0)));
                assert(isIn);
                GiNaC::ex& expr       = it->second;
                int32 const nodeIndex = node->get_index();
                int32 const domain    = this->nodes_.get_domain(nodeIndex);
                for (int32 k = 0; k < domain; ++k)
                {
                    node_t* const son = node->get_son(k);
                    expr += exprMap.find(son)->second
                          * probs[as_uindex(nodeIndex)][as_uindex(k)]
                                .as_underlying_unsafe();
                }
            }
        }
    );
    return symprobs::expression(exprMap.find(root)->second);
}

#endif

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::state_frequency(
    diagram_t const& diagram,
    int32 state
) -> double
{
    int32 const indexFrom  = 0;
    int32 const indexTo    = this->get_var_count();
    int64 const domainSize = this->nodes_.domain_product(indexFrom, indexTo);
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
    // TODO vector cache?
    dpld_cache cache;
    node_t* const oldRoot = diagram.unsafe_get_root();
    node_t* lhsRoot       = oldRoot;
    node_t* rhsRoot       = oldRoot;
    if (not oldRoot->is_terminal() && oldRoot->get_index() == varChange.index_)
    {
        lhsRoot = oldRoot->get_son(varChange.from_);
        rhsRoot = oldRoot->get_son(varChange.to_);
    }
    node_t* const newRoot
        = this->dpld_impl(cache, varChange, fChange, lhsRoot, rhsRoot);
    this->nodes_.run_deferred();
    return diagram_t(newRoot);
}

template<class Degree, class Domain>
template<class FChange>
auto reliability_manager<Degree, Domain>::dpld_impl(
    dpld_cache& cache,
    var_change varChange,
    FChange fChange,
    node_t* const lhs,
    node_t* const rhs
) -> node_t*
{
    constexpr auto get_son = [] (node_t* const node,
                                 int32 const k,
                                 int32 const varIndex,
                                 int32 const varValue)
    {
        auto const son = node->get_son(k);
        return son->is_internal() && son->get_index() == varIndex
                 ? son->get_son(varValue)
                 : son;
    };

    auto const cached = cache.find(dpld_cache_entry(lhs, rhs));
    if (cached != cache.end())
    {
        return cached->second;
    }

    node_t* result = nullptr;

    if (lhs->is_terminal() && rhs->is_terminal())
    {
        result = this->nodes_.make_terminal_node(
            static_cast<int32>(fChange(lhs->get_value(), rhs->get_value()))
        );
    }
    else
    {
        int32 const lhsLevel = this->nodes_.get_level(lhs);
        int32 const rhsLevel = this->nodes_.get_level(rhs);
        int32 const topLevel = utils::min(lhsLevel, rhsLevel);
        int32 const topIndex = this->nodes_.get_index(topLevel);
        int32 const domain   = this->nodes_.get_domain(topIndex);
        son_conainer sons    = this->nodes_.make_son_container(domain);
        for (int32 k = 0; k < domain; ++k)
        {
            node_t* const fst
                = lhsLevel == topLevel
                    ? get_son(lhs, k, varChange.index_, varChange.from_)
                    : lhs;

            node_t* const snd
                = rhsLevel == topLevel
                    ? get_son(rhs, k, varChange.index_, varChange.to_)
                    : rhs;
            sons[k] = this->dpld_impl(cache, varChange, fChange, fst, snd);
        }

        result = this->nodes_.make_internal_node(topIndex, sons);
    }

    cache.emplace(dpld_cache_entry(lhs, rhs), result);

    return result;
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::to_dpld_e(
    int32 const varFrom,
    int32 const varIndex,
    diagram_t const& dpld
) -> diagram_t
{
    node_t* const root    = dpld.unsafe_get_root();
    int32 const rootLevel = this->nodes_.get_level(root);
    int32 const varLevel  = this->nodes_.get_level(varIndex);
    node_t* newRoot       = nullptr;

    if (varLevel < rootLevel)
    {
        int32 const varDomain = this->nodes_.get_domain(varIndex);
        son_conainer sons     = this->nodes_.make_son_container(varDomain);
        for (int32 k = 0; k < varDomain; ++k)
        {
            sons[k] = k == varFrom ? root
                                   : this->nodes_.make_terminal_node(Undefined);
        }
        newRoot = this->nodes_.make_internal_node(varIndex, sons);
        return diagram_t(newRoot);
    }
    else
    {
        std::unordered_map<node_t*, node_t*> memo;
        newRoot = this->to_dpld_e_impl(memo, varFrom, varIndex, root);
    }

    // TODO run at other places too
    // TODO add perf to benchmark scripts
    this->nodes_.run_deferred();
    return diagram_t(newRoot);
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::to_dpld_e_impl(
    std::unordered_map<node_t*, node_t*>& memo,
    int32 const varFrom,
    int32 const varIndex,
    node_t* const node
) -> node_t*
{
    if (node->is_terminal())
    {
        return node;
    }

    auto const memoIt = memo.find(node);
    if (memoIt != memo.end())
    {
        return memoIt->second;
    }

    int32 const varDomain  = this->nodes_.get_domain(varIndex);
    int32 const varLevel   = this->nodes_.get_level(varIndex);
    int32 const nodeLevel  = this->nodes_.get_level(node);
    int32 const nodeIndex  = this->nodes_.get_index(nodeLevel);
    int32 const nodeDomain = this->nodes_.get_domain(nodeIndex);
    son_conainer sons      = this->nodes_.make_son_container(nodeDomain);
    for (int32 k = 0; k < nodeDomain; ++k)
    {
        node_t* const son    = node->get_son(k);
        int32 const sonLevel = this->nodes_.get_level(son);
        if (varLevel > nodeLevel && varLevel < sonLevel)
        {
            // A new node goes in between the current node and its k-th son.
            // Transformation does not need to continue.
            son_conainer newSons = this->nodes_.make_son_container(varDomain);
            for (int32 l = 0; l < varDomain; ++l)
            {
                newSons[l] = l == varFrom
                               ? son
                               : this->nodes_.make_terminal_node(Undefined);
            }
            sons[k] = this->nodes_.make_internal_node(varIndex, newSons);
        }
        else
        {
            // A new node will be inserted somewhere deeper.
            sons[k] = this->to_dpld_e_impl(memo, varFrom, varIndex, son);
        }
    }
    node_t* const newNode = this->nodes_.make_internal_node(nodeIndex, sons);
    memo.emplace(node, newNode);
    return newNode;
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::structural_importance(
    diagram_t const& dpld
) -> double
{
    int32 const indexFrom  = 0;
    int32 const indexTo    = this->get_var_count();
    int64 const domainSize = this->nodes_.domain_product(indexFrom, indexTo);
    return static_cast<double>(this->satisfy_count(1, dpld))
         / static_cast<double>(domainSize);
}

template<class Degree, class Domain>
template<probs::prob_matrix Ps>
auto reliability_manager<Degree, Domain>::birnbaum_importance(
    Ps const& probs,
    diagram_t const& dpld
) -> double
{
    return this->calculate_probability(1, probs, dpld);
}

template<class Degree, class Domain>
template<probs::prob_matrix Ps>
auto reliability_manager<Degree, Domain>::fussell_vesely_importance(
    Ps const& probs,
    diagram_t const& dpld,
    double const unavailability,
    int32 const componentState,
    int32 const componentIndex
) -> double
{
    diagram_t const mnf         = this->to_mnf(dpld);
    double const mnfProbability = this->calculate_probability(1, probs, mnf);
    double nominator            = 0;
    for (int32 lowerState = 0; lowerState < componentState; ++lowerState)
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
    std::vector<Vars> cuts;
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
    int32 const varCount = this->get_var_count();
    std::vector<diagram_t> dpldes;

    for (int32 varIndex = 0; varIndex < varCount; ++varIndex)
    {
        int32 const varDomain = this->nodes_.get_domain(varIndex);
        for (int32 varFrom = 0; varFrom < varDomain - 1; ++varFrom)
        {
            var_change varChange {varIndex, varFrom, varFrom + 1};
            diagram_t const dpld
                = this->dpld(varChange, dpld::type_3_increase(state), diagram);
            dpldes.push_back(this->to_dpld_e(varFrom, varIndex, dpld));
        }
    }

    diagram_t const conj = this->template tree_fold<ops::PI_CONJ>(dpldes);
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
    int32 const varCount = this->get_var_count();
    std::vector<diagram_t> dpldes;

    for (int32 varIndex = 0; varIndex < varCount; ++varIndex)
    {
        int32 const varDomain = this->nodes_.get_domain(varIndex);
        for (int32 varFrom = 1; varFrom < varDomain; ++varFrom)
        {
            var_change varChange {varIndex, varFrom, varFrom - 1};
            diagram_t const dpld
                = this->dpld(varChange, dpld::type_3_decrease(state), diagram);
            dpldes.push_back(this->to_dpld_e(varFrom, varIndex, dpld));
        }
    }

    diagram_t const conj = this->template tree_fold<ops::PI_CONJ>(dpldes);
    this->template satisfy_all_g<Vars, Out>(1, conj, out);
}

template<class Degree, class Domain>
template<probs::prob_matrix Ps>
auto reliability_manager<Degree, Domain>::calculate_ntps_post_impl(
    std::vector<int32> const& selectedValues,
    Ps const& probs,
    node_t* const root
) -> double
{
    this->nodes_.for_each_terminal_node([] (node_t* const node)
                                        { node->get_data() = 0.0; });

    for (int32 const selectedValue : selectedValues)
    {
        node_t* const node = this->nodes_.get_terminal_node(selectedValue);
        if (node)
        {
            node->get_data() = 1.0;
        }
    }

    this->nodes_.traverse_post(
        root,
        [this, &probs] (node_t* const node) mutable
        {
            if (not node->is_terminal())
            {
                node->get_data()      = 0.0;
                int32 const nodeIndex = node->get_index();
                int32 const domain    = this->nodes_.get_domain(nodeIndex);
                for (int32 k = 0; k < domain; ++k)
                {
                    node_t* const son = node->get_son(k);
                    node->get_data()
                        += son->get_data()
                         * probs[as_uindex(nodeIndex)][as_uindex(k)];
                }
            }
        }
    );
    return root->get_data();
}

template<class Degree, class Domain>
template<probs::prob_matrix Ps>
auto reliability_manager<Degree, Domain>::calculate_ntps_level_impl(
    Ps const& probs,
    node_t* const root
) -> void
{
    this->nodes_.traverse_pre(
        root,
        [] (node_t* const node) { node->get_data() = 0.0; }
    );
    this->nodes_.for_each_terminal_node([] (node_t* const node)
                                        { node->get_data() = 0.0; });
    root->get_data() = 1.0;

    this->nodes_.traverse_level(
        root,
        [this, &probs] (node_t* const node)
        {
            if (node->is_internal())
            {
                int32 const nodeIndex = node->get_index();
                int32 const domain    = this->nodes_.get_domain(nodeIndex);
                for (int32 k = 0; k < domain; ++k)
                {
                    node_t* const son = node->get_son(k);
                    son->get_data()
                        += node->get_data()
                         * probs[as_uindex(nodeIndex)][as_uindex(k)];
                }
            }
        }
    );
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::to_mnf(diagram_t const& diagram)
    -> diagram_t
{
    std::unordered_map<node_t*, node_t*> memo;
    node_t* const newRoot = this->to_mnf_impl(memo, diagram.unsafe_get_root());
    this->nodes_.run_deferred();
    return diagram_t(newRoot);
}

template<class Degree, class Domain>
auto reliability_manager<Degree, Domain>::to_mnf_impl(
    std::unordered_map<node_t*, node_t*>& memo,
    node_t* node
) -> node_t*
{
    if (node->is_terminal())
    {
        return node;
    }

    auto const memoIt = memo.find(node);
    if (memoIt != memo.end())
    {
        return memoIt->second;
    }

    int32 const nodeIndex = node->get_index();
    int32 const domain    = this->nodes_.get_domain(nodeIndex);
    son_conainer sons     = this->nodes_.make_son_container(domain);
    for (int32 k = 0; k < domain; ++k)
    {
        node_t* const son = node->get_son(k);
        sons[k]           = this->to_mnf_impl(memo, son);
    }

    for (int32 k = domain - 1; k > 0; --k)
    {
        node_t* const son = sons[k];
        if (son->is_terminal() && son->get_value() == 1)
        {
            for (int32 l = 0; l < k; ++l)
            {
                sons[l] = son;
            }
            break;
        }
    }

    for (int32 k = domain - 2; k >= 0; --k)
    {
        node_t* const son = sons[k];
        if (son->is_terminal() && son->get_value() == 0)
        {
            sons[k] = sons[k + 1];
        }
    }

    node_t* const newNode = this->nodes_.make_internal_node(nodeIndex, sons);
    memo.emplace(node, newNode);
    return newNode;
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
        static_cast<std::vector<int32>&&>(order)
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
        static_cast<domains::mixed&&>(domain),
        static_cast<std::vector<int32>&&>(order)
    )
{
}
} // namespace teddy

#endif
