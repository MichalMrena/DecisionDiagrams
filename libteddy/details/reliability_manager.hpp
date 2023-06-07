#ifndef LIBTEDDY_DETAILS_RELIABILITY_MANAGER_HPP
#define LIBTEDDY_DETAILS_RELIABILITY_MANAGER_HPP

#include <array>
#include <libteddy/details/diagram_manager.hpp>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace teddy
{
template<class Degree>
concept is_bss = std::same_as<degrees::fixed<2>, Degree>;

template<class Probabilities>
concept component_probabilities =
    requires(Probabilities ps, int32 index, int32 val) {
        {
            ps[index][val]
        } -> std::convertible_to<double>;
    };

template<class F>
concept f_val_change = requires(F f, int32 l, int32 r) {
                           {
                               f(l, r)
                           } -> std::convertible_to<bool>;
                       };

/**
 *  \struct value_change
 *  \brief Describes change of a value of a variable or a function.
 */
struct value_change
{
    int32 from;
    int32 to;
};

/**
 *  \class reliability_manager
 *  \brief Base class for reliability managers.
 *
 *  Base class for reliability managers. Defines all functions for
 *  reliability analysis.
 */
template<degree Degree, domain Domain>
class reliability_manager : public diagram_manager<double, Degree, Domain>
{
public:
    using diagram_t =
        typename diagram_manager<double, Degree, Domain>::diagram_t;

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
    auto calculate_probabilities(Ps const& ps, diagram_t sf) -> void;

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
    auto probability(int32 j, Ps const& ps, diagram_t sf) -> double;

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
    auto get_probability(int32 j) const -> double;

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
    template<component_probabilities Ps, class Foo = void>
    requires(is_bss<Degree>)
    auto availability(Ps const& ps, diagram_t sf) -> second_t<Foo, double>;

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
    auto availability(int32 j, Ps const& ps, diagram_t sf) -> double;

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
    auto get_availability() const -> second_t<Foo, double>;

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
    auto get_availability(int32 j) const -> double;

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
    template<component_probabilities Ps, class Foo = void>
    requires(is_bss<Degree>)
    auto unavailability(Ps const& ps, diagram_t sf) -> second_t<Foo, double>;

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
    auto unavailability(int32 j, Ps const& ps, diagram_t sf) -> double;

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
    auto get_unavailability() -> second_t<Foo, double>;

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
    auto get_unavailability(int32 j) -> double;

    /**
     *  \brief Returns system state frequency of state \p j .
     *  \param sf Structure function.
     *  \param j System state.
     *  \return Frequency of system state \p j .
     */
    auto state_frequency(diagram_t sf, int32 j) -> double;

    /**
     *  \brief Calculates Direct Partial Boolean Derivative.
     *
     *  \param var Change of the value of \p i th component.
     *  \param f Change of the value of the system.
     *  \param sf Structure function.
     *  \param i Index of the component.
     *  \return Diagram representing Direct Partial Boolean Derivative.
     */
    auto dpld(value_change var, value_change f, diagram_t sf, int32 i)
        -> diagram_t;

    /**
     *  \brief Calculates Direct Partial Boolean Derivative of type 1.
     *
     *  Identifies situations in which change of the state of \p i th
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
    auto idpld_type_1_decrease(
        value_change var, int32 j, diagram_t sf, int32 i
    ) -> diagram_t;

    /**
     *  \brief Calculates Direct Partial Boolean Derivative of type 1.
     *
     *  Identifies situations in which change of the state of \p i th
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
    auto idpld_type_1_increase(
        value_change var, int32 j, diagram_t sf, int32 i
    ) -> diagram_t;

    /**
     *  \brief Calculates Direct Partial Boolean Derivative of type 2.
     *
     *  Identifies situations in which change of the state of \p i th
     *  component causes degradation of the system state.
     *
     *  \param var Change of the value of \p i th component.
     *  \param sf Structure function.
     *  \param i Index of the component.
     *  \return Diagram representing Direct Partial Boolean Derivative
     *  of type 2.
     */
    auto idpld_type_2_decrease(value_change var, diagram_t sf, int32 i)
        -> diagram_t;

    /**
     *  \brief Calculates Direct Partial Boolean Derivative of type 2.
     *
     *  Identifies situations in which change of the state of \p i th
     *  component causes improvement of the system state.
     *
     *  \param var Change of the value of \p i th component.
     *  \param sf Structure function.
     *  \param i Index of the component.
     *  \return Diagram representing Direct Partial Boolean Derivative
     *  of type 2.
     */
    auto idpld_type_2_increase(value_change var, diagram_t sf, int32 i)
        -> diagram_t;

    /**
     *  \brief Calculates Direct Partial Boolean Derivative of type 2.
     *
     *  Identifies situations in which change ot the state of \p i th
     *  component causes system state degradation from a state
     *  at least \p j to a state worse than \p j .
     *
     *  \param var Change of the value of \p i th component.
     *  \param j System state.
     *  \param sf Structure function.
     *  \param i Index of the component.
     *  \return Diagram representing Direct Partial Boolean Derivative
     *  of type 3.
     */
    auto idpld_type_3_decrease(
        value_change var, int32 j, diagram_t sf, int32 i
    ) -> diagram_t;

    /**
     *  \brief Calculates Direct Partial Boolean Derivative of type 2.
     *
     *  Identifies situations in which change ot the state of \p i th
     *  component causes system state improvement from a state
     *  state worse than \p j to a state at least \p j .
     *
     *  \param var Change of the value of \p i th component.
     *  \param j System state.
     *  \param sf Structure function.
     *  \param i Index of the component.
     *  \return Diagram representing Direct Partial Boolean Derivative
     *  of type 3.
     */
    auto idpld_type_3_increase(
        value_change var, int32 j, diagram_t sf, int32 i
    ) -> diagram_t;

    /**
     * \brief Transforms \p dpld into Extended DPLD
     * \param varFrom Value from whitch the variable changes
     * \param i Index of the variable
     * \param varFrom Derivative
     * \return Diagram representing Extended DPLD
     */
    auto to_dpld_e(int32 varFrom, int32 i, diagram_t dpld) -> diagram_t;

    /**
     *  \brief Calculates Structural Importace (SI) of a component
     *
     *  Different types of DPLDs can be used for the calculation.
     *  It is up to the user to pick the one that suits his needs.
     *
     *  \param dpld Direct Partial Boolean Derivative of any type
     *  \return Structural importance of given component
     */
    auto structural_importance(diagram_t dpld) -> double;

    /**
     *  \brief Calculates Birnbaum importance (BI) of a component.
     *
     *  Different types of DPLDs can be used for the calculation.
     *  It is up to the user to pick the one that suits his needs.
     *
     *  \param ps Component state probabilities
     *  \param var Component state change
     *  \param dpld Direct Partial Boolean Derivative of any type
     *  \param i Index of the component
     *  \return Birnbaum importance of given component
     */
    template<component_probabilities Ps>
    auto birnbaum_importance(Ps const& ps, value_change var, diagram_t dpld, int32 i) -> double;

    /**
     *  \brief Finds all Minimal Cut Vector (MCVs) of the system with
     *  respect to the system state \p j .
     *
     *  This function uses \c satisfy_all function. Please keep in mind
     *  that for bigger systems, there can be a huge number of MCVs.
     *
     *  \tparam Vars Container type that defines \c operator[] and allows
     *  assigning unsigned integers.
     *  \param sf Structure function.
     *  \param j System state.
     *  \returns Vector of Minimal Cut Vectors.
     */
    template<out_var_values Vars>
    auto mcvs(diagram_t sf, int32 j) -> std::vector<Vars>;

    /**
     *  \brief Finds all Minimal Cut Vector of the system with respect
     *  to the system state \p j .
     *
     *  This function uses \c satisfy_all_g function. Please keep in mind
     *  that for bigger systems, there can be a huge number of MCVs.
     *
     *  \tparam Vars Container type that defines \c operator[] and allows
     *  assigning unsigned integers.
     *  \param sf Structure function.
     *  \param j System state.
     *  \param out Output iterator that is used to output instances
     *  of \p Vars .
     */
    template<out_var_values Vars, std::output_iterator<Vars> Out>
    auto mcvs_g(diagram_t sf, int32 j, Out out) -> void;

protected:
    reliability_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        std::vector<int32> order
    )
    requires(domains::is_fixed<Domain>()());

    reliability_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        domains::mixed,
        std::vector<int32> order
    )
    requires(domains::is_mixed<Domain>()());

private:
    using node_t = typename diagram_manager<double, Degree, Domain>::node_t;

    // TODO this will be merged with apply_dpld in the future
    template<f_val_change F>
    auto dpld_g(diagram_t sf, value_change var, int32 i, F change)
        -> diagram_t;

    // TODO toto by mohlo preberat aj zmenu premennej
    // potom by to nebralo dva diagramy ale iba jeden - priamo
    // strukturnu funkciu. Prehladavanie v apply by sa modifikovalo
    // podla zmien premennych.
    template<f_val_change F>
    auto apply_dpld(diagram_t, diagram_t, F) -> diagram_t;

    auto domain_size() const -> int64;

    template<component_probabilities Ps>
    auto calculate_ntp (std::vector<int32> const& selected, Ps const& ps, diagram_t d) -> double;
};

template<degree Degree, domain Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::calculate_probabilities(
    Ps const& ps, diagram_t sf
) -> void
{
    auto const root = sf.unsafe_get_root();

    this->nodes_.traverse_pre(
        root,
        [](auto const n)
        {
            n->data() = 0.0;
        }
    );
    this->nodes_.for_each_terminal_node(
        [](auto const n)
        {
            n->data() = 0.0;
        }
    );
    root->data() = 1.0;

    this->nodes_.traverse_level(
        root,
        [this, &ps](auto const node)
        {
            if (node->is_internal())
            {
                auto const nodeIndex = node->get_index();
                auto k               = 0;
                this->nodes_.for_each_son(
                    node,
                    [node, nodeIndex, &ps, &k](auto const son)
                    {
                        son->data() += node->data()
                                     * ps[as_uindex(nodeIndex)][as_uindex(k)];
                        ++k;
                    }
                );
            }
        }
    );
}

template<degree Degree, domain Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::probability(
    int32 const j, Ps const& ps, diagram_t f
) -> double
{
    // return this->calculate_ntp({j}, ps, f);
    return this->calculate_ntp(std::vector<int32>({j}), ps, f);
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::get_probability(int32 const j) const
    -> double
{
    auto const n = this->nodes_.get_terminal_node(j);
    return n ? n->data() : 0.0;
}

template<degree Degree, domain Domain>
template<component_probabilities Ps, class Foo>
requires(is_bss<Degree>)
auto reliability_manager<Degree, Domain>::availability(
    Ps const& ps, diagram_t f
) -> second_t<Foo, double>
{
    return this->availability(1, ps, f);
}

template<degree Degree, domain Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::availability(
    int32 const j, Ps const& ps, diagram_t f
) -> double
{
    auto js = std::vector<int32>();
    this->nodes_.for_each_terminal_node([j, &js](auto const n)
    {
        if (n->get_value() >= j)
        {
            js.emplace_back(n->get_value());
        }
    });
    return this->calculate_ntp(js, ps, f);
}

template<degree Degree, domain Domain>
template<class Foo>
auto reliability_manager<Degree, Domain>::get_availability() const
    -> second_t<Foo, double>
{
    auto const node = this->nodes_.get_terminal_node(1);
    return node ? node->data() : 0;
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::get_availability(int32 const j) const
    -> double
{
    auto A = .0;
    this->nodes_.for_each_terminal_node(
        [j, &A](auto const node)
        {
            if (node->get_value() >= j)
            {
                A += node->data();
            }
        }
    );
    return A;
}

template<degree Degree, domain Domain>
template<component_probabilities Ps, class Foo>
requires(is_bss<Degree>)
auto reliability_manager<Degree, Domain>::unavailability(
    Ps const& ps, diagram_t f
) -> second_t<Foo, double>
{
    return this->unavailability(1, ps, f);
}

template<degree Degree, domain Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::unavailability(
    int32 const j, Ps const& ps, diagram_t f
) -> double
{
    auto js = std::vector<int32>();
    this->nodes_.for_each_terminal_node([j, &js](auto const n)
    {
        if (n->get_value() < j)
        {
            js.emplace_back(n->get_value());
        }
    });
    return this->calculate_ntp(js, ps, f);
}

template<degree Degree, domain Domain>
template<class Foo>
auto reliability_manager<Degree, Domain>::get_unavailability()
    -> second_t<Foo, double>
{
    return this->get_unavailability(1);
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::get_unavailability(int32 const j)
    -> double
{
    auto result = .0;
    this->nodes_.for_each_terminal_node(
        [j, &result](auto const node)
        {
            if (node->get_value() < j)
            {
                result += node->data();
            }
        }
    );
    return result;
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::state_frequency(
    diagram_t sf, int32 j
) -> double
{
    return static_cast<double>(this->satisfy_count(j, sf)) /
           static_cast<double>(this->domain_size());
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::dpld(
    value_change const var, value_change const f, diagram_t sf, int32 const i
) -> diagram_t
{
    return this->dpld_g(
        sf,
        var,
        i,
        [f](auto const l, auto const r)
        {
            return l == f.from && r == f.to;
        }
    );
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::idpld_type_1_decrease(
    value_change var, int32 j, diagram_t sf, int32 i
) -> diagram_t
{
    return this->dpld_g(
        sf,
        var,
        i,
        [j](auto const l, auto const r)
        {
            return l == j && r < j;
        }
    );
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::idpld_type_1_increase(
    value_change var, int32 j, diagram_t sf, int32 i
) -> diagram_t
{
    return this->dpld_g(
        sf,
        var,
        i,
        [j](auto const l, auto const r)
        {
            return l == j && r > j;
        }
    );
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::idpld_type_2_decrease(
    value_change var, diagram_t sf, int32 i
) -> diagram_t
{
    return this->dpld_g(
        sf,
        var,
        i,
        [](auto const l, auto const r)
        {
            return l > r;
        }
    );
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::idpld_type_2_increase(
    value_change var, diagram_t sf, int32 i
) -> diagram_t
{
    return this->dpld_g(
        sf,
        var,
        i,
        [](auto const l, auto const r)
        {
            return l < r;
        }
    );
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::idpld_type_3_decrease(
    value_change const var, int32 const j, diagram_t sf, int32 const i
) -> diagram_t
{
    return this->dpld_g(
        sf,
        var,
        i,
        [j](auto const l, auto const r)
        {
            return l >= j && r < j;
        }
    );
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::idpld_type_3_increase(
    value_change const var, int32 const j, diagram_t sf, int32 const i
) -> diagram_t
{
    return this->dpld_g(
        sf,
        var,
        i,
        [j](auto const l, auto const r)
        {
            return l < j && r >= j;
        }
    );
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::to_dpld_e(
    int32 varFrom, int32 i, diagram_t dpld
) -> diagram_t
{
    auto const root      = dpld.unsafe_get_root();
    auto const rootLevel = this->nodes_.get_level(root);
    auto const varLevel  = this->nodes_.get_level(i);

    if (varLevel < rootLevel)
    {
        auto sons = this->nodes_.make_sons(
            i,
            [this, varFrom, root](int32 const k)
            {
                return k == varFrom ? root
                                    : this->nodes_.terminal_node(Undefined);
            }
        );
        auto const newRoot = this->nodes_.internal_node(i, std::move(sons));
        return diagram_t(newRoot);
    }

    auto memo     = std::unordered_map<node_t*, node_t*>();
    auto const go = [=, this, &memo](auto const& self, node_t* const n)
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
            [=, this, &self](int32 const k)
            {
                auto const son      = n->get_son(k);
                auto const sonLevel = this->nodes_.get_level(son);
                if (varLevel > nodeLevel && varLevel < sonLevel)
                {
                    // A new node goes in between the current node
                    // and its k th son.
                    // Transformation does not need to continue.
                    auto newNodeSons = this->nodes_.make_sons(
                        i,
                        [this, varFrom, son](int32 const l)
                        {
                            return l == varFrom
                                        ? son
                                        : this->nodes_.terminal_node(
                                            Undefined
                                        );
                        }
                    );
                    return this->nodes_.internal_node(
                        i, std::move(newNodeSons)
                    );
                }
                else
                {
                    // A new node will be inserted somewhere deeper.
                    return self(self, son);
                }
            }
        );
        auto const transformedNode =
            this->nodes_.internal_node(nodeIndex, std::move(sons));
        memo.emplace(n, transformedNode);
        return transformedNode;
    };
    return diagram_t(go(go, root));
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::structural_importance(diagram_t dpld)
    -> double
{
    auto const from       = int32(0);
    auto const to         = static_cast<int32>(this->get_var_count());
    auto const domainSize = this->nodes_.domain_product(from, to);
    return static_cast<double>(this->satisfy_count(1, dpld)) /
           static_cast<double>(domainSize);
}

template<degree Degree, domain Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::birnbaum_importance
    (Ps const& ps, value_change const var, diagram_t dpld, int32 const i) -> double
{
    auto const dplde = this->to_dpld_e(var.from, i, dpld);
    // this->calculate_probabilities(ps, dplde);
    // return this->get_probability(1);
    return this->probability(1, ps, dplde);
}

template<degree Degree, domain Domain>
template<out_var_values Vars>
auto reliability_manager<Degree, Domain>::mcvs(diagram_t sf, int32 const j)
    -> std::vector<Vars>
{
    auto cuts = std::vector<Vars>();
    this->mcvs_g<Vars>(sf, j, std::back_inserter(cuts));
    return cuts;
}

template<degree Degree, domain Domain>
template<out_var_values Vars, std::output_iterator<Vars> Out>
auto reliability_manager<Degree, Domain>::mcvs_g(
    diagram_t sf, int32 const j, Out out
) -> void
{
    auto const varCount = this->get_var_count();
    auto dpldes         = std::vector<diagram_t>();

    for (auto i = 0; i < varCount; ++i)
    {
        auto const varDomain = this->nodes_.get_domain(i);
        for (auto varFrom = 0; varFrom < varDomain - 1; ++varFrom)
        {
            auto const varChange = value_change {varFrom, varFrom + 1};
            auto const dpld = this->idpld_type_3_increase(varChange, j, sf, i);
            dpldes.emplace_back(this->to_dpld_e(varFrom, i, dpld));
        }
    }

    auto const conj = this->template tree_fold<ops::PI_CONJ>(dpldes);
    this->template satisfy_all_g<Vars, Out>(1, conj, out);
}

template<degree Degree, domain Domain>
template<f_val_change F>
auto reliability_manager<Degree, Domain>::dpld_g(
    diagram_t sf, value_change var, int32 i, F change
) -> diagram_t
{
    auto const lhs = this->cofactor(sf, i, var.from);
    auto const rhs = this->cofactor(sf, i, var.to);
    return this->apply_dpld(lhs, rhs, change);
}

template<degree Degree, domain Domain>
template<f_val_change F>
auto reliability_manager<Degree, Domain>::apply_dpld(
    diagram_t lhs, diagram_t rhs, F change
) -> diagram_t
{
    using cache_pair = struct
    {
        node_t* left;
        node_t* right;
    };
    auto constexpr cache_pair_hash = [](auto const p)
    {
        auto const hash1 = std::hash<node_t*>()(p.left);
        auto const hash2 = std::hash<node_t*>()(p.right);
        auto result      = size_t{0};
        result ^= hash1 + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= hash2 + 0x9e3779b9 + (result << 6) + (result >> 2);
        return result;
    };
    auto constexpr cache_pair_equals = [](auto const l, auto const r)
    {
        return l.left == r.left && l.right == r.right;
    };
    auto cache = std::unordered_map<
        cache_pair,
        node_t*,
        decltype(cache_pair_hash),
        decltype(cache_pair_equals)>();

    auto const go = [this, &cache, change](
                        auto const& self, auto const l, auto const r
                    ) -> node_t*
    {
        auto const cached = cache.find(cache_pair {l, r});
        if (cached != std::end(cache))
        {
            return cached->second;
        }

        auto const lhsVal = node_value(l);
        auto const rhsVal = node_value(r);
        auto const opVal // TODO toto by sa dalo lepsie
            = lhsVal == Nondetermined || rhsVal == Nondetermined
                  ? Nondetermined
                  : static_cast<int32>(change(lhsVal, rhsVal));
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
            auto sons           = this->nodes_.make_sons(
                topIndex,
                [=, &self](auto const k)
                {
                    auto const fst =
                        lhsLevel == topLevel
                                      ? l->get_son(k)
                                      : l; // TODO tu by bol get_son, ktory by
                                 // preskakoval fixovane premenne
                    auto const snd = rhsLevel == topLevel ? r->get_son(k) : r;
                    return self(self, fst, snd);
                }
            );

            u = this->nodes_.internal_node(topIndex, std::move(sons));
        }

        // TODO in place
        cache.emplace(std::make_pair(cache_pair {l, r}, u));
        return u;
    };

auto const newRoot = go(go, lhs.unsafe_get_root(), rhs.unsafe_get_root());
    return diagram_t(newRoot);
}

template<degree Degree, domain Domain>
auto reliability_manager<Degree, Domain>::domain_size() const -> int64
{
    auto const from = int32(0);
    auto const to   = this->get_var_count();
    return this->nodes_.domain_product(from, to);
}

template<degree Degree, domain Domain>
template<component_probabilities Ps>
auto reliability_manager<Degree, Domain>::calculate_ntp
    (std::vector<int32> const& selected, Ps const& ps, diagram_t d) -> double
{
    this->nodes_.for_each_terminal_node([](auto const n)
    {
        n->data() = 0.0;
    });

    for (auto const s : selected)
    {
        auto const n = this->nodes_.get_terminal_node(s);
        if (n)
        {
            n->data() = 1.0;
        }
    }

    this->nodes_.traverse_post(
        d.unsafe_get_root(),
        [this, &ps](auto const n) mutable
        {
            if (not n->is_terminal())
            {
                n->data()    = 0.0;
                auto const i = n->get_index();
                this->nodes_.for_each_son(
                    n,
                    [=, k = 0, &ps](auto const son) mutable
                    {
                        auto const p = ps[as_uindex(i)][as_uindex(k)];
                        n->data() += son->data() * p;
                        ++k;
                    }
                );
            }
        }
    );
    return d.unsafe_get_root()->data();
}

template<degree Degree, domain Domain>
reliability_manager<Degree, Domain>::reliability_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order
)
requires(domains::is_fixed<Domain>()())
    : diagram_manager<double, Degree, Domain>(
          varCount, nodePoolSize, overflowNodePoolSize, std::move(order)
      )
{
}

template<degree Degree, domain Domain>
reliability_manager<Degree, Domain>::reliability_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    domains::mixed ds,
    std::vector<int32> order
)
requires(domains::is_mixed<Domain>()())
    : diagram_manager<double, Degree, Domain>(
          varCount,
          nodePoolSize,
          overflowNodePoolSize,
          std::move(ds),
          std::move(order)
      )
{
}
} // namespace teddy

#endif
