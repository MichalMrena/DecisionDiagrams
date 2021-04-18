#ifndef MIX_DD_BDD_MANAGER_HPP
#define MIX_DD_BDD_MANAGER_HPP

#include "mdd_manager.hpp"
#include "diagrams/pla_file.hpp"

namespace teddy
{
    enum class fold_e {left, tree};

    template<class VertexData, class ArcData>
    class bdd_manager : public mdd_manager<VertexData, ArcData, 2>
    {
    /// Public aliases
    public:
        using base       = mdd_manager<VertexData, ArcData, 2>;
        using bdd_t      = typename base::mdd_t;
        using vertex_t   = typename base::vertex_t;

    /// Constructors
    public:
        /**
         *  @param varCount Number of variables that you want to use.
         *  @param vertexCount Initial count of pre-allocated vertices.
         *  Value of @p vertexCount might have a significant impact on performance.
         */
        bdd_manager (std::size_t varCount, std::size_t vertexCount = 10'000);
        bdd_manager (bdd_manager const&) = delete;

    /// Creation
    public:
        /**
         *  @return BDD representing complemented @p i th variable.
         */
        auto variable_not (index_t i) -> bdd_t;

        /**
         *  @return BDD representing complemented @p i th variable.
         */
        auto operator() (index_t i, NOT) -> bdd_t;

        /**
         *  @return @c std::vector of BDDs representing @p vars variables.
         */
        auto variables (std::vector<bool_var> const& vars) -> std::vector<bdd_t>;

        /**
         *  @return BDD representing product of @p vars variables.
         */
        auto product (std::vector<bool_var> const& vars) -> bdd_t;

        /**
         *  @return BDD representing product of @p cube .
         */
        auto product (bool_cube const& cube) -> bdd_t;

        // TODO add sum

        /**
         *  @brief Creates diagrams from PLA @p file .
         *  @return @c std::vector of BDDs of functions from the file.
         */
        auto from_pla (pla_file const& file, fold_e const mm = fold_e::tree) -> std::vector<bdd_t>;

        using base::operator();

    /// Tools
    public:
        /**
         *  @return Number of different variable assignments for which @p d evaluates to 1.
         */
        auto satisfy_count (bdd_t& d) -> std::size_t;

        /**
         *  @return Truth density of @p d .
         */
        auto truth_density (bdd_t& d) -> double;

        /**
         *  @return @c std::vector of all different variable assignments for which @p d
         *          evaluates to 1.
         *  @tparam VariableValues something that holds values of individual variables.
         *  @tparam SetIthVar function object that sets value of i-th variable in @c VariableValues{} .
         *          @c std::vector, @c std::array, @c std::bitset, integral type
         *          are supported by default. In case you want to use custom one, check
         *          @c var_vals.hpp for implementation details.
         */
        template< class VariableValues
                , class SetIthVar = set_var_val<2, VariableValues> >
        auto satisfy_all (bdd_t const& d) const -> std::vector<VariableValues>;

        /**
         *  @brief Generic version of satisfy_all.
         *         Variable assignments are enumerated into @p out .
         *  @tparam OutputIt output iterator.
         */
        template< class VariableValues
                , class OutputIt
                , class SetIthVar = set_var_val<2, VariableValues> >
        auto satisfy_all_g (bdd_t const& d, OutputIt out) const -> void;

    /// Manipulation
    public:
        /**
         *  @return BDD representing negation of @p d .
         */
        auto negate (bdd_t const& d) -> bdd_t;

    /// Reliability
    public:
        /**
         *  @brief Calculates probability of each leaf in @p f based on @p ps .
         *  @param ps @c std::vector of probabilities where number at index i
         *               is probability that i-th component is in state 1.
         */
        auto calculate_probabilities (std::vector<double> const& ps, bdd_t& f) -> void;

        /**
         *  @return Probability that system is in state 1.
         *  Returned value is undefined if calculate_probabilities was not called before.
         */
        auto get_availability () const -> double;

        /**
         *  @return Probability that system is in state 0.
         *  Returned value is undefined if calculate_probabilities was not called before.
         */
        auto get_unavailability () const -> double;

        /**
         *  @brief Calculates and returns availability.
         *  Same as calling @c calculate_probabilities(ps,f) and @c get_availability() .
         *  @param ps @c std::vector of probabilities where number at index i
         *               is probability that i-th component is in state 1.
         *  @return System availability.
         */
        auto availability (std::vector<double> const& ps, bdd_t& f) -> double;

        /**
         *  @brief Calculates and returns unavailability.
         *  Same as calling @c calculate_probabilities(ps,f) and @c get_unavailability() .
         *  @param ps @c std::vector of probabilities where number at index i
         *               is probability that i-th component is in state 1.
         *  @return System unavailability.
         */
        auto unavailability (std::vector<double> const& ps, bdd_t& f)-> double;

        /**
         *  @brief Calculates Direct Partial Boolean derivative.
         *  @param f function to derivate.
         *  @param i index of variable to derivate by.
         *  @return BDD representing the derivative.
         */
        auto dpbd (bdd_t const& f, index_t i) -> bdd_t;

        /**
         *  @brief Calculates Direct Partial Boolean Derivative for each variable.
         *  @param f function to derivate.
         *  @return @c std::vector of BDDs representing derivatives.
         */
        auto dpbds (bdd_t const& f) -> std::vector<bdd_t>;

        /**
         *  @return Structural importance based on @p dpbd .
         */
        auto structural_importance (bdd_t& dpbd) -> double;

        /**
         *  @brief Calculates SI of each component based on @p dpbds .
         *  @return @c std::vector of SIs.
         */
        auto structural_importances (std::vector<bdd_t>& dpbds) -> std::vector<double>;

        /**
         *  @param ps @c std::vector of probabilities where number at index i
         *               is probability that i-th component is in state 1.
         *  @return Birnbaum imporance based on @p dpbd .
         */
        auto birnbaum_importance (std::vector<double> const& ps, bdd_t& dpbd) -> double;

        /**
         *  @brief Calculates BI for each component based on @p dpbds .
         *  @return @c std::vector of BIs.
         */
        auto birnbaum_importances (std::vector<double> const& ps, std::vector<bdd_t>& dpbds) -> std::vector<double>;

        /**
         *  @param BI Birnbaum importance.
         *  @param qi Probability that i-th component is in state 0.
         *  @param U  System unavailability.
         *  @return Criticality importance of i-th component base on Birnbaum index.
         */
        auto criticality_importance (double BI, double qi, double U) -> double;

        /**
         *  @brief Calculates CI for each component based on @p BIs , @p ps , @p U .
         *  @return @c std::vector of CIs.
         */
        auto criticality_importances (std::vector<double> const& BIs, std::vector<double> const& ps, double U) -> std::vector<double>;

        /**
         *  @param ps @c std::vector of probabilities where number at index i
         *               is probability that i-th component is in state 1.
         *  @param qi Probability that i-th component is in state 0.
         *  @param U  System unavailability.
         *  @return Fussel-Vesely importance of i-th component.
         */
        auto fussell_vesely_importance (bdd_t& dpbd, double qi, std::vector<double> const& ps, double U) -> double;

        /**
         *  @brief Calculates FI for each component based on @p ps , @p U .
         *  @return @c std::vector of FIs.
         */
        auto fussell_vesely_importances (std::vector<bdd_t>& dpbds, std::vector<double> const& ps, double U) -> std::vector<double>;

        /**
         *  @brief Enumerates all Minimal Cut Vectors of a system based on @p dpbds .
         *  @return @c std::vector of @p VectorType .
         */
        template<class VectorType>
        auto mcvs (std::vector<bdd_t> const& dpbds) -> std::vector<VectorType>;

    /// Internal aliases
    private:
        using prob_table = typename base::prob_table;
        using vertex_a      = typename base::vertex_a;

    /// Creation internals
    private:
        auto or_merge (std::vector<bdd_t>& diagrams, fold_e) -> bdd_t;

    /// Reliability internals
    public:
        auto to_prob_table (std::vector<double> const& ps) -> prob_table;
        auto to_mnf        (bdd_t const& dpbd)  -> bdd_t;
        auto to_dpbd_e     (index_t const i, bdd_t const& dpbd) -> bdd_t;
    };

    /**
     *  @brief Factory function for creating instance of @c bdd_manager .
     */
    inline auto make_bdd_manager(std::size_t varCount, std::size_t vertexCount = 10'000);

    /**
     *  @brief @c negate .
     */
    template<class VertexData, class ArcData>
    auto operator! (mdd<VertexData, ArcData, 2> const& lhs) -> mdd<VertexData, ArcData, 2>;
}

#include "diagrams/bdd_manager_creator.ipp"
#include "diagrams/bdd_manager_reliability.ipp"
#include "diagrams/bdd_manager_manipulator.ipp"
#include "diagrams/bdd_manager_tools.ipp"
#include "diagrams/bdd_manager.ipp"

#endif