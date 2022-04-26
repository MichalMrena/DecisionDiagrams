#ifndef MIX_DD_DIAGRAM_MANAGER_HPP
#define MIX_DD_DIAGRAM_MANAGER_HPP

#include "diagram.hpp"
#include "operators.hpp"
#include "node_manager.hpp"
#include "pla_file.hpp"
#include "utils.hpp"
#include <cmath>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <ranges>

namespace teddy
{
    template<class Vars>
    concept in_var_values = requires (Vars vs, index_t i)
    {
        { vs[i] } -> std::convertible_to<uint_t>;
    };

    template<class Vars>
    concept out_var_values = requires (Vars vs, index_t i, uint_t v)
    {
        vs[i] = v;
    };

    template<class Degree>
    concept is_bdd = std::same_as<degrees::fixed<2>, Degree>;

    enum class fold_type
    {
        Left,
        Tree
    };

    /**
     *  \class diagram_manager
     *  \brief Base class for all diagram managers that generically
     *  implements all of the algorithms.
     */
    template<class Data, degree Degree, domain Domain>
    class diagram_manager
    {
    public:
        /**
         *  \brief Alias for the diagram type used in the
         *  functions of this manager.
         */
        using diagram_t = diagram<Data, Degree>;

    public:
        /**
         *  \brief Creates diagram representing constant function.
         *  \param v Value of the constant function.
         *  \return Diagram representing constant function.
         */
        auto constant (uint_t v) -> diagram_t;

        /**
         *  \brief Creates diagram representing function of single variable.
         *  \param i Index of the variable.
         *  \return Diagram of a function of single variable.
         */
        auto variable (index_t i) -> diagram_t;

        /**
         *  \brief Creates BDD representing function of complemented variable.
         *  \param i Index of the variable.
         *  \return Diagram of a function of single variable.
         */
        template<class Foo = void> requires(is_bdd<Degree>)
        auto variable_not (index_t i) -> second_t<Foo, diagram_t>;

        /**
         *  \brief Creates diagram representing function of single variable.
         *  \param i Index of the variable.
         *  \return Diagram of a function of single variable.
         */
        auto operator() (index_t) -> diagram_t;

        /**
         *  \brief Creates vector of diagrams representing functions
         *  of single variables.
         *
         *  \tparam T integral type convertible to unsgined int.
         *  \param is initializer list of indices.
         *  \return Vector of diagrams.
         */
        template<std::convertible_to<index_t> T>
        auto variables (std::initializer_list<T> is) -> std::vector<diagram_t>;

        /**
         *  \brief Creates vector of diagrams representing functions
         *  of single variables.
         *
         *  \tparam I iterator type for the input range.
         *  \tparam S sentinel type for \p I . (end iterator)
         *  \param first iterator to the first element of range of indices
         *  represented by integral type convertible to unsgined int.
         *  \param last sentinel for \p first (end iterator).
         *  \return Vector of diagrams.
         */
        template<std::input_iterator I, std::sentinel_for<I> S>
        auto variables (I first, S last) -> std::vector<diagram_t>;

        /**
         *  \brief Creates vector of diagrams representing functions
         *  of single variables.
         *
         *  \tparam Is range of indices represented by integral type
         *  convertible to unsigned int.
         *  \param is range of Ts (e.g. std::vector<unsigned int>).
         *  \return Vector of diagrams.
         */
        template<std::ranges::input_range Is>
        auto variables (Is const& is) -> std::vector<diagram_t>;

        /**
         *  \brief Creates diagram from a truth vector of a function.
         *
         *  Example for the function f(x) = max(x0, x1, x2):
         *  \code
         *  // Truth table:
         *  +----+----+----+----+---+----+-----+----+---+
         *  | x1 | x2 | x3 | f  | _ | x1 |  x2 | x3 | f |
         *  +----+----+----+----+---+----+-----+----+---+
         *  | 0  | 0  | 0  | 0  |   | 1  |  0  | 0  | 1 |
         *  | 0  | 0  | 1  | 1  |   | 1  |  0  | 1  | 1 |
         *  | 0  | 0  | 2  | 2  |   | 1  |  0  | 2  | 2 |
         *  | 0  | 1  | 0  | 1  |   | 1  |  1  | 0  | 1 |
         *  | 0  | 1  | 1  | 1  |   | 1  |  1  | 1  | 1 |
         *  | 0  | 1  | 2  | 2  |   | 1  |  1  | 2  | 2 |
         *  +----+----+----+----+---+----+-----+----+---+
         *  \endcode
         *
         *  \code
         *  // Truth vector:
         *  [0 1 2 1 1 2 1 1 2 1 1 2]
         *  \endcode
         *
         *  \code
         *  // Teddy code:
         *  teddy::ifmdd_manager<3> manager(3, 100, {2, 2, 3});
         *  std::vector<unsigned int> vec {0, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2};
         *  diagram_t f = manager.from_vector(vec);
         *  \endcode
         *
         *  \tparam I Iterator type for the input range.
         *  \tparam S Sentinel type for \p I . (end iterator)
         *  \param first iterator to the first element of the truth vector.
         *  \param last sentinel for \p first . (end iterator)
         *  \return Diagram representing function given by the truth vector.
         */
        template<std::input_iterator I, std::sentinel_for<I> S>
        auto from_vector (I first, S last) -> diagram_t;

        /**
         *  \brief Creates diagram from a truth vector of a function.
         *
         *  See the other overload for details.
         *
         *  \tparam R Type of the range that contains the truth vector.
         *  \param vector Range representing the truth vector.
         *  Elements of the range must be convertible to unsigned int.
         *  \return Diagram representing function given by the truth vector.
         */
        template<std::ranges::input_range R>
        auto from_vector (R&& vector) -> diagram_t;

        /**
         *  \brief Creates truth vector from the diagram.
         *
         *  Significance of variables is the same as in the \c from_vector
         *  function i.e. variable on the last level of the diagram is
         *  least significant. The following assertion holds:
         *  \code
         *  assert(manager.from_vector(manager.to_vector(d)))
         *  \endcode
         *
         *  \param d Diagram.
         *  \return Vector of ints representing truth vector.
         */
        auto to_vector (diagram_t const& d) const -> std::vector<uint_t>;

        /**
         *  \brief Creates truth vector from the diagram.
         *
         *  \tparam O Output iterator type
         *  \param d Diagram.
         *  \param out Output iterator that is used to output the truth vector.
         */
        template<std::output_iterator<uint_t> O>
        auto to_vector_g (diagram_t const& d, O out) const -> void;

        /**
         *  \brief Creates BDDs defined by PLA file.
         *
         *  \tparam Foo Dummy template to enable SFINE.
         *  \param file PLA file loaded in the instance of \c pla_file class.
         *  \param foldType fold type used in diagram creation.
         *  \return Vector of diagrams.
         */
        template<class Foo = void> requires(is_bdd<Degree>)
        auto from_pla ( pla_file const& file
                      , fold_type foldType = fold_type::Tree )
                      -> second_t<Foo, std::vector<diagram_t>>;

        /**
         *  \brief Merges two diagrams using given binary operation.
         *
         *  Binary operations are defined in the namespace \c teddy::ops .
         *  All availabe operations are listed in the following table:
         *  \code
         *  +-------------------+---------------------------------------+
         *  | Binary operation  |           Description                 |
         *  +-------------------+---------------------------------------+
         *  | AND               | Logical and. ^                        |
         *  | OR                | Logical or. ^                         |
         *  | XOR               | Logical xor. ^                        |
         *  | NAND              | Logical nand. ^                       |
         *  | NOR               | Logical nor. ^                        |
         *  | EQUAL_TO          | Equal to relation. ^                  |
         *  | NOT_EQUAL_TO      | Not equal to relation. ^              |
         *  | LESS              | Less than relation. ^                 |
         *  | LESS_EQUAL        | Less than or equal relation. ^        |
         *  | GREATER           | Greater than relation. ^              |
         *  | GREATER_EQUAL     | Greater than or equal relation. ^     |
         *  | MIN               | Minimum of two values.                |
         *  | MAX               | Maximum of two values.                |
         *  | PLUS              | Modular addition: (a + b) mod P.      |
         *  | MULTIPLIES        | Modular multiplication: (a * b) mod P |
         *  +-------------------+---------------------------------------+
         *  ^ 0 is false and 1 is true
         *
         *  // Examples:
         *  manager.apply<teddy::ops::AND>(bdd1, bdd2);
         *  manager.apply<teddy::ops::PLUS<4>>(mdd1, mdd2);
         *  \endcode
         *
         *  \tparam Op Binary operation.
         *  \param l first diagram.
         *  \param r second diagram.
         *  \return Diagram representing merger of \p l and \p r .
         */
        template<bin_op Op>
        auto apply (diagram_t const& l, diagram_t const& r) -> diagram_t;

        /**
         *  \brief Merges diagams in a range using the \c apply function
         *  and binary operation.
         *
         *  Uses left fold order of evaluation (sequentially from the left).
         *
         *  \code
         *  // Example:
         *  std::vector<diagram_t> vs = manager.variables({0, 1, 2});
         *  diagram_t product = manager.left_fold<teddy::ops>(vs);
         *  \endcode
         *
         *  \tparam Op Binary operation.
         *  \tparam R Range containing diagrams (e.g. std::vector<diagram_t>).
         *  \param range Input range of diagrams to be merged.
         *  \return Diagram representing merger of all diagrams from the range.
         */
        template<bin_op Op, std::ranges::input_range R>
        auto left_fold (R const& range) -> diagram_t;

        /**
         *  \brief Merges diagams in a range using the \c apply function
         *  and binary operation.
         *
         *  Uses left fold order of evaluation (sequentially from the left).
         *
         *  \code
         *  // Example:
         *  std::vector<diagram_t> vs = manager.variables({0, 1, 2});
         *  diagram_t product = manager.left_fold<teddy::ops>(
         *      vs.begin(), vs.end());
         *  \endcode
         *
         *  \tparam Op Binary operation.
         *  \tparam I Range iterator type.
         *  \tparam S Sentinel type for \c I (end iterator).
         *  \param first Input iterator to the first diagram.
         *  \param last Sentinel for \p first (end iterator).
         *  \return Diagram representing merger of all diagrams from the range.
         */
        template< bin_op               Op
                , std::input_iterator  I
                , std::sentinel_for<I> S >
        auto left_fold (I first, S last) -> diagram_t;

        /**
         *  \brief Merges diagams in a range using the \c apply function
         *  and binary operation.
         *
         *  Uses tree fold order of evaluation ((d1 op d2) op (d3 op d4) ...) .
         *  Tree fold uses the input range \p range to store some intermediate
         *  results. \p range is left in valid but unspecified state.
         *
         *  \code
         *  // Example:
         *  std::vector<diagram_t> vs = manager.variables({0, 1, 2});
         *  diagram_t product = manager.tree_fold<teddy::ops>(vs);
         *  \endcode
         *
         *  \tparam Op Binary operation.
         *  \tparam R Range containing diagrams (e.g. std::vector<diagram_t>).
         *  \param range Random access range of diagrams to be merged.
         *  (e.g. std::vector)
         *  \return Diagram representing merger of all diagrams from the range.
         */
        template<bin_op Op, std::ranges::random_access_range R>
        auto tree_fold (R& range) -> diagram_t;

        /**
         *  \brief Merges diagams in a range using the \c apply function
         *  and binary operation.
         *
         *  Uses tree fold order of evaluation ((d1 op d2) op (d3 op d4) ...) .
         *  Tree fold uses the input range \p range to store some intermediate
         *  results. \p range is left in valid but unspecified state.
         *
         *  \code
         *  // Example:
         *  std::vector<diagram_t> vs = manager.variables({0, 1, 2});
         *  diagram_t product = manager.tree_fold<teddy::ops>(
         *      vs.begin(), vs.end());
         *  \endcode
         *
         *  \tparam Op Binary operation.
         *  \tparam I Range iterator type.
         *  \tparam S Sentinel type for \c I (end iterator).
         *  \param first Random access iterator to the first diagram.
         *  \param last Sentinel for \p first (end iterator).
         *  \return Diagram representing merger of all diagrams from the range.
         */
        template< bin_op                      Op
                , std::random_access_iterator I
                , std::sentinel_for<I>        S >
        auto tree_fold (I first, S last) -> diagram_t;

        /**
         *  \brief Evaluates value of the function represented by the diagram.
         *
         *  Complexity is \c O(n) where \c n is the number of variables.
         *
         *  \tparam Vars Container type that defines operator[] and returns
         *  value convertible to unsigned int.
         *  \param d Diagram.
         *  \param vs Container holding values of variables.
         *  \return Value of the function represented by \p d for variable
         *  values given in \p vs .
         */
        template<in_var_values Vars>
        auto evaluate (diagram_t const& d, Vars const& vs) const -> uint_t;

        /**
         *  \brief Calculates number of variable assignments for which
         *  the functions evaluates to 1.
         *
         *  Complexity is \c O(|d|) where \c |d| is the number of nodes.
         *
         *  \tparam Foo Dummy parameter to enable SFINE.
         *  \param d Diagram representing the function.
         *  \return Number of different variable assignments for which the
         *  the function represented by \p d evaluates to 1.
         */
        template<class Foo = void> requires(is_bdd<Degree>)
        auto satisfy_count (diagram_t& d) -> second_t<Foo, std::size_t>;

        /**
         *  \brief Calculates number of variable assignments for which
         *  the functions evaluates to certain value.
         *
         *  Complexity is \c O(|d|) where \c |d| is the number of nodes.
         *
         *  \param val Value of the function.
         *  \param d Diagram representing the function.
         *  \return Number of different variable assignments for which the
         *  the function represented by \p d evaluates to \p val .
         */
        auto satisfy_count (uint_t val, diagram_t& d) -> std::size_t;

        /**
         *  \brief Enumerates all elements of the satisfying set.
         *
         *  Enumerates all elements of the satisfying set of the function
         *  i.e. variable assignments for which the Boolean function
         *  evaluates to 1.
         *
         *  Complexity is \c O(n*|Sf|) where \c |Sf| is the size of the
         *  satisfying set and \c n is the number of variables. Please
         *  note that this is quite high for bigger functions and
         *  the computation will probably not finish in reasonable time.
         *
         *  \tparam Vars Container type that defines \c operator[] and allows
         *  assigning unsigned integers.
         *  \tparam Foo Dummy parameter to enable SFINE.
         *  \param d Diagram representing the function.
         *  \return Vector of \p Vars .
         */
        template<out_var_values Vars, class Foo = void> requires(is_bdd<Degree>)
        auto satisfy_all
            (diagram_t const& d) const -> second_t<Foo, std::vector<Vars>>;

        /**
         *  \brief Enumerates all elements of the satisfying set.
         *
         *  Enumerates all elements of the satisfying set of the function
         *  i.e. variable assignments for which the function
         *  evaluates to certain value.
         *
         *  Complexity is \c O(n*|Sf|) where \c |Sf| is the size of the
         *  satisfying set and \c n is the number of variables. Please
         *  note that this is quite high for bigger functions and
         *  the computation will probably not finish in reasonable time.
         *
         *  \tparam Vars Container type that defines \c operator[] and allows
         *  assigning unsigned integers.
         *  \param val Value of the function.
         *  \param d Diagram representing the function.
         *  \return Vector of \p Vars .
         */
        template<out_var_values Vars>
        auto satisfy_all
            (uint_t val, diagram_t const& d) const -> std::vector<Vars>;

        /**
         *  \brief Enumerates all elements of the satisfying set.
         *
         *  Enumerates all elements of the satisfying set of the function
         *  i.e. variable assignments for which the function
         *  evaluates to certain value.
         *
         *  Complexity is \c O(n*|Sf|) where \c |Sf| is the size of the
         *  satisfying set and \c n is the number of variables. Please
         *  note that this is quite high for bigger functions and
         *  the computation will probably not finish in reasonable time.
         *
         *  \tparam Vars Container type that defines \c operator[] and allows
         *  assigning unsigned integers.
         *  \tparam O Output iterator type.
         *  \param val Value of the function.
         *  \param d Diagram representing the function.
         *  \param out Output iterator that is used to output instances
         *  of \p Vars .
         */
        template< out_var_values             Vars
                , std::output_iterator<Vars> O >
        auto satisfy_all_g
            (uint_t val, diagram_t const& d, O out) const -> void;

        /**
         *  \brief Calculates cofactor of the functions.
         *
         *  Calculates cofactor of the function i.e. fixes value of the \p i th
         *  variable to the value \p val .
         *
         *  \param d Diagram representing the function.
         *  \param i Index of the variable to be fixed.
         *  \param val Value to which the \p i th varibale should be fixed.
         *  \return Diagram representing cofactor of the function.
         */
        auto cofactor (diagram_t const& d, index_t i, uint_t val) -> diagram_t;

        /**
         *  \brief Transforms values of the function.
         *
         *  \code
         *  // Example of the call with 4-valued MDD.
         *  manager.transform(diagram, [](unsigned int v)
         *  {
         *      return 3 - v;
         *  });
         *  \endcode
         *
         *  \tparam F Type of the transformation function.
         *  \param d Diagram representing the function.
         *  \param f Transformation function that is applied
         *  to values of the function. Default value keeps 0 and transform
         *  everything else to 1.
         *  \return Diagram representing transformed function.
         */
        template<uint_to_bool F>
        auto transform
            (diagram_t const& d, F f = utils::not_zero) -> diagram_t;

        /**
         *  \brief Enumerates indices of variables that the function depends on.
         *
         *  \param d Diagram representing the function.
         *  \return Vector of indices.
         */
        auto dependency_set (diagram_t const& d) const -> std::vector<index_t>;

        /**
         *  \brief Enumerates indices of variables that the function depends on.
         *
         *  \tparam O Output iterator type.
         *  \param d Diagram representing the function.
         *  \param out Output iterator that is used to output indices.
         *  \return Vector of indices.
         */
        template<std::output_iterator<index_t> O>
        auto dependency_set_g (diagram_t const& d, O out) const -> void;

        /**
         *  \brief Reduces diagrams to its canonical form.
         *
         *  You probably won't need to call this.
         *
         *  \param  d Diagram.
         *  \return Diagram in a reduced canonical form.
         */
        auto reduce (diagram_t const&) -> diagram_t;

        /**
         *  \brief Returns number of nodes that are currently
         *  used by the manager.
         *
         *  This function returns number of nodes that are currently stored
         *  in the unique tables. Total number of allocated nodes might
         *  and probably will be higher. See implementation details on github
         *  for details.
         *
         *  \return Number of nodes.
         */
        auto node_count () const -> std::size_t;

        /**
         *  \brief Returns number of nodes in the diagram including
         *  terminal nodes.
         *  \param d Diagram.
         *  \return Number of node.
         */
        auto node_count (diagram_t const& d) const -> std::size_t;

        /**
         *  \brief Prints dot representation of the graph.
         *
         *  Prints dot representation of the entire multi rooted graph to
         *  the output stream.
         *
         *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
         */
        auto to_dot_graph (std::ostream& out) const -> void;

        /**
         *  \brief Prints dot representation of the diagram.
         *
         *  Prints dot representation of the diagram to
         *  the output stream.
         *
         *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
         *  \param d Diagram.
         */
        auto to_dot_graph
            (std::ostream& out, diagram_t const& d) const -> void;

        /**
         *  \brief Runs garbage collection.
         *
         *  Forces the garbage collection to run which removes nodes that are
         *  not referenced from the unique tables. These nodes, however, are
         *  not deallocated. See implementation details on github
         *  for details.
         *  GC is run automatically so you probably won't need to run
         *  this function yourself.
         */
        auto gc () -> void;

        /**
         *  \brief Runs variable sifting algorithm that tries to minimize
         *  number of nodes.
         */
        auto sift () -> void;

        /**
         *  \brief Returns number of variables for this manager
         *  set in the constructor.
         *  \return Number of variables.
         */
        auto get_var_count () const -> std::size_t;

        /**
         *  \brief Returns current order of variables.
         *
         *  If no sifting was performed in between call to the constructor
         *  and call to this function then the order is the same as
         *  specified in the constructor.
         *
         *  \return Vector of indices. Index at l-th position is the
         *  index of variable at l-th level of the diagram.
         */
        auto get_order () const -> std::vector<index_t> const&;

        /**
         *  \brief Return domains of variables.
         *
         *  In case of \c bdd_manager and \c mdd_manager domains of each
         *  variable is the same i.e. 2 or P. In case of \c imdd_manager
         *  and \c ifmdd_manager the domains are the same as set
         *  in the constructor.
         *
         *  \return Vector of domains.
         */
        auto get_domains () const -> std::vector<uint_t>;

        /**
         *  \brief Sets the relative cache size w.r.t the number of nodes.
         *
         *  Size of the cache is calculated as:
         *  \code
         *  ratio * uniqueNodeCount
         *  \endcode
         *
         *  \param ratio Number from the interval (0,oo).
         */
        auto set_cache_ratio (double ratio) -> void;

        /**
         *  \brief Sets ratio used in calculation of size of the new node pool.
         *
         *  Size of the new additional pool as calculated as:
         *  \code
         *  ratio * initNodeCount.
         *  \endcode
         *
         *  \param ratio Number from the interval (0,1].
         */
        auto set_pool_ratio (double ratio) -> void;

        /**
         *  \brief Sets ratio used to determine new node pool allocation.
         *
         *  New pool is allocated if:
         *  \code
         *  garbageCollectedNodes < ratio * initNodeCount
         *  \endcode
         *
         *  \param ratio Number from the interval [0,1].
         */
        auto set_gc_ratio (double ratio) -> void;

    protected:
        using node_t = typename diagram<Data, Degree>::node_t;

    protected:

        // verzia apply ktora na cachovanie pouzije hash tabulku
        // a bin. operacia bude dana lambdou
        // template<class F>
        // auto apply_strict (diagram_t, diagram_t, F) -> diagram_t;

    private:
        template<class F>
        auto transform_internal (node_t*, F&&) -> node_t*;

        template<uint_to_uint F>
        auto transform_terminal (node_t*, F) -> node_t*;

    protected:
        /**
         *  \brief Initializes diagram manager.
         *
         *  This overload is for managers that have fixed domains
         *  (known at copile time).
         *
         *  \param varCount Number of variables.
         *  \param nodePoolSize Number of nodes that is pre-allocated.
         *  \param overflowNodePoolSize Size of the additional node pools.
         *  \param order Order of variables.
         */
        diagram_manager ( std::size_t varCount
                        , std::size_t nodePoolSize
                        , std::size_t overflowNodePoolSize
                        , std::vector<index_t> order )
                        requires(domains::is_fixed<Domain>()());

        /**
         *  \brief Initializes diagram manager.
         *
         *  This overload is for managers that have mixed domains
         *  specified by the \p ds paramter.
         *
         *  \param varCount Number of variables.
         *  \param nodePoolSize Number of nodes that is pre-allocated.
         *  \param overflowNodePoolSize Size of the additional node pools.
         *  \param ds Domains of varibales.
         *  \param order Order of variables.
         */
        diagram_manager ( std::size_t varCount
                        , std::size_t nodePoolSize
                        , std::size_t overflowNodePoolSize
                        , domains::mixed ds
                        , std::vector<index_t> order )
                        requires(domains::is_mixed<Domain>()());

    public:
        diagram_manager (diagram_manager const&) = delete;
        diagram_manager (diagram_manager&&)      = default;
        auto operator=  (diagram_manager const&) -> diagram_manager& = delete;
        auto operator=  (diagram_manager&&)      -> diagram_manager& = default;

    protected:
        node_manager<Data, Degree, Domain> nodes_;
    };

    namespace detail
    {
        inline auto default_or_fwd
            (std::size_t const n, std::vector<index_t>& is)
        {
            return is.empty()
                       ? utils::fill_vector(n, utils::identity)
                       : std::vector<index_t>(std::move(is));
        }
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::constant
        (uint_t const v) -> diagram_t
    {
        return diagram_t(nodes_.terminal_node(v));
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::variable
        (index_t const i) -> diagram_t
    {
        return diagram_t(nodes_.internal_node(i, nodes_.make_sons(i,
            [this](auto const v)
        {
            return nodes_.terminal_node(v);
        })));
    }

    template<class Data, degree Degree, domain Domain>
    template<class Foo> requires(is_bdd<Degree>)
    auto diagram_manager<Data, Degree, Domain>::variable_not
        (index_t const i) -> second_t<Foo, diagram_t>
    {
        return diagram_t(nodes_.internal_node(i, nodes_.make_sons(i,
            [this](auto const v)
        {
            return nodes_.terminal_node(1 - v);
        })));
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::operator()
        (index_t const i) -> diagram_t
    {
        return this->variable(i);
    }

    template<class Data, degree Degree, domain Domain>
    template<std::ranges::input_range Is>
    auto diagram_manager<Data, Degree, Domain>::variables
        (Is const& is) -> std::vector<diagram_t>
    {
        namespace rs = std::ranges;
        return this->variables(rs::begin(is), rs::end(is));
    }

    template<class Data, degree Degree, domain Domain>
    template<std::convertible_to<index_t> T>
    auto diagram_manager<Data, Degree, Domain>::variables
        (std::initializer_list<T> const is) -> std::vector<diagram_t>
    {
        namespace rs = std::ranges;
        return this->variables(rs::begin(is), rs::end(is));
    }

    template<class Data, degree Degree, domain Domain>
    template<std::input_iterator I, std::sentinel_for<I> S>
    auto diagram_manager<Data, Degree, Domain>::variables
        (I const first, S const last) -> std::vector<diagram_t>
    {
        static_assert(
            std::convertible_to<std::iter_value_t<I>, index_t> );
        return utils::fmap(first, last, [this](auto const i)
        {
            return this->variable(static_cast<index_t>(i));
        });
    }

    template<class Data, degree Degree, domain Domain>
    template<std::input_iterator I, std::sentinel_for<I> S>
    auto diagram_manager<Data, Degree, Domain>::from_vector
        (I first, S last) -> diagram_t
    {
        // TODO
        if (0 == this->get_var_count())
        {
            assert(first != last and std::next(first) == last);
            return diagram_t(nodes_.terminal_node(*first));
        }

        auto const lastLevel = static_cast<level_t>(this->get_var_count() - 1);
        auto const lastIndex = nodes_.get_index(lastLevel);

        if constexpr (std::random_access_iterator<I>)
        {
            namespace rs = std::ranges;
            [[maybe_unused]]
            auto const count = nodes_.domain_product(0, lastLevel + 1);
            [[maybe_unused]]
            auto const dist  = static_cast<std::size_t>(
                                   rs::distance(first, last));
            assert(dist > 0 && dist == count);
        }

        using stack_frame = struct { node_t* node; level_t level; };
        auto stack = std::vector<stack_frame>();
        auto const shrink_stack = [this, &stack]()
        {
            for (;;)
            {
                auto const currentLevel = stack.back().level;
                if (0 == currentLevel)
                {
                    break;
                }

                auto const end = std::rend(stack);
                auto it        = std::rbegin(stack);
                auto count     = 0ul;
                while (it != end and it->level == currentLevel)
                {
                    ++it;
                    ++count;
                }
                auto const newIndex  = nodes_.get_index(currentLevel - 1);
                auto const newDomain = nodes_.get_domain(newIndex);

                if (count < newDomain)
                {
                    break;
                }

                auto newSons = nodes_.make_sons(newIndex,
                    [&stack, newDomain](auto const o)
                {
                    return stack[stack.size() - newDomain + o].node;
                });
                auto const newNode = nodes_.internal_node( newIndex
                                                         , std::move(newSons) );
                stack.erase(std::end(stack) - newDomain, std::end(stack));
                stack.push_back(stack_frame {newNode, currentLevel - 1});
            }
        };

        while (first != last)
        {
            auto sons = nodes_.make_sons(lastIndex, [this, &first](auto const)
            {
                return nodes_.terminal_node(*first++);
            });
            auto const node = nodes_.internal_node(lastIndex, std::move(sons));
            stack.push_back(stack_frame {node, lastLevel});
            shrink_stack();
        }

        assert(stack.size() == 1);
        return diagram_t(stack.back().node);
    }

    template<class Data, degree Degree, domain Domain>
    template<std::ranges::input_range R>
    auto diagram_manager<Data, Degree, Domain>::from_vector
        (R&& r) -> diagram_t
    {
        namespace rs = std::ranges;
        return this->from_vector(rs::begin(r), rs::end(r));
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::to_vector
        (diagram_t const& d) const -> std::vector<uint_t>
    {
        auto vs = std::vector<uint_t>();
        vs.reserve(nodes_.domain_product(
            0u, static_cast<level_t>(this->get_var_count())));
        this->to_vector_g(d, std::back_inserter(vs));
        return vs;
    }

    template<class Data, degree Degree, domain Domain>
    template<std::output_iterator<teddy::uint_t> O>
    auto diagram_manager<Data, Degree, Domain>::to_vector_g
        (diagram_t const& d, O out) const -> void
    {
        if (this->get_var_count() == 0)
        {
            assert(d.get_root()->is_terminal());
            *out++ = d.get_root()->get_value();
            return;
        }

        auto vars = std::vector<uint_t>(this->get_var_count());
        auto wasLast = false;
        do
        {
            *out++ = this->evaluate(d, vars);

            auto overflow = true;
            auto level = nodes_.get_leaf_level();
            while (level > 0 && overflow)
            {
                --level;
                auto const index = nodes_.get_index(level);
                ++vars[index];
                overflow = vars[index] == nodes_.get_domain(index);
                if (overflow)
                {
                    vars[index] = 0;
                }

                wasLast = overflow && 0 == level;
            }
        }
        while (not wasLast);
    }

    template<class Data, degree Degree, domain Domain>
    template<class Foo> requires(is_bdd<Degree>)
    auto diagram_manager<Data, Degree, Domain>::from_pla
        ( pla_file const& file
        , fold_type const foldType ) -> second_t<Foo, std::vector<diagram_t>>
    {
        auto const product = [this](auto const& cube)
        {
            auto vs = std::vector<diagram_t>();
            vs.reserve(cube.size());
            for (auto i = 0u; i < cube.size(); ++i)
            {
                if (cube.get(i) == 1)
                {
                    vs.emplace_back(this->variable(i));
                }
                else if (cube.get(i) == 0)
                {
                    vs.emplace_back(this->variable_not(i));
                }
            }
            return this->left_fold<ops::AND>(vs);
        };

        auto const orFold = [this, foldType](auto& ds)
        {
            switch (foldType)
            {
                case fold_type::Left:
                    return this->left_fold<ops::OR>(ds);

                case fold_type::Tree:
                    return this->tree_fold<ops::OR>(ds);

                default:
                    assert(false);
                    return this->constant(0);
            }
        };

        auto const& plaLines      = file.get_lines();
        auto const  lineCount     = file.line_count();
        auto const  functionCount = file.function_count();

        // Create a diagram for each function.
        auto functionDiagrams = std::vector<diagram_t>();
        functionDiagrams.reserve(functionCount);
        for (auto fi = 0u; fi < functionCount; ++fi)
        {
            // First create a diagram for each product.
            auto products = std::vector<diagram_t>();
            products.reserve(lineCount);
            for (auto li = 0u; li < lineCount; ++li)
            {
                // We are doing SOP so we are only interested
                // in functions with value 1.
                if (plaLines[li].fVals.get(fi) == 1)
                {
                    products.emplace_back(product(plaLines[li].cube));
                }
            }

            // In this case we just have a constant function.
            if (products.empty())
            {
                products.emplace_back(this->constant(0));
            }

            // Then merge products using OR.
            functionDiagrams.emplace_back(orFold(products));
        }

        return functionDiagrams;
    }

    template<class Data, degree Degree, domain Domain>
    template<bin_op Op>
    auto diagram_manager<Data, Degree, Domain>::apply
        (diagram_t const& d1, diagram_t const& d2) -> diagram_t
    {
        auto const go = [this](auto&& go_, auto const l, auto const r)
        {
            auto const cached = nodes_.template cache_find<Op>(l, r);
            if (cached)
            {
                return cached;
            }

            auto const lhsVal = node_value(l);
            auto const rhsVal = node_value(r);
            auto const opVal  = Op()(lhsVal, rhsVal);
            auto u = static_cast<node_t*>(nullptr);

            if (opVal != Nondetermined)
            {
                u = nodes_.terminal_node(opVal);
            }
            else
            {
                auto const lhsLevel = nodes_.get_level(l);
                auto const rhsLevel = nodes_.get_level(r);
                auto const topLevel = std::min(lhsLevel, rhsLevel);
                auto const topNode  = topLevel == lhsLevel ? l : r;
                auto const topIndex = topNode->get_index();
                auto sons = nodes_.make_sons(topIndex, [=, &go_](auto const k)
                {
                    auto const fst = lhsLevel == topLevel ? l->get_son(k) : l;
                    auto const snd = rhsLevel == topLevel ? r->get_son(k) : r;
                    return go_(go_, fst, snd);
                });

                u = nodes_.internal_node(topIndex, std::move(sons));
            }

            nodes_.template cache_put<Op>(l, r, u);
            return u;
        };

        auto const r = go(go, d1.get_root(), d2.get_root());
        auto const d = diagram_t(r);
        return d;
    }

    template<class Data, degree Degree, domain Domain>
    template<bin_op Op, std::ranges::input_range R>
    auto diagram_manager<Data, Degree, Domain>::left_fold
        (R const& ds) -> diagram_t
    {
        namespace rs = std::ranges;
        return this->left_fold<Op>(rs::begin(ds), rs::end(ds));
    }

    template<class Data, degree Degree, domain Domain>
    template< bin_op               Op
            , std::input_iterator  I
            , std::sentinel_for<I> S >
    auto diagram_manager<Data, Degree, Domain>::left_fold
        (I first, S const last) -> diagram_t
    {
        static_assert(std::same_as<std::iter_value_t<I>, diagram_t>);

        auto r = std::move(*first);
        ++first;

        while (first != last)
        {
            r = this->apply<Op>(r, *first);
            ++first;
        }

        return r;
    }

    template<class Data, degree Degree, domain Domain>
    template<bin_op Op, std::ranges::random_access_range R>
    auto diagram_manager<Data, Degree, Domain>::tree_fold
        (R& ds) -> diagram_t
    {
        namespace rs = std::ranges;
        return this->tree_fold<Op>(rs::begin(ds), rs::end(ds));
    }

    template<class Data, degree Degree, domain Domain>
    template< bin_op                      Op
            , std::random_access_iterator I
            , std::sentinel_for<I>        S >
    auto diagram_manager<Data, Degree, Domain>::tree_fold
        (I first, S const last) -> diagram_t
    {
        static_assert(std::same_as<std::iter_value_t<I>, diagram_t>);

        auto const count  = std::distance(first, last);
        auto currentCount = count;
        auto const numOfSteps
            = static_cast<std::size_t>(std::ceil(std::log2(count)));

        for (auto step = 0u; step < numOfSteps; ++step)
        {
            auto const justMoveLast = currentCount & 1;
            currentCount = (currentCount / 2) + justMoveLast;
            auto const pairCount    = currentCount - justMoveLast;

            for (auto i = 0u; i < pairCount; ++i)
            {
                *(first + i) = this->apply<Op>( *(first + 2 * i)
                                              , *(first + 2 * i + 1) );
            }

            if (justMoveLast)
            {
                *(first + currentCount - 1)
                    = std::move(*(first + 2 * (currentCount - 1)));
            }
        }

        return diagram_t(std::move(*first));
    }

    template<class Data, degree Degree, domain Domain>
    template<in_var_values Vars>
    auto diagram_manager<Data, Degree, Domain>::evaluate
        (diagram_t const& d, Vars const& vs) const -> uint_t
    {
        auto n = d.get_root();

        while (not n->is_terminal())
        {
            auto const i = n->get_index();
            assert(nodes_.is_valid_var_value(i, vs[i]));
            n = n->get_son(vs[i]);
        }

        return n->get_value();
    }

    template<class Data, degree Degree, domain Domain>
    template<class Foo> requires(is_bdd<Degree>)
    auto diagram_manager<Data, Degree, Domain>::satisfy_count
        (diagram_t& d) -> second_t<Foo, std::size_t>
    {
        return this->satisfy_count(1, d);
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::satisfy_count
        (uint_t const val, diagram_t& d) -> std::size_t
    {
        if constexpr (domains::is_fixed<Domain>()())
        {
            assert(val < Domain()());
        }

        auto constexpr CanUseDataMember
            = std::is_floating_point_v<Data> or std::is_integral_v<Data>;
        using T = std::conditional_t<CanUseDataMember, Data, std::size_t>;

        // A function that returns reference to
        // the data associated with given node.
        auto data = []()
        {
            if constexpr (CanUseDataMember)
            {
                // Simply return reference to the data member.
                return [](auto const n) mutable -> decltype(auto)
                {
                    return (n->data());
                };
            }
            else
            {
                // Return reference to the data that is stored in the map.
                return [map = std::unordered_map<node_t*, T>()]
                    (auto const n) mutable -> T&
                {
                    // If there is no value for given key [] creates new pair
                    // and value-initializes the value (0 for primitive types).
                    return map[n];
                };
            }
        }();

        // Actual satisfy count algorithm.
        nodes_.traverse_post(d.get_root(), [this, val, &data]
            (auto const n) mutable
        {
            if (n->is_terminal())
            {
                data(n) = n->get_value() == val ? 1 : 0;
            }
            else
            {
                data(n) = 0;
                auto const nLevel = nodes_.get_level(n);
                nodes_.for_each_son(n, [=, this, &data](auto const son) mutable
                {
                    auto const sonLevel = nodes_.get_level(son);
                    auto const diff     = nodes_.domain_product( nLevel + 1
                                                               , sonLevel );
                    data(n) += data(son) * static_cast<T>(diff);
                });
            }
        });

        auto const rootAlpha = static_cast<std::size_t>(data(d.get_root()));
        auto const rootLevel = nodes_.get_level(d.get_root());
        return rootAlpha * nodes_.domain_product(0, rootLevel);
    }

    template<class Data, degree Degree, domain Domain>
    template<out_var_values Vars, class Foo> requires(is_bdd<Degree>)
    auto diagram_manager<Data, Degree, Domain>::satisfy_all
        (diagram_t const& d) const -> second_t<Foo, std::vector<Vars>>
    {
        return this->satisfy_all<Vars>(d);
    }

    template<class Data, degree Degree, domain Domain>
    template<out_var_values Vars>
    auto diagram_manager<Data, Degree, Domain>::satisfy_all
        (uint_t const val, diagram_t const& d) const -> std::vector<Vars>
    {
        auto vs = std::vector<Vars>();
        this->satisfy_all_g<Vars>(val, d, std::back_inserter(vs));
        return vs;
    }

    template<class Data, degree Degree, domain Domain>
    template<out_var_values Vars, std::output_iterator<Vars> O>
    auto diagram_manager<Data, Degree, Domain>::satisfy_all_g
        (uint_t const val, diagram_t const& d, O out) const -> void
    {
        if constexpr (domains::is_fixed<Domain>()())
        {
            assert(val < Domain()());
        }

        auto xs = Vars {};
        auto go = [this, &xs, val, out]
            (auto&& go_, auto const l, auto const n) mutable
        {
            auto const nodeValue = node_value(n);
            auto const nodeLevel = nodes_.get_level(n);

            if (n->is_terminal() && val != nodeValue)
            {
                return;
            }
            else if (l == nodes_.get_leaf_level() && val == nodeValue)
            {
                *out++ = xs;
                return;
            }
            else if (nodeLevel > l)
            {
                auto const index  = nodes_.get_index(l);
                auto const domain = nodes_.get_domain(index);
                for (auto iv = 0u; iv < domain; ++iv)
                {
                    xs[index] = iv;
                    go_(go_, l + 1, n);
                }
            }
            else
            {
                auto const index = n->get_index();
                nodes_.for_each_son(n, [=, &xs, iv = uint_t {0}]
                    (auto const son) mutable
                {
                    xs[index] = iv;
                    go_(go_, l + 1, son);
                    ++iv;
                });
            }
        };

        go(go, level_t {0}, d.get_root());
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::cofactor
        (diagram_t const& d, index_t const i, uint_t const v) -> diagram_t
    {
        if (d.get_root()->is_terminal())
        {
            return d;
        }

        auto const root = d.get_root();
        if (root->get_index() == i)
        {
            return diagram_t(root->get_son(v));
        }

        auto memo = std::unordered_map<node_t*, node_t*>();
        auto const go = [this, &memo, i, v](auto&& self, auto const n)
        {
            auto memoIt = memo.find(n);
            if (memoIt != std::end(memo))
            {
                return memoIt->second;
            }

            if (n->is_terminal())
            {
                return n;
            }

            auto sons = n->get_index() == i
                ? nodes_.make_sons(i, [son = n->get_son(v)](auto const)
                {
                    return son;
                })
                : nodes_.make_sons(n->get_index(), [n, &self](auto const k)
                {
                    return self(self, n->get_son(k));
                });

            auto const newN
                = nodes_.internal_node(n->get_index(), std::move(sons));
            memo.emplace(n, newN);
            return newN;
        };

        auto const newRoot = go(go, root);
        return diagram_t(newRoot);
    }

    template<class Data, degree Degree, domain Domain>
    template<uint_to_bool F>
    auto diagram_manager<Data, Degree, Domain>::transform
        (diagram_t const& d, F f) -> diagram_t
    {
        return diagram_t(this->transform_terminal(d.get_root(), f));
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::dependency_set
        (diagram_t const& d) const -> std::vector<index_t>
    {
        auto is = std::vector<index_t>();
        is.reserve(this->get_var_count());
        this->dependency_set_g(d, std::back_inserter(is));
        is.shrink_to_fit();
        return is;
    }

    template<class Data, degree Degree, domain Domain>
    template<std::output_iterator<index_t> O>
    auto diagram_manager<Data, Degree, Domain>::dependency_set_g
        (diagram_t const& d, O out) const -> void
    {
        auto memo = std::vector<bool>(this->get_var_count(), false);
        nodes_.traverse_pre(d.get_root(), [&memo, out](auto const n) mutable
        {
            if (n->is_internal())
            {
                auto const i = n->get_index();
                if (not memo[i])
                {
                    *out++ = i;
                }
                memo[i] = true;
            }
        });
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::reduce
        (diagram_t const& d) -> diagram_t
    {
        auto const newRoot = this->transform_terminal( d.get_root()
                                                     , utils::identity );
        return diagram_t(newRoot);
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::node_count
        () const -> std::size_t
    {
        return nodes_.get_node_count();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::node_count
        (diagram_t const& d) const -> std::size_t
    {
        return nodes_.get_node_count(d.get_root());
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::to_dot_graph
        (std::ostream& ost) const -> void
    {
        nodes_.to_dot_graph(ost);
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::to_dot_graph
        (std::ostream& ost, diagram_t const& d) const -> void
    {
        nodes_.to_dot_graph(ost, d.get_root());
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::get_var_count
        () const -> std::size_t
    {
        return nodes_.get_var_count();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::get_order
        () const -> std::vector<index_t> const&
    {
        return nodes_.get_order();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::get_domains
        () const -> std::vector<uint_t>
    {
        return nodes_.get_domains();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::set_cache_ratio
        (double ratio) -> void
    {
        nodes_.set_cache_ratio(ratio);
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::set_pool_ratio
        (double ratio) -> void
    {
        nodes_.set_pool_ratio(ratio);
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::set_gc_ratio
        (double ratio) -> void
    {
        nodes_.set_gc_ratio(ratio);
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::gc
        () -> void
    {
        nodes_.collect_garbage();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::sift
        () -> void
    {
        nodes_.sift_vars();
    }

    template<class Data, degree Degree, domain Domain>
    template<class F>
    auto diagram_manager<Data, Degree, Domain>::transform_internal
        (node_t* const root, F&& f) -> node_t*
    {
        auto memo = std::unordered_map<node_t*, node_t*>();
        auto const go = [this, &memo](auto&& go_, auto&& f_, auto const n)
        {
            auto const it = memo.find(n);
            if (memo.end() != it)
            {
                return it->second;
            }

            if (n->is_terminal())
            {
                return n;
            }

            auto const u = nodes_.internal_node( n->get_index()
                                               , f_(go_, f_, n) );
            memo.emplace(n, u);
            return u;
        };

        return go(go, f, root);
    }

    template<class Data, degree Degree, domain Domain>
    template<uint_to_uint F>
    auto diagram_manager<Data, Degree, Domain>::transform_terminal
        (node_t* const root, F f) -> node_t*
    {
        auto memo = std::unordered_map<node_t*, node_t*>();
        auto const go = [this, &f, &memo](auto&& go_, auto const n)
        {
            auto const it = memo.find(n);
            if (memo.end() != it)
            {
                return it->second;
            }

            if (n->is_terminal())
            {
                auto const newVal = static_cast<uint_t>(f(n->get_value()));
                return nodes_.terminal_node(newVal);
            }
            else
            {
                auto const i = n->get_index();
                auto const newNode = nodes_.internal_node(i, nodes_.make_sons(i,
                    [&go_, &f, n](auto const k)
                {
                    return go_(go_, n->get_son(k));
                }));
                memo.emplace(n, newNode);
                return newNode;
            }
        };
        return go(go, root);
    }


    template<class Data, degree Degree, domain Domain>
    diagram_manager<Data, Degree, Domain>::diagram_manager
        ( std::size_t const    varCount
        , std::size_t const    nodePoolSize
        , std::size_t const    overflowNodePoolSize
        , std::vector<index_t> order )
        requires(domains::is_fixed<Domain>()()) :
        nodes_ ( varCount
               , nodePoolSize
               , overflowNodePoolSize
               , detail::default_or_fwd(varCount, order) )
    {
    }

    template<class Data, degree Degree, domain Domain>
    diagram_manager<Data, Degree, Domain>::diagram_manager
        ( std::size_t const    varCount
        , std::size_t const    nodePoolSize
        , std::size_t const    overflowNodePoolSize
        , domains::mixed       ds
        , std::vector<index_t> order )
        requires(domains::is_mixed<Domain>()()) :
        nodes_ ( varCount
               , nodePoolSize
               , overflowNodePoolSize
               , detail::default_or_fwd(varCount, order)
               , std::move(ds) )
    {
    }
}

#endif