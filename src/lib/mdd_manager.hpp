#ifndef MIX_DD_MDD_MANAGER_HPP
#define MIX_DD_MDD_MANAGER_HPP

#include "diagrams/mdd.hpp"
#include "diagrams/vertex_manager.hpp"
#include "diagrams/var_vals.hpp"
#include "diagrams/operators.hpp"
#include <array>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    class mdd_manager
    {
    /* Public aliases */
    public:
        using mdd_t      = mdd<VertexData, ArcData, P>;
        using mdd_v      = std::vector<mdd_t>;
        using vertex_t   = vertex<VertexData, ArcData, P>;
        using log_t      = typename log_val_traits<P>::type;
        using log_v      = std::vector<log_t>;
        using index_v    = std::vector<index_t>;
        using level_v    = std::vector<level_t>;
        using prob_table = std::vector<std::array<double, P>>;
        using double_v   = std::vector<double>;

    /* Constructors */
    public:
        mdd_manager (std::size_t const varCount);
        mdd_manager (mdd_manager const&) = delete;

    /* Creation */
    public:
        /**
            @return Diagram with single vertex representing constant value @p val .
         */
        auto constant (log_t const val) -> mdd_t;

        /**
            @return Diagram representing @p i th variable.
         */
        auto variable (index_t const i) -> mdd_t;

        /**
            @return std::vector of diagrams representing @p is ths variables.
         */
        auto variables (index_v const& is) -> mdd_v;

        /**
            See variable above.
         */
        auto operator() (index_t const i) -> mdd_t;

        /**
               TODO
         */
        template<class InputIt>
        auto from_vector (InputIt first, InputIt last) -> mdd_t;

        /**
               TODO
         */
        template<class Range>
        auto from_vector (Range&& range) -> mdd_t;

    /* Tools */
    public:
        /**
            @return Number of vertices.
         */
        auto vertex_count () const -> std::size_t;

        /**
            @return Number of vertices in @p d .
         */
        auto vertex_count (mdd_t const& d) const -> std::size_t;

        /**
            @return Number of vertices with given index.
         */
        auto vertex_count (index_t const i) const -> std::size_t;

        /**
            @brief Prints DOT representation of all vertices into @p ost .
         */
        auto to_dot_graph (std::ostream& ost) const -> void;

        /**
            @brief Prints DOT representation of @p d into @p ost .
         */
        auto to_dot_graph (std::ostream& ost, mdd_t const& d) const -> void;

        /**
            @return Number of different variable assignments for which @p d evaluates to @p val .
         */
        auto satisfy_count (log_t const val, mdd_t& d) -> std::size_t;

        /**
            @return std::vector containing variable indices that are in @p d .
         */
        auto dependency_set (mdd_t const& d) -> std::vector<index_t>;

        /**
            @brief Enumerates variable indices that are in @p d into @p out .
         */
        template<class OutputIt>
        auto dependency_set_g (mdd_t const& d, OutputIt out) -> void;

        /**
            @return Value of @p d for variable assignment @p vs .
            @tparam VariableValues something that holds values of individual variables.
            @tparam GetIthVar function object that extract value of i-th variable from @p vs .
                    std::vector, std::array, std::bitset (for P = 2), integral type (for P = 2)
                    are supported by default. In case you want to use custom one, check
                    var_vals.hpp for implementation details.
         */
        template< class VariableValues
                , class GetIthVar = get_var_val<P, VariableValues> >
        auto evaluate (mdd_t const& d, VariableValues const& vs) const -> log_t;

        /**
            @return std::vector of all different variable assignments for which @p d
                    evaluates to @p val .
            @tparam VariableValues something that holds values of individual variables.
            @tparam SetIthVar function object that sets value of i-th variable in VariableValues {}.
                    std::vector, std::array, std::bitset (for P = 2), integral type (for P = 2)
                    are supported by default. In case you want to use custom one, check
                    var_vals.hpp for implementation details.
         */
        template< class VariableValues
                , class SetIthVar = set_var_val<P, VariableValues> >
        auto satisfy_all (log_t const val, mdd_t const& d) const -> std::vector<VariableValues>;

        /**
            @brief Generic version of satisfy_all.
                   Variable assignments are enumerated into @p out .
            @tparam OutputIt output iterator.
         */
        template< class VariableValues
                , class OutputIt
                , class SetIthVar = set_var_val<P, VariableValues> >
        auto satisfy_all_g (log_t const val, mdd_t const& d, OutputIt out) const -> void;

    /* Manipulation */
    public:
        /**
            @return Result of combining @p lhs and @p rhs with given binary operation.
            @tparam Op see README for table of available operations.
         */
        template<template<std::size_t> class Op>
        auto apply (mdd_t const& lhs, mdd_t const& rhs) -> mdd_t;

        /**
            @return Function @p d where @p i th variable is set to constant @p val .
         */
        auto restrict_var (mdd_t const& d, index_t const i, log_t const val) -> mdd_t;

        /**
            @return Function @p d where each terminal vertex > 1 is transformed into 1.
         */
        auto booleanize (mdd_t const& d) -> mdd_t;

        /**
            @brief Combines diagrams in @p ds using given binary operator.
                   Associates to left: (((d1 op d2) op d3) op d4) .. op dn
            @result Result of combining.
         */
        template<template<std::size_t> class Op>
        auto left_fold (mdd_v const& ds) -> mdd_t;

        /**
            @brief Combines diagrams in @p ds using given binary operator.
                   Associates in tree-like way: ((d1 op d2) op (d3 op d4)) op (d5 op d6)
                   Uses @p ds for storing partial results.
            @result Result of combining.
         */
        template<template<std::size_t> class Op>
        auto tree_fold (mdd_v& ds) -> mdd_t;

        /**
            @brief Generic version of left_fold.
         */
        template<template<std::size_t> class Op, class InputIt>
        auto left_fold (InputIt first, InputIt last) -> mdd_t;

        /**
            @brief Generic version of tree_fold.
         */
        template<template<std::size_t> class Op, class RandomIt>
        auto tree_fold (RandomIt first, RandomIt last) -> mdd_t;

        /**
            @brief TODO
         */
        auto reduce (mdd_t const& d) -> mdd_t;

    /* Reliability */
    public:
        /**
            @brief Calculates probability of each leaf in @p f based on @p ps .
            @param ps 2D array where ps[i][j] is the probability that i-th variable has value j.
         */
        auto calculate_probabilities (prob_table const& ps, mdd_t& f) -> void;

        /**
            @return Probability in @p level th leaf.
                    Returned value is undefined if calculate_probabilities was not called before.
         */
        auto get_probability (log_t const level) const -> double;

        /**
            @return Availability for logical @p level .
                    Returned value is undefined if calculate_probabilities was not called before.
         */
        auto get_availability (log_t const level) const -> double;

        /**
            @return Unavailability for logical @p level .
                    Returned value is undefined if calculate_probabilities was not called before.
         */
        auto get_unavailability (log_t const level) const -> double;

        /**
            @brief Calculates and returns availability.
                   Same as calling calculate_probabilities(ps, f) and get_availability(level).
            @param ps 2D array where ps[i][j] is the probability that i-th variable has value j.
            @return Availability for logical @p level .
         */
        auto availability (log_t const level, prob_table const& ps, mdd_t& f) -> double;

        /**
            @brief Calculates and returns unavailability.
                   Same as calling calculate_probabilities(ps, f) and get_unavailability(level).
            @param ps 2D array where ps[i][j] is the probability that i-th variable has value j.
            @return Availability for logical @p level .
         */
        auto unavailability (log_t const level, prob_table const& ps, mdd_t& f) -> double;

        /**
            @brief Calculates Direct Partial Boolean derivative.
            @param var change of @p i th variable.
            @param f   change of function.
            @param sf  diagram representing structure function.
            @return diagram representing derivative.
         */
        auto dpbd (val_change<P> const var, val_change<P> const f, mdd_t const& sf, index_t const i) -> mdd_t;

        /**
            @brief Calculates Integrated Direct Partial Boolean derivative of type 1.
            @param var  change of @p i th variable.
            @param fVal value of the function.
            @param sf   diagram representing structure function.
            @return diagram representing derivative.
         */
        auto dpbd_integrated_1 (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i) -> mdd_t;

        /**
            @brief Calculates Integrated Direct Partial Boolean derivative of type 2.
            @param var change of @p i th variable.
            @param sf  diagram representing structure function.
            @return diagram representing derivative.
         */
        auto dpbd_integrated_2 (val_change<P> const var, mdd_t const& sf, index_t const i) -> mdd_t;

        /**
            @brief Calculates Integrated Direct Partial Boolean derivative of type 2.
            @param var change of @p i th variable.
            @param fVal value of the function.
            @param sf  diagram representing structure function.
            @return diagram representing derivative.
         */
        auto dpbd_integrated_3 (val_change<P> const var, log_t const fVal, mdd_t const& sf, index_t const i) -> mdd_t;

        /**
            @brief Calculates Direct Partial Boolean Derivative for each variable.
            @param var change of i-th variable.
            @param f   change of the function.
            @param sf  diagram representing structure function.
            @return std::vector of diagrams representing derivatives.
         */
        auto dpbds (val_change<P> const var, val_change<P> const f, mdd_t const& sf) -> mdd_v;

        /**
            @brief Calculates Direct Partial Boolean Derivative of type 1 for each variable.
            @param var  change of i-th variable.
            @param fVal value of the function.
            @param sf   diagram representing structure function.
            @return std::vector of diagrams representing derivatives.
         */
        auto dpbds_integrated_1 (val_change<P> const var, log_t const fVal, mdd_t const& sf) -> mdd_v;

        /**
            @brief Calculates Direct Partial Boolean Derivative of type 2 for each variable.
            @param var  change of i-th variable.
            @param sf   diagram representing structure function.
            @return std::vector of diagrams representing derivatives.
         */
        auto dpbds_integrated_2 (val_change<P> const var, mdd_t const& sf) -> mdd_v;

        /**
            @brief Calculates Direct Partial Boolean Derivative of type 3 for each variable.
            @param var  change of i-th variable.
            @param fVal value of the function.
            @param sf   diagram representing structure function.
            @return std::vector of diagrams representing derivatives.
         */
        auto dpbds_integrated_3 (val_change<P> const var, log_t const fVal, mdd_t const& sf) -> mdd_v;

        /**
            @param dpbd diagram representing Direct Partial Boolean Derivative of @p i th variable.
            @return Structural importance of @p i th variable.
         */
        auto structural_importance (mdd_t& dpbd, index_t const i) -> double;

        /**
            @param dpbds std::vector of diagrams representing Direct Partial Boolean Derivatives.
            @return std::vector of Structural importances.
         */
        auto structural_importances (mdd_v& dpbds) -> double_v;

        /**
            @param dpbd diagram representing Direct Partial Boolean Derivative of i-th variable.
            @param ps 2D array where ps[i][j] is the probability that i-th variable has value j.
            @return Birnbaum importance of i-th variable.
         */
        auto birnbaum_importance (prob_table const& ps, mdd_t& dpbd) -> double;

        /**
            @param dpbd diagram representing Direct Partial Boolean Derivative of i-th variable.
            @param ps 2D array where ps[i][j] is the probability that i-th variable has value j.
            @return std::vector of Birnbaum importances.
         */
        auto birnbaum_importances (prob_table const& ps, mdd_v& dpbds) -> double_v;

        /**
            @param dpbd diagram representing Direct Partial Boolean Derivative of i-th variable.
            @param ps 2D array where ps[i][j] is the probability that i-th variable has value j.
            @param U  Unavailability.
            @return Fussell-vesely importance of i-th variable.
         */
        auto fussell_vesely_importance (prob_table const& ps, double const U, mdd_t const& dpbd) -> double;

        /**
            @param dpbd diagram representing Direct Partial Boolean Derivative of i-th variable.
            @param ps 2D array where ps[i][j] is the probability that i-th variable has value j.
            @return std::vector of Fussell-vesely importances.
         */
        auto fussell_vesely_importances (prob_table const& ps, double const U, mdd_v const& dpbds) -> double_v;

        /**
            @brief Finds all Minimal Cut Vectors.
            @param dpbds std::vector of Direct Partial Boolean Derivatives.
            @tparam VariableValues something that holds values of individual variables.
            @return std::vector of all Minimal Cut Vectors.
         */
        template< class VariableValues
                , class SetIthVar = set_var_val<P, VariableValues> >
        auto mcvs (mdd_t const& sf, log_t const logLevel) -> std::vector<VariableValues>;

        /**
            @brief Generic version of mcvs. MCVs are enumerated into @p out .
            @param sf       Diagram representing structure function.
            @param logLevel Logical level of the @p sf for which MCVs should be calculated.
            @tparam OutputIt output iterator.
         */
        template< class VariableValues
                , class OutputIt
                , class SetIthVar = set_var_val<P, VariableValues> >
        auto mcvs_g (mdd_t const& sf, log_t const logLevel, OutputIt out) -> void;

    /* Other */
    public:
        auto set_domains     (log_v domains)        -> void;
        auto set_order       (index_v levelToIndex) -> void;
        auto get_order       () const               -> index_v const&;
        auto swap_vars       (index_t const i)      -> void;
        auto clear           () -> void;
        auto clear_cache     () -> void;
        auto collect_garbage () -> void;

    /* Internal aliases */
    protected:
        using vertex_a         = std::array<vertex_t*, P>;
        using vertex_v         = std::vector<vertex_t*>;
        using vertex_vv        = std::vector<vertex_v>;
        using apply_key_t      = std::tuple<vertex_t*, op_id_t, vertex_t*>;
        using apply_memo_t     = std::unordered_map<apply_key_t, vertex_t*, utils::tuple_hash_t<apply_key_t>>;
        using transform_key_t  = vertex_t*;
        using transform_memo_t = std::unordered_map<vertex_t*, vertex_t*>;
        using vertex_manager_t = vertex_manager<VertexData, ArcData, P>;

    /* Tools internals */
    public:
        template<class VertexIterator>
        auto to_dot_graph_impl (std::ostream& ost, VertexIterator for_each_v) const -> void;

        auto domain_product (level_t const from, level_t const to) const -> std::size_t;

        auto fill_levels (mdd_t const& d) const -> vertex_vv;

        template< class VariableValues
                , class OutputIt
                , class SetIthVar = set_var_val<P, VariableValues> >
        auto satisfy_all_step ( log_t const     val
                              , level_t const   l
                              , vertex_t* const v
                              , VariableValues& xs
                              , OutputIt&       out ) const -> void;

        template<class VertexOp>
        auto traverse_pre (mdd_t const& d, VertexOp&& op) const -> void;

        template<class VertexOp>
        auto traverse_post (mdd_t const& d, VertexOp&& op) const -> void;

        template<class VertexOp>
        auto traverse_level (mdd_t const& d, VertexOp&& op) const -> void;

    /* Manipulation internals */
    protected:
        template<class Transformator>
        auto transform (mdd_t const& d, Transformator&& transform_sons) -> mdd_t;

        template<class Transformator>
        auto transform_step (Transformator&& transform_sons, vertex_t* const v) -> vertex_t*;

    /* Creator internals */
    protected:
        auto variable   (index_t const i, log_t const domain) -> mdd_t;
        auto operator() (index_t const i, log_t const domain) -> mdd_t;

        template<class LeafVals>
        auto variable_impl (index_t const i, LeafVals&& vals, std::size_t const domain = P) -> mdd_t;

    /* Reliability internals */
    protected:
        auto sum_terminals         (log_t const from, log_t const to) const    -> double;
        auto structural_importance (std::size_t const domainSize, mdd_t& dpbd) -> double;
        auto to_dpbd_e             (log_t const varFrom, index_t const varIndex, mdd_t const& dpbd) -> mdd_t;
        auto to_mnf                (mdd_t const& dpbd) -> mdd_t;

    /* Member variables */
    protected:
        vertex_manager_t manager_;
        transform_memo_t transformMemo_;
        log_v            domains_;
    };

    template<std::size_t P>
    auto make_mdd_manager(std::size_t const varCount);

    template<class VertexData, class ArcData, std::size_t P>
    auto register_manager(mdd_manager<VertexData, ArcData, P>& m);

    template<class VertexData, class ArcData, std::size_t P>
    auto operator&& ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator|| ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator^ ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator== ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator!= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator< ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator<= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator> ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator>= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator+ ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator* ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>;

    template<class VertexData, class ArcData, std::size_t P>
    auto operator! ( mdd<VertexData, ArcData, P> const& lhs ) -> mdd<VertexData, ArcData, P>;
}

#include "diagrams/mdd_manager.ipp"
#include "diagrams/mdd_manager_tools.ipp"
#include "diagrams/mdd_manager_manipulator.ipp"
#include "diagrams/mdd_manager_creator.ipp"
#include "diagrams/mdd_manager_reliability.ipp"

#endif