#ifndef MIX_DD_MDD_MANAGER_HPP
#define MIX_DD_MDD_MANAGER_HPP

#include "diagrams/mdd.hpp"
#include "diagrams/vertex_manager.hpp"
#include "diagrams/var_vals.hpp"
#include "diagrams/operators.hpp"
#include <array>

namespace teddy
{
    template<class VertexData, class ArcData, std::size_t P>
    class mdd_manager
    {
    /// Public aliases
    public:
        using mdd_t      = mdd<VertexData, ArcData, P>;
        using vertex_t   = vertex<VertexData, ArcData, P>;
        using log_t      = typename log_val_traits<P>::type;
        using prob_table = std::vector<std::array<double, P>>;

    /// Constructors
    public:
        /**
         *  @param varCount Number of variables that you want to use.
         *  @param vertexCount Initial count of pre-allocated vertices.
         *  Value of @p vertexCount might have a significant impact on performance.
         */
        mdd_manager (std::size_t varCount, std::size_t vertexCount = 10'000);
        mdd_manager (mdd_manager const&) = delete;

    /// Technical stuff
    public:
        /**
         *  @brief Sets domains of variables.
         *  Domain -> number of values that a variable can take.
         *  Number at index i in @p domains is domain of i/-th variable.
         */
        auto set_domains (std::vector<log_t> domains) -> void;

        /**
         *  @brief Sets initial order of variables.
         *  Nuber at level j in @p levelToIndex is index of variable at level j.
         *  Variables are ordered by their indices by default.
         */
        auto set_order (std::vector<index_t> levelToIndex) -> void;

        /**
         *  @return Const reference to current order of variables.
         *  Order returned by this function will be different from
         *  the default order of from an order set by @c set_order
         *  iif dynamic reordering is enabled.
         */
        auto get_order () const -> std::vector<index_t> const&;

        /**
         *  @brief Sets size of the apply cache.
         *  Size of cache tables is calculated by @c vertex_count() / @p denominator .
         */
        auto set_cache_ratio (std::size_t denominator) -> void;

        /**
         *  @brief Sets ratio of additionally allocated vertices.
         *  Number of additionally allocated vertices is calculated
         *  by @c init_vertex_count / @p denominator .
         */
        auto set_pool_ratio (std::size_t denominator) -> void;

        /**
         *  @brief Enables/disables dynamic reordering od variables.
         */
        auto set_dynamic_reorder (bool reorder) -> void;

    /// Creation
    public:
        /**
         *  @return Diagram with single vertex representing constant value @p val .
         */
        auto constant (log_t val) -> mdd_t;

        /**
         *  @return Diagram representing @p i th variable.
         */
        auto variable (index_t i) -> mdd_t;

        /**
         *  @return @c std::vector of diagrams representing @p is ths variables.
         */
        auto variables (std::vector<index_t> const& is) -> std::vector<mdd_t>;

        /**
         *  @return Diagram representing @p i th variable.
         */
        auto operator() (index_t i) -> mdd_t;

        /**
         *  @brief Creates a diagram from a truth vector.
         *  @param first First element of the truth vector. Bools or 0/1 can be used
         *  @param last First past last element of the truth vector.
         *
         *  Example of a truth vector and corresponding truth table.
         *  x1 x2  f
         *   0  0  0
         *   0  1  0    ->   [0001]
         *   1  0  0
         *   1  1  1
         */
        template<class InputIt>
        auto from_vector (InputIt first, InputIt last) -> mdd_t;

        /**
         *  @brief See @c from_vector above. This version only gets iterators from @p range .
         */
        template<class Range>
        auto from_vector (Range&& range) -> mdd_t;

    /// Tools
    public:
        /**
         *  @return Number of vertices.
         */
        auto vertex_count () const -> std::size_t;

        /**
         *  @return Number of vertices in @p d .
         */
        auto vertex_count (mdd_t const& d) const -> std::size_t;

        /**
         *  @return Number of vertices with index @p i.
         */
        auto vertex_count (index_t i) const -> std::size_t;

        /**
         *  @brief Prints DOT representation of all vertices into @p ost .
         */
        auto to_dot_graph (std::ostream& ost) const -> void;

        /**
         *  @brief Prints DOT representation of @p d into @p ost .
         */
        auto to_dot_graph (std::ostream& ost, mdd_t const& d) const -> void;

        /**
         *  @return Number of different variable assignments for which @p d evaluates to @p val .
         */
        auto satisfy_count (log_t val, mdd_t& d) -> std::size_t;

        /**
         *  @return @c std::vector containing variable indices that are in @p d .
         */
        auto dependency_set (mdd_t const& d) -> std::vector<index_t>;

        /**
         *  @brief Enumerates variable indices that are in @p d into @p out .
         */
        template<class OutputIt>
        auto dependency_set_g (mdd_t const& d, OutputIt out) -> void;

        /**
         *  @return Value of @p d for variable values in @p vs .
         *  @tparam VariableValues something that holds values of individual variables.
         *  @tparam GetIthVar function object that extract value of i-th variable from @p vs .
         *          @c std::vector, @c std::array, @c std::bitset (for P = 2), integral type (for P = 2)
         *          are supported by default. In case you want to use custom one, check
         *          var_vals.hpp for implementation details.
         */
        template< class VariableValues
                , class GetIthVar = get_var_val<P, VariableValues> >
        auto evaluate (mdd_t const& d, VariableValues const& vs) const -> log_t;

        /**
         *  @return @c std::vector of all different variable assignments for which @p d
         *          evaluates to @p val .
         *  @tparam VariableValues something that holds values of individual variables.
         *  @tparam SetIthVar function object that sets value of i-th variable in @c VariableValues{} .
         *          @c std::vector, @c std::array, @c std::bitset (for P = 2), integral type (for P = 2)
         *          are supported by default. In case you want to use custom one, check
         *          @c var_vals.hpp for implementation details.
         */
        template< class VariableValues
                , class SetIthVar = set_var_val<P, VariableValues> >
        auto satisfy_all (log_t val, mdd_t const& d) const -> std::vector<VariableValues>;

        /**
         *  @brief Generic version of satisfy_all.
         *         Variable assignments are enumerated into @p out .
         *  @tparam OutputIt output iterator.
         */
        template< class VariableValues
                , class OutputIt
                , class SetIthVar = set_var_val<P, VariableValues> >
        auto satisfy_all_g (log_t val, mdd_t const& d, OutputIt out) const -> void;

    /// Manipulation
    public:
        /**
         *  @return Result of combining @p lhs and @p rhs with given binary operation.
         *  @tparam Op see README.md for table of available operations.
         */
        template<template<std::size_t> class Op>
        auto apply (mdd_t const& lhs, mdd_t const& rhs) -> mdd_t;

        /**
         *  @return Function @p d where @p i th variable is set to constant @p val .
         */
        auto cofactor (mdd_t const& d, index_t i, log_t val) -> mdd_t;

        /**
         *  @return Function @p d where each terminal vertex > 1 is transformed into 1.
         */
        auto booleanize (mdd_t const& d) -> mdd_t;

        /**
         *  @brief Combines diagrams in @p ds using given binary operation.
         *         Associates to the left: (((d1 op d2) op d3) op d4) .. op dn
         *  @result Result of combining.
         */
        template<template<std::size_t> class Op>
        auto left_fold (std::vector<mdd_t> const& ds) -> mdd_t;

        /**
         *  @brief Combines diagrams in @p ds using given binary operation.
         *         Associates to the right:
         *  @result Result of combining.
         */
        template<template<std::size_t> class Op>
        auto right_fold (std::vector<mdd_t> const& ds) -> mdd_t;

        /**
         *  @brief Combines diagrams in @p ds using given binary operation.
         *         Associates in tree-like way: ((d1 op d2) op (d3 op d4)) op (d5 op d6)
         *         Uses @p ds for storing partial results.
         *  @result Result of combining.
         */
        template<template<std::size_t> class Op>
        auto tree_fold (std::vector<mdd_t>& ds) -> mdd_t;

        /**
         *  @brief Generic version of left_fold.
         */
        template<template<std::size_t> class Op, class InputIt>
        auto left_fold (InputIt first, InputIt last) -> mdd_t;

        /**
         *  @brief Generic version of tree_fold.
         */
        template<template<std::size_t> class Op, class RandomIt>
        auto tree_fold (RandomIt first, RandomIt last) -> mdd_t;

        /**
         *  @brief Reduces @p d into canonical form.
         *  This function is relevant only if dynamic reordering is enabled.
         *  Otherwise for each @c d; @c d.equals(reduce(d)) .
         */
        auto reduce (mdd_t const& d) -> mdd_t;

    /// Reliability
    public:
        /**
         *  @brief Calculates probability of each leaf in @p f based on @p ps .
         *  @param ps 2D array where @c ps[i][j] is the probability that i-th variable has value j.
         */
        auto calculate_probabilities (prob_table const& ps, mdd_t& f) -> void;

        /**
         *  @return Probability in @p level th leaf.
         *          Returned value is undefined if calculate_probabilities was not called before.
         */
        auto get_probability (log_t level) const -> double;

        /**
         *  @return Availability for logical @p level .
         *          Returned value is undefined if calculate_probabilities was not called before.
         */
        auto get_availability (log_t level) const -> double;

        /**
         *  @return Unavailability for logical @p level .
         *          Returned value is undefined if calculate_probabilities was not called before.
         */
        auto get_unavailability (log_t level) const -> double;

        /**
         *  @brief Calculates and returns availability.
         *  Same as calling @c calculate_probabilities(ps,f) and @c get_availability(level) .
         *  @param ps 2D array where @c ps[i][j] is the probability that i-th variable has value j.
         *  @return Availability for logical @p level .
         */
        auto availability (log_t level, prob_table const& ps, mdd_t& f) -> double;

        /**
         *  @brief Calculates and returns unavailability.
         *  Same as calling @c calculate_probabilities(ps,f) and @c get_unavailability(level) .
         *  @param ps 2D array where @c ps[i][j] is the probability that i-th variable has value j.
         *  @return Availability for logical @p level .
         */
        auto unavailability (log_t level, prob_table const& ps, mdd_t& f) -> double;

        /**
         *  @brief Calculates Direct Partial Boolean derivative.
         *  @param var change of the @p i th variable.
         *  @param f   change of the function.
         *  @param sf  diagram representing structure function.
         *  @return diagram representing the derivative.
         */
        auto dpbd (val_change<P> var, val_change<P> f, mdd_t const& sf, index_t i) -> mdd_t;

        /**
         *  @brief Calculates Integrated Direct Partial Boolean derivative of type 1.
         *  @param var  change of @p i th variable.
         *  @param fVal value of the function.
         *  @param sf   diagram representing structure function.
         *  @return diagram representing derivative.
         */
        auto dpbd_integrated_1 (val_change<P> var, log_t fVal, mdd_t const& sf, index_t i) -> mdd_t;

        /**
         *  @brief Calculates Integrated Direct Partial Boolean derivative of type 2.
         *  @param var change of @p i th variable.
         *  @param sf  diagram representing structure function.
         *  @return diagram representing derivative.
         */
        auto dpbd_integrated_2 (val_change<P> var, mdd_t const& sf, index_t i) -> mdd_t;

        /**
         *  @brief Calculates Integrated Direct Partial Boolean derivative of type 2.
         *  @param var change of @p i th variable.
         *  @param fVal value of the function.
         *  @param sf  diagram representing structure function.
         *  @return diagram representing derivative.
         */
        auto dpbd_integrated_3 (val_change<P> var, log_t fVal, mdd_t const& sf, index_t i) -> mdd_t;

        /**
         *  @brief Calculates Direct Partial Boolean Derivative for each variable.
         *  @param var change of i-th variable.
         *  @param f   change of the function.
         *  @param sf  diagram representing structure function.
         *  @return @c std::vector of diagrams representing derivatives.
         */
        auto dpbds (val_change<P> var, val_change<P> f, mdd_t const& sf) -> std::vector<mdd_t>;

        /**
         *  @brief Calculates Direct Partial Boolean Derivative of type 1 for each variable.
         *  @param var  change of i-th variable.
         *  @param fVal value of the function.
         *  @param sf   diagram representing structure function.
         *  @return @c std::vector of diagrams representing derivatives.
         */
        auto dpbds_integrated_1 (val_change<P> var, log_t fVal, mdd_t const& sf) -> std::vector<mdd_t>;

        /**
         *  @brief Calculates Direct Partial Boolean Derivative of type 2 for each variable.
         *  @param var  change of i-th variable.
         *  @param sf   diagram representing structure function.
         *  @return @c std::vector of diagrams representing derivatives.
         */
        auto dpbds_integrated_2 (val_change<P> var, mdd_t const& sf) -> std::vector<mdd_t>;

        /**
         *  @brief Calculates Direct Partial Boolean Derivative of type 3 for each variable.
         *  @param var  change of i-th variable.
         *  @param fVal value of the function.
         *  @param sf   diagram representing structure function.
         *  @return @c std::vector of diagrams representing derivatives.
         */
        auto dpbds_integrated_3 (val_change<P> var, log_t fVal, mdd_t const& sf) -> std::vector<mdd_t>;

        /**
         *  @param dpbd diagram representing Direct Partial Boolean Derivative of @p i th variable.
         *  @return Structural importance of @p i th variable.
         */
        auto structural_importance (mdd_t& dpbd, index_t i) -> double;

        /**
         *  @param dpbds vector of diagrams representing Direct Partial Boolean Derivatives.
         *  @return @c std::vector of Structural importances.
         */
        auto structural_importances (std::vector<mdd_t>& dpbds) -> std::vector<double>;

        /**
         *  @param dpbd diagram representing Direct Partial Boolean Derivative of i-th variable.
         *  @param ps 2D array where @c ps[i][j] is the probability that i-th variable has value j.
         *  @return Birnbaum importance of i-th variable.
         */
        auto birnbaum_importance (prob_table const& ps, mdd_t& dpbd) -> double;

        /**
         *  @param dpbd diagram representing Direct Partial Boolean Derivative of i-th variable.
         *  @param ps 2D array where @c ps[i][j] is the probability that i-th variable has value j.
         *  @return @c std::vector of Birnbaum importances.
         */
        auto birnbaum_importances (prob_table const& ps, std::vector<mdd_t>& dpbds) -> std::vector<double>;

        /**
         *  @param dpbd diagram representing Direct Partial Boolean Derivative of i-th variable.
         *  @param ps 2D array where @c ps[i][j] is the probability that i-th variable has value j.
         *  @param U  Unavailability.
         *  @return Fussell-vesely importance of i-th variable.
         */
        auto fussell_vesely_importance (prob_table const& ps, double U, mdd_t const& dpbd) -> double;

        /**
         *  @param dpbd diagram representing Direct Partial Boolean Derivative of i-th variable.
         *  @param ps 2D array where @c ps[i][j] is the probability that i-th variable has value j.
         *  @return @c std::vector of Fussell-vesely importances.
         */
        auto fussell_vesely_importances (prob_table const& ps, double U, std::vector<mdd_t> const& dpbds) -> std::vector<double>;

        /**
         *  @brief Finds all Minimal Cut Vectors.
         *  @param dpbds vector of Direct Partial Boolean Derivatives.
         *  @tparam VariableValues something that holds values of individual variables.
         *  @return @c std::vector of all Minimal Cut Vectors.
         */
        template< class VariableValues
                , class SetIthVar = set_var_val<P, VariableValues> >
        auto mcvs (mdd_t const& sf, log_t logLevel) -> std::vector<VariableValues>;

        /**
         *  @brief Generic version of mcvs. MCVs are enumerated into @p out .
         *  @param sf       Diagram representing structure function.
         *  @param logLevel Logical level of the @p sf for which MCVs should be calculated.
         *  @tparam OutputIt output iterator.
         */
        template< class VariableValues
                , class OutputIt
                , class SetIthVar = set_var_val<P, VariableValues> >
        auto mcvs_g (mdd_t const& sf, log_t logLevel, OutputIt out) -> void;

    /// Just for testing. Will be private and accessed via friend relationship.
    public:
        auto swap_vars       (index_t const i) -> void;
        auto clear           () -> void;
        auto clear_cache     () -> void;
        auto collect_garbage () -> void;
        auto sift_variables  () -> void;

    /// Internal aliases
    protected:
        using vertex_a         = std::array<vertex_t*, P>;
        using transform_key_t  = vertex_t*;
        using transform_memo_t = std::unordered_map<vertex_t*, vertex_t*>;
        using vertex_manager_t = vertex_manager<VertexData, ArcData, P>;

    /// Tools internals
    public:
        template<class VertexIterator>
        auto to_dot_graph_impl (std::ostream& ost, VertexIterator for_each_v) const -> void;

        auto domain_product (level_t const from, level_t const to) const -> std::size_t;

        auto fill_levels (mdd_t const& d) const -> std::vector<std::vector<vertex_t*>>;

        template<class VertexOp>
        auto traverse_pre (mdd_t const& d, VertexOp&& op) const -> void;

        template<class VertexOp>
        auto traverse_post (mdd_t const& d, VertexOp&& op) const -> void;

        template<class VertexOp>
        auto traverse_level (mdd_t const& d, VertexOp&& op) const -> void;

    /// Manipulation internals
    protected:
        template<class Transformator>
        auto transform (mdd_t const& d, Transformator&& transform_sons) -> mdd_t;

        template<class Transformator>
        auto transform_step (Transformator&& transform_sons, vertex_t* const v) -> vertex_t*;

    /// Reliability internals
    protected:
        auto sum_terminals         (log_t const from, log_t const to) const    -> double;
        auto structural_importance (std::size_t const domainSize, mdd_t& dpbd) -> double;
        auto to_dpbd_e             (log_t const varFrom, index_t const varIndex, mdd_t const& dpbd) -> mdd_t;
        auto to_mnf                (mdd_t const& dpbd) -> mdd_t;

    /// Member variables
    protected:
        vertex_manager_t   manager_;
        transform_memo_t   transformMemo_;
        std::vector<log_t> domains_;
    };

    /**
     *  @brief Factory function for creating instance of @c mdd_manager .
     */
    template<std::size_t P>
    auto make_mdd_manager(std::size_t varCount, std::size_t vertexCount = 10'000);

    /**
     *  @brief Registers @p m so that you can use below declared overloaded operators.
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto register_manager(mdd_manager<VertexData, ArcData, P>& m);

    /**
     *  @brief @c apply<AND> .
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto operator&& ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    /**
     *  @brief @c apply<OR> .
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto operator|| ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    /**
     *  @brief @c apply<XOR> .
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto operator^ ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    /**
     *  @brief @c apply<EQUAL_TO> .
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto operator== ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    /**
     *  @brief @c apply<NOT_EQUAL_TO> .
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto operator!= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    /**
     *  @brief @c apply<LESS> .
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto operator< ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    /**
     *  @brief @c apply<LESS_EQUAL> .
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto operator<= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    /**
     *  @brief @c apply<GREATER> .
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto operator> ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    /**
     *  @brief @c apply<GREATER_EQUAL> .
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto operator>= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    /**
     *  @brief @c apply<PLUS> .
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto operator+ ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    /**
     *  @brief @c apply<MULTIPLIES> .
     */
    template<class VertexData, class ArcData, std::size_t P>
    auto operator* ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;
}

#include "diagrams/mdd_manager.ipp"
#include "diagrams/mdd_manager_tools.ipp"
#include "diagrams/mdd_manager_manipulator.ipp"
#include "diagrams/mdd_manager_creator.ipp"
#include "diagrams/mdd_manager_reliability.ipp"

#endif