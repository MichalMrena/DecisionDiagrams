#ifndef LIBTEDDY_DETAILS_DIAGRAM_MANAGER_HPP
#define LIBTEDDY_DETAILS_DIAGRAM_MANAGER_HPP

#include <libteddy/details/diagram.hpp>
#include <libteddy/details/node_manager.hpp>
#include <libteddy/details/operators.hpp>
#include <libteddy/details/pla_file.hpp>
#include <libteddy/details/tools.hpp>

#include <cmath>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <ranges>
#include <tuple>

namespace teddy
{
template<class Vars>
concept in_var_values = requires(Vars values, int32 index) {
                            {
                                values[index]
                            } -> std::convertible_to<int32>;
                        };

template<class Vars>
concept out_var_values = requires(Vars values, int32 index, int32 value) {
                             values[index] = value;
                         };

template<class Node>
concept expression_node = requires(Node node, int32 value) {
                              {
                                  node.is_variable()
                              } -> std::same_as<bool>;
                              {
                                  node.is_constant()
                              } -> std::same_as<bool>;
                              {
                                  node.is_operation()
                              } -> std::same_as<bool>;
                              {
                                  node.get_index()
                              } -> std::same_as<int32>;
                              {
                                  node.get_value()
                              } -> std::same_as<int32>;
                              {
                                  node.evaluate(value, value)
                              } -> std::same_as<int32>;
                              {
                                  node.get_left()
                              } -> std::same_as<Node const&>;
                              {
                                  node.get_right()
                              } -> std::same_as<Node const&>;
                          };

template<class Cache, class Node>
concept cache_handle
    = requires(Cache cache, Node* lhs, Node* rhs, Node* result) {
          cache.put(result, lhs, rhs);
          {
              cache.lookup(lhs, rhs)
          } -> std::same_as<Node*>;
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
     *  \brief Creates diagram representing constant function
     *  \param val Value of the constant function
     *  \return Diagram representing constant function
     */
    auto constant (int32 val) -> diagram_t;

    /**
     *  \brief Creates diagram representing function of single variable
     *  \param index Index of the variable
     *  \return Diagram of a function of single variable
     */
    auto variable (int32 index) -> diagram_t;

    /**
     *  \brief Creates BDD representing function of complemented variable
     *  \param index Index of the variable
     *  \return Diagram of a function of single variable
     */
    template<class Foo = void>
    requires(is_bdd<Degree>)
    auto variable_not (int32 index) -> second_t<Foo, diagram_t>;

    /**
     *  \brief Creates diagram representing function of single variable
     *  \param i Index of the variable
     *  \return Diagram of a function of single variable
     */
    auto operator() (int32 index) -> diagram_t;

    /**
     *  \brief Creates vector of diagrams representing single variables
     *  \tparam T integral type convertible to unsgined int
     *  \param indices initializer list of indices
     *  \return Vector of diagrams
     */
    template<std::convertible_to<int32> T>
    auto variables (std::initializer_list<T> indices) -> std::vector<diagram_t>;

    /**
     *  \brief Creates vector of diagrams representing functions
     *  of single variables
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
     *  \brief Creates vector of diagrams representing single variables
     *  \tparam Is range of integral indices
     *  \param indices range of Ts (e.g. std::vector<int>)
     *  \return Vector of diagrams
     */
    template<std::ranges::input_range Is>
    auto variables (Is const& indices) -> std::vector<diagram_t>;

    /**
     *  \brief Creates diagram from a truth vector of a function
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
     *  std::vector<int> vec {0, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2};
     *  diagram_t f = manager.from_vector(vec);
     *  \endcode
     *
     *  \tparam I Iterator type for the input range.
     *  \tparam S Sentinel type for \p I (end iterator)
     *  \param first iterator to the first element of the truth vector
     *  \param last sentinel for \p first (end iterator)
     *  \return Diagram representing function given by the truth vector
     */
    template<std::input_iterator I, std::sentinel_for<I> S>
    auto from_vector (I first, S last) -> diagram_t;

    /**
     *  \brief Creates diagram from a truth vector of a function
     *
     *  See the other overload for details.
     *
     *  \tparam R Type of the range that contains the truth vector
     *  \param vector Range representing the truth vector
     *  Elements of the range must be convertible to int
     *  \return Diagram representing function given by the truth vector
     */
    template<std::ranges::input_range R>
    auto from_vector (R&& vector) -> diagram_t;

    /**
     *  \brief Creates truth vector from the diagram
     *
     *  Significance of variables is the same as in the \c from_vector
     *  function i.e., variable on the last level of the diagram is
     *  least significant. The following assertion holds:
     *  \code
     *  assert(manager.from_vector(manager.to_vector(d)))
     *  \endcode
     *
     *  \param diagram Diagram
     *  \return Vector of ints representing the truth vector
     */
    auto to_vector (diagram_t const& diagram) const -> std::vector<int32>;

    /**
     *  \brief Creates truth vector from the diagram
     *  \tparam O Output iterator type
     *  \param diagram Diagram
     *  \param out Output iterator that is used to output the truth vector
     */
    template<std::output_iterator<teddy::int32> O>
    auto to_vector_g (diagram_t const& diagram, O out) const -> void;

    /**
     *  \brief Creates BDDs defined by PLA file.
     *
     *  \tparam Foo Dummy template to enable SFINE.
     *  \param file PLA file loaded in the instance of \c pla_file class.
     *  \param foldType fold type used in diagram creation.
     *  \return Vector of diagrams.
     */
    template<class Foo = void>
    requires(is_bdd<Degree>)
    auto from_pla (pla_file const& file, fold_type foldType = fold_type::Tree)
        -> second_t<Foo, std::vector<diagram_t>>;

    /**
     *  \brief Creates diagram from an expression tree (AST).
     *  \tparam Node Node type of the tree.
     *          Must provide API given by the concept.
     *          Required API might change in the future.
     *  \param root Root of the expression tree.
     *  \return Diagram representing function defined by the expression.
     */
    template<expression_node Node>
    auto from_expression_tree (Node const& root) -> diagram_t;

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
     *  \tparam Op Binary operation
     *  \param lhs first diagram
     *  \param rhs second diagram
     *  \return Diagram representing merger of \p lhs and \p rhs
     */
    template<teddy_bin_op Op>
    auto apply (diagram_t const& lhs, diagram_t const& rhs) -> diagram_t;

    /**
     *  \brief Merges diagams in the range using the \c apply function
     *  and binary operation
     *
     *  Uses left fold order of evaluation (sequentially from the left).
     *
     *  \code
     *  // Example:
     *  std::vector<diagram_t> vs = manager.variables({0, 1, 2});
     *  diagram_t product = manager.left_fold<teddy::ops>(vs);
     *  \endcode
     *
     *  \tparam Op Binary operation
     *  \tparam R Range containing diagrams (e.g. std::vector<diagram_t>)
     *  \param diagrams Input range of diagrams to be merged
     *  \return Diagram representing merger of all diagrams from the range
     */
    template<teddy_bin_op Op, std::ranges::input_range R>
    auto left_fold (R const& diagrams) -> diagram_t;

    /**
     *  \brief Merges diagams in the range using the \c apply function
     *  and binary operation
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
     *  \tparam Op Binary operation
     *  \tparam I Range iterator type
     *  \tparam S Sentinel type for \c I (end iterator)
     *  \param first Input iterator to the first diagram
     *  \param last Sentinel for \p first (end iterator)
     *  \return Diagram representing merger of all diagrams from the range
     */
    template<teddy_bin_op Op, std::input_iterator I, std::sentinel_for<I> S>
    auto left_fold (I first, S last) -> diagram_t;

    /**
     *  \brief Merges diagams in the range using the \c apply function
     *  and binary operation
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
     *  \tparam Op Binary operation
     *  \tparam R Range containing diagrams (e.g. std::vector<diagram_t>)
     *  \param diagrams Random access range of diagrams to be merged
     *  (e.g. std::vector)
     *  \return Diagram representing merger of all diagrams from the range
     */
    template<teddy_bin_op Op, std::ranges::random_access_range R>
    auto tree_fold (R& diagrams) -> diagram_t;

    /**
     *  \brief Merges diagams in the range using the \c apply function
     *  and binary operation
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
     *  \tparam Op Binary operation
     *  \tparam I Range iterator type
     *  \tparam S Sentinel type for \c I (end iterator)
     *  \param first Random access iterator to the first diagram
     *  \param last Sentinel for \p first (end iterator)
     *  \return Diagram representing merger of all diagrams from the range
     */
    template<
        teddy_bin_op Op,
        std::random_access_iterator I,
        std::sentinel_for<I> S>
    auto tree_fold (I first, S last) -> diagram_t;

    /**
     *  \brief Evaluates value of the function represented by the diagram
     *
     *  Complexity is \c O(n) where \c n is the number of variables.
     *
     *  \tparam Vars Container type that defines operator[] and returns
     *  value convertible to int
     *  \param diagram Diagram
     *  \param values Container holding values of variables
     *  \return Value of the function represented by \p d for variable
     *  values given in \p vs
     */
    template<in_var_values Vars>
    auto evaluate (diagram_t const& diagram, Vars const& values) const -> int32;

    /**
     *  \brief Calculates number of variable assignments for which
     *  the functions evaluates to certain value
     *
     *  Complexity is \c O(|d|) where \c |d| is the number of nodes.
     *
     *  \param value Value of the function
     *  \param diagram Diagram representing the function
     *  \return Number of different variable assignments for which the
     *  the function represented by \p d evaluates to \p val
     */
    auto satisfy_count (int32 value, diagram_t const& diagram) -> int64;

    /**
     *  \brief Finds variable assignment for which diagram evaluates to \p value
     *
     *  Complexity is \c O(|n|) where \c |n| is the number of nodes.
     *
     *  \param value Value of the function
     *  \param diagram Diagram representing the function
     *  \return Optinal containing variable assignment or std::nullopt
     *          if there is no such an assignment
     */
    template<out_var_values Vars>
    auto satisfy_one (int32 value, diagram_t const& diagram) -> std::optional<Vars>;

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
     *  assigning integers, std::vector is also allowed
     *  \param value Value of the function
     *  \param diagram Diagram representing the function
     *  \return Vector of \p Vars
     */
    template<out_var_values Vars>
    auto satisfy_all (int32 value, diagram_t const& diagram) const
        -> std::vector<Vars>;

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
     *  assigning integers. std::vector is also allowed.
     *  \tparam O Output iterator type.
     *  \param value Value of the function
     *  \param diagram Diagram representing the function
     *  \param out Output iterator that is used to output instances
     *  of \p Vars .
     */
    template<out_var_values Vars, std::output_iterator<Vars> O>
    auto satisfy_all_g (int32 value, diagram_t const& diagram, O out) const
        -> void;

    /**
     *  \brief Calculates cofactor of the functions
     *
     *  Calculates cofactor of the function i.e. fixes value of the \p i th
     *  variable to the value \p val .
     *
     *  \param diagram Diagram representing the function
     *  \param varIndex Index of the variable to be fixed
     *  \param varValue Value to which the \p i th varibale should be fixed
     *  \return Diagram representing cofactor of the function
     */
    auto get_cofactor (diagram_t const& diagram, int32 varIndex, int32 varValue)
        -> diagram_t;

    /**
     *  \brief Transforms values of the function
     *
     *  \code
     *  // Example of the call with 4-valued MDD.
     *  manager.transform(diagram, [](int v)
     *  {
     *      return 3 - v;
     *  });
     *  \endcode
     *
     *  \tparam F Type of the transformation function
     *  \param diagram Diagram representing the function
     *  \param transformer Transformation function that is applied
     *  to values of the function.
     *  \return Diagram representing transformed function
     */
    template<int_to_int F>
    auto transform (diagram_t const& diagram, F transformer) -> diagram_t;

    /**
     *  \brief Enumerates indices of variables that the function depends on
     *  \param diagram Diagram representing the function
     *  \return Vector of indices
     */
    auto get_dependency_set (diagram_t const& diagram) const
        -> std::vector<int32>;

    /**
     *  \brief Enumerates indices of variables that the function depends on
     *  \tparam O Output iterator type.
     *  \param diagram Diagram representing the function.
     *  \param out Output iterator that is used to output indices.
     */
    template<std::output_iterator<int32> O>
    auto get_dependency_set_g (diagram_t const& diagram, O out) const -> void;

    /**
     *  \brief Reduces diagrams to its canonical form
     *
     *  You probably won't need to call this.
     *
     *  \param  diagram Diagram
     *  \return Diagram in a reduced canonical form
     */
    auto reduce (diagram_t const& diagram) -> diagram_t;

    /**
     *  \brief Returns number of nodes that are currently
     *  used by the manager.
     *
     *  This function returns number of nodes that are currently stored
     *  in the unique tables. Total number of allocated nodes might
     *  and probably will be higher. See implementation details on github
     *  for details.
     *
     *  \return Number of nodes
     */
    [[nodiscard]] auto get_node_count () const -> int64;

    /**
     *  \brief Returns number of nodes in the diagram including
     *  terminal nodes
     *  \param diagram Diagram
     *  \return Number of node
     */
    auto get_node_count (diagram_t const& diagram) const -> int64;

    /**
     *  \brief Prints dot representation of the graph
     *
     *  Prints dot representation of the entire multi rooted graph to
     *  the output stream.
     *
     *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
     */
    auto to_dot_graph (std::ostream& out) const -> void;

    /**
     *  \brief Prints dot representation of the diagram
     *
     *  Prints dot representation of the diagram to
     *  the output stream.
     *
     *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
     *  \param diagram Diagram
     */
    auto to_dot_graph (std::ostream& out, diagram_t const& diagram) const
        -> void;

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
    auto force_gc () -> void;

    /**
     *  \brief Runs variable reordering heuristic.
     */
    auto force_reorder () -> void;

    /**
     *  \brief Clears apply cache.
     */
    auto clear_cache () -> void;

    /**
     *  \brief Returns number of variables for this manager
     *  set in the constructor.
     *  \return Number of variables.
     */
    [[nodiscard]] auto get_var_count () const -> int32;

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
    [[nodiscard]] auto get_order () const -> std::vector<int32> const&;

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
    [[nodiscard]] auto get_domains () const -> std::vector<int32>;

    /**
     *  \brief Sets the relative cache size w.r.t the number of nodes
     *
     *  Size of the cache is calculated as:
     *  \code
     *  ratio * uniqueNodeCount
     *  \endcode
     *
     *  \param ratio Number from the interval (0,oo)
     */
    auto set_cache_ratio (double ratio) -> void;

    /**
     *  \brief Sets ratio used to determine new node pool allocation
     *
     *  New pool is allocated if:
     *  \code
     *  garbageCollectedNodes < ratio * initNodeCount
     *  \endcode
     *
     *  \param ratio Number from the interval [0,1]
     */
    auto set_gc_ratio (double ratio) -> void;

    /**
     *  \brief Enables or disables automatic variable reordering
     *
     *  Note that when automatic reordering is enabled the manager
     *  can't guarantee that all diagrams will remain canonical.
     *  To ensure that a diagram \c d is canonical
     *  (e.g. to compare two functions), you need to call \c reduce on them.
     *
     *  \param doReorder Specifies whether to disable (false) or
     *           enable (true) automatic reordering
     */
    auto set_auto_reorder (bool doReorder) -> void;

protected:
    using node_t = typename diagram<Data, Degree>::node_t;

private:
    /**
     *  \class local_cache_handle
     *  \brief Uses local map cache existing just for a single apply call
     *  \tparam Op binary operation
     */
    template<class Map>
    class local_cache_handle
    {
    public:
        local_cache_handle(Map& map);

        template<std::same_as<node<Data, Degree>*>... Nodes>
        auto put (node_t* result, Nodes... input) -> void;

        template<std::same_as<node<Data, Degree>*>... Nodes>
        auto lookup (Nodes... input) const -> node_t*;

    private:
        Map* map_;
    };

    /**
     *  \class global_cache_handle
     *  \brief Uses global operation cache persistent in-between apply calls
     *  \tparam Op binary operation
     */
    template<class Op>
    class global_cache_handle
    {
    public:
        global_cache_handle(node_manager<Data, Degree, Domain>& nodeManager);
        auto put (node_t* result, node_t* lhs, node_t* rhs) -> void;
        auto lookup (node_t* lhs, node_t* rhs) const -> node_t*;

    private:
        node_manager<Data, Degree, Domain>* nodes_;
    };

private:
    template<int_to_int F>
    auto transform_terminal (node_t* root, F transformer) -> node_t*;

    template<class Op, class Cache, class... Nodes>
    auto apply_detail (Cache& cache, Op operation, Nodes... roots)
        -> diagram_t;

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
    diagram_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        std::vector<int32> order
    )
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
    diagram_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        domains::mixed domain,
        std::vector<int32> order
    )
    requires(domains::is_mixed<Domain>()());

public:
    diagram_manager(diagram_manager const&)                         = delete;
    diagram_manager(diagram_manager&&) noexcept                     = default;
    ~diagram_manager()                                              = default;
    auto operator= (diagram_manager const&) -> diagram_manager&     = delete;
    auto operator= (diagram_manager&&) noexcept -> diagram_manager& = default;

protected:
    node_manager<Data, Degree, Domain> nodes_;
};

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::constant(int32 const val)
    -> diagram_t
{
    return diagram_t(nodes_.make_terminal_node(val));
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::variable(int32 const index)
    -> diagram_t
{
    return diagram_t(nodes_.make_internal_node(
        index,
        nodes_.make_sons(
            index,
            [this] (int32 const val)
            {
                return nodes_.make_terminal_node(val);
            }
        )
    ));
}

template<class Data, degree Degree, domain Domain>
template<class Foo>
requires(is_bdd<Degree>)
auto diagram_manager<Data, Degree, Domain>::variable_not(int32 const index)
    -> second_t<Foo, diagram_t>
{
    return diagram_t(nodes_.make_internal_node(
        index,
        nodes_.make_sons(
            index,
            [this] (int32 const val)
            {
                return nodes_.make_terminal_node(1 - val);
            }
        )
    ));
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::operator() (int32 const index)
    -> diagram_t
{
    return this->variable(index);
}

template<class Data, degree Degree, domain Domain>
template<std::ranges::input_range Is>
auto diagram_manager<Data, Degree, Domain>::variables(Is const& indices)
    -> std::vector<diagram_t>
{
    return this->variables(begin(indices), end(indices));
}

template<class Data, degree Degree, domain Domain>
template<std::convertible_to<int32> T>
auto diagram_manager<Data, Degree, Domain>::variables(
    std::initializer_list<T> const indices
) -> std::vector<diagram_t>
{
    return this->variables(begin(indices), end(indices));
}

template<class Data, degree Degree, domain Domain>
template<std::input_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Data, Degree, Domain>::variables(
    I const first,
    S const last
) -> std::vector<diagram_t>
{
    static_assert(std::convertible_to<std::iter_value_t<I>, int32>);
    return utils::fmap(
        first,
        last,
        [this] (auto const index)
        {
            return this->variable(static_cast<int32>(index));
        }
    );
}

template<class Data, degree Degree, domain Domain>
template<std::input_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Data, Degree, Domain>::from_vector(I first, S last)
    -> diagram_t
{
    if (0 == this->get_var_count())
    {
        assert(first != last && std::next(first) == last);
        return diagram_t(nodes_.make_terminal_node(*first));
    }

    auto const lastLevel = static_cast<int32>(this->get_var_count() - 1);
    auto const lastIndex = nodes_.get_index(lastLevel);

    if constexpr (std::random_access_iterator<I>)
    {
        [[maybe_unused]] auto const count
            = nodes_.domain_product(0, lastLevel + 1);
        [[maybe_unused]] auto const dist
            = static_cast<int64>(std::distance(first, last));
        assert(dist > 0 && dist == count);
    }

    using stack_frame = struct
    {
        node_t* node;
        int32 level;
    };

    auto stack              = std::vector<stack_frame>();
    auto const shrink_stack = [this, &stack] ()
    {
        for (;;)
        {
            auto const currentLevel = stack.back().level;
            if (0 == currentLevel)
            {
                break;
            }

            auto const endId = rend(stack);
            auto stackIt     = rbegin(stack);
            auto count       = 0;
            while (stackIt != endId and stackIt->level == currentLevel)
            {
                ++stackIt;
                ++count;
            }
            auto const newIndex  = nodes_.get_index(currentLevel - 1);
            auto const newDomain = nodes_.get_domain(newIndex);

            if (count < newDomain)
            {
                break;
            }

            auto newSons = nodes_.make_sons(
                newIndex,
                [&stack, newDomain] (auto const sonOrder)
                {
                    return stack[as_uindex(ssize(stack) - newDomain + sonOrder)]
                        .node;
                }
            );
            auto const newNode
                = nodes_.make_internal_node(newIndex, std::move(newSons));
            stack.erase(end(stack) - newDomain, end(stack));
            stack.push_back(stack_frame {newNode, currentLevel - 1});
        }
    };

    while (first != last)
    {
        auto sons = nodes_.make_sons(
            lastIndex,
            [this, &first] (auto const)
            {
                return nodes_.make_terminal_node(*first++);
            }
        );
        auto const node = nodes_.make_internal_node(lastIndex, std::move(sons));
        stack.push_back(stack_frame {node, lastLevel});
        shrink_stack();
    }

    assert(stack.size() == 1);
    return diagram_t(stack.back().node);
}

template<class Data, degree Degree, domain Domain>
template<std::ranges::input_range R>
auto diagram_manager<Data, Degree, Domain>::from_vector(R&& vector) -> diagram_t
{
    return this->from_vector(begin(vector), end(vector));
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::to_vector(diagram_t const& diagram
) const -> std::vector<int32>
{
    auto vector = std::vector<int32>();
    vector.reserve(as_usize(nodes_.domain_product(0, this->get_var_count())));
    this->to_vector_g(diagram, std::back_inserter(vector));
    return vector;
}

template<class Data, degree Degree, domain Domain>
template<std::output_iterator<teddy::int32> O>
auto diagram_manager<Data, Degree, Domain>::to_vector_g(
    diagram_t const& diagram,
    O out
) const -> void
{
    if (this->get_var_count() == 0)
    {
        assert(diagram.unsafe_get_root()->is_terminal());
        *out++ = diagram.unsafe_get_root()->get_value();
        return;
    }

    auto vars    = std::vector<int32>(as_usize(this->get_var_count()));
    auto wasLast = false;
    do
    {
        *out++        = this->evaluate(diagram, vars);

        auto overflow = true;
        auto level    = nodes_.get_leaf_level();
        while (level > 0 && overflow)
        {
            --level;
            auto const index = nodes_.get_index(level);
            ++vars[as_uindex(index)];
            overflow = vars[as_uindex(index)] == nodes_.get_domain(index);
            if (overflow)
            {
                vars[as_uindex(index)] = 0;
            }

            wasLast = overflow && 0 == level;
        }
    } while (not wasLast);
}

template<class Data, degree Degree, domain Domain>
template<class Foo>
requires(is_bdd<Degree>)
auto diagram_manager<Data, Degree, Domain>::from_pla(
    pla_file const& file,
    fold_type const foldType
) -> second_t<Foo, std::vector<diagram_t>>
{
    auto const product = [this] (auto const& cube)
    {
        auto variables = std::vector<diagram_t>();
        variables.reserve(cube.size());
        for (auto i = 0; i < cube.size(); ++i)
        {
            if (cube.get(i) == 1)
            {
                variables.emplace_back(this->variable(i));
            }
            else if (cube.get(i) == 0)
            {
                variables.emplace_back(this->variable_not(i));
            }
        }
        return this->left_fold<ops::AND>(variables);
    };

    auto const orFold = [this, foldType] (auto& diagrams)
    {
        switch (foldType)
        {
        case fold_type::Left:
            return this->left_fold<ops::OR>(diagrams);

        case fold_type::Tree:
            return this->tree_fold<ops::OR>(diagrams);

        default:
            assert(false);
            return this->constant(0);
        }
    };

    auto const& plaLines     = file.get_lines();
    auto const lineCount     = file.get_line_count();
    auto const functionCount = file.function_count();

    // Create a diagram for each function.
    auto functionDiagrams = std::vector<diagram_t>();
    functionDiagrams.reserve(functionCount);
    for (auto fi = 0; fi < functionCount; ++fi)
    {
        // First create a diagram for each product.
        auto products = std::vector<diagram_t>();
        // products.reserve(lineCount);
        for (auto li = 0; li < lineCount; ++li)
        {
            // We are doing SOP so we are only interested
            // in functions with value 1.
            if (plaLines[as_usize(li)].fVals_.get(fi) == 1)
            {
                products.emplace_back(product(plaLines[li].cube_));
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
template<expression_node Node>
auto diagram_manager<Data, Degree, Domain>::from_expression_tree(
    Node const& root
) -> diagram_t
{
    using apply_cache_t = std::
        unordered_map<std::tuple<node_t*, node_t*>, node_t*, utils::tuple_hash>;

    auto constexpr apply_op_wrap = [] (auto const& operation)
    {
        return [operation] (auto const lhs, auto const rhs)
        {
            if (lhs == Nondetermined || rhs == Nondetermined)
            {
                return Nondetermined;
            }
            return static_cast<int32>(operation(lhs, rhs));
        };
    };

    auto const step
        = [this, apply_op_wrap] (auto const& self, auto const& exprNode)
    {
        if (exprNode.is_constant())
        {
            return this->constant(exprNode.get_value());
        }

        if (exprNode.is_variable())
        {
            return this->variable(exprNode.get_index());
        }

        assert(exprNode.is_operation());
        auto const lhs       = self(self, exprNode.get_left());
        auto const rhs       = self(self, exprNode.get_right());
        auto const operation = apply_op_wrap(
            [&] (auto const left, auto const right)
            {
                return exprNode.evaluate(left, right);
            }
        );

        auto applyCache  = apply_cache_t();
        auto cacheHandle = local_cache_handle<apply_cache_t>(applyCache);

        return this->apply_detail(
            cacheHandle,
            operation,
            lhs.unsafe_get_root(),
            rhs.unsafe_get_root()
        );
    };
    return step(step, root);
}

template<class Data, degree Degree, domain Domain>
template<teddy_bin_op Op>
auto diagram_manager<Data, Degree, Domain>::apply(
    diagram_t const& lhs,
    diagram_t const& rhs
) -> diagram_t
{
    auto cacheHandle = global_cache_handle<Op>(nodes_);
    return this->apply_detail(
        cacheHandle,
        Op(),
        lhs.unsafe_get_root(),
        rhs.unsafe_get_root()
    );
}

template<class Data, degree Degree, domain Domain>
template<teddy_bin_op Op, std::ranges::input_range R>
auto diagram_manager<Data, Degree, Domain>::left_fold(R const& diagrams)
    -> diagram_t
{
    return this->left_fold<Op>(
        std::ranges::begin(diagrams),
        std::ranges::end(diagrams)
    );
}

template<class Data, degree Degree, domain Domain>
template<teddy_bin_op Op, std::input_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Data, Degree, Domain>::left_fold(I first, S const last)
    -> diagram_t
{
    static_assert(std::same_as<std::iter_value_t<I>, diagram_t>);

    auto result = std::move(*first);
    ++first;

    while (first != last)
    {
        result = this->apply<Op>(result, *first);
        ++first;
    }

    return result;
}

template<class Data, degree Degree, domain Domain>
template<teddy_bin_op Op, std::ranges::random_access_range R>
auto diagram_manager<Data, Degree, Domain>::tree_fold(R& diagrams) -> diagram_t
{
    return this->tree_fold<Op>(
        std::ranges::begin(diagrams),
        std::ranges::end(diagrams)
    );
}

template<class Data, degree Degree, domain Domain>
template<teddy_bin_op Op, std::random_access_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Data, Degree, Domain>::tree_fold(I first, S const last)
    -> diagram_t
{
    static_assert(std::same_as<std::iter_value_t<I>, diagram_t>);

    auto const count      = std::distance(first, last);
    auto currentCount     = count;
    auto const numOfSteps = static_cast<int64>(std::ceil(std::log2(count)));

    for (auto step = 0; step < numOfSteps; ++step)
    {
        auto const justMoveLast = currentCount & 1;
        currentCount            = (currentCount / 2) + justMoveLast;
        auto const pairCount    = currentCount - justMoveLast;

        for (auto i = 0; i < pairCount; ++i)
        {
            *(first + i)
                = this->apply<Op>(*(first + 2 * i), *(first + 2 * i + 1));
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
auto diagram_manager<Data, Degree, Domain>::evaluate(
    diagram_t const& diagram,
    Vars const& values
) const -> int32
{
    auto node = diagram.unsafe_get_root();

    while (not node->is_terminal())
    {
        auto const index = node->get_index();
        assert(nodes_.is_valid_var_value(index, values[as_uindex(index)]));
        node = node->get_son(values[as_uindex(index)]);
    }

    return node->get_value();
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::satisfy_count(
    int32 const value,
    diagram_t const& diagram
) -> int64
{
    if constexpr (domains::is_fixed<Domain>()())
    {
        assert(value < Domain()());
    }

    auto constexpr CanUseDataMember
        = std::is_floating_point_v<Data> or std::is_integral_v<Data>;
    using T = std::conditional_t<CanUseDataMember, Data, int64>;

    // A function that returns reference to
    // the data associated with given node.
    auto data = [] ()
    {
        if constexpr (CanUseDataMember)
        {
            // Simply return reference to the data member.
            return [] (auto const n) mutable -> decltype(auto)
            {
                return (n->data());
            };
        }
        else
        {
            // Return reference to the data that is stored in the map.
            return [map = std::unordered_map<node_t*, T>()] (auto const n
                   ) mutable -> T&
            {
                // If there is no value for given key [] creates new pair
                // and value-initializes the value (0 for primitive types).
                return map[n];
            };
        }
    }();

    // Actual satisfy count algorithm.
    nodes_.traverse_post(
        diagram.unsafe_get_root(),
        [this, value, &data] (node_t* const node) mutable
        {
            if (node->is_terminal())
            {
                data(node) = node->get_value() == value ? 1 : 0;
            }
            else
            {
                data(node)        = 0;
                auto const nLevel = nodes_.get_level(node);
                nodes_.for_each_son(
                    node,
                    [this, &data, node, nLevel] (auto const son) mutable
                    {
                        auto const sonLevel = nodes_.get_level(son);
                        auto const diff
                            = nodes_.domain_product(nLevel + 1, sonLevel);
                        data(node) += data(son) * static_cast<T>(diff);
                    }
                );
            }
        }
    );

    auto const rootAlpha = static_cast<int64>(data(diagram.unsafe_get_root()));
    auto const rootLevel = nodes_.get_level(diagram.unsafe_get_root());
    return rootAlpha * nodes_.domain_product(0, rootLevel);
}

template<class Data, degree Degree, domain Domain>
template<out_var_values Vars>
auto diagram_manager<Data, Degree, Domain>::satisfy_one(
    int32 const value,
    diagram_t const& diagram
) -> std::optional<Vars>
{
    if constexpr (domains::is_fixed<Domain>()())
    {
        assert(value < Domain()());
    }

    auto varValues = [this] ()
    {
        if constexpr (utils::is_std_vector<Vars>)
        {
            return Vars(as_usize(this->get_var_count()));
        }
        else
        {
            return Vars {};
        }
    }();

    auto const step = [this, &varValues, value](
        auto const& self,
        node_t* const node
    )
    {
        if (node->is_terminal())
        {
            return node->get_value() == value;
        }

        auto const nodeDomain = nodes_.get_domain(node->get_index());
        for (auto sonOrder = 0; sonOrder < nodeDomain; ++sonOrder)
        {
            varValues[as_uindex(node->get_index())] = sonOrder;
            if (self(self, node->get_son(sonOrder)))
            {
                return true;
            }
        }

        return false;
    };

    return step(step, diagram.unsafe_get_root())
        ? std::make_optional(varValues)
        : std::nullopt;
}

template<class Data, degree Degree, domain Domain>
template<out_var_values Vars>
auto diagram_manager<Data, Degree, Domain>::satisfy_all(
    int32 const value,
    diagram_t const& diagram
) const -> std::vector<Vars>
{
    auto result = std::vector<Vars>();
    this->satisfy_all_g<Vars>(value, diagram, std::back_inserter(result));
    return result;
}

template<class Data, degree Degree, domain Domain>
template<out_var_values Vars, std::output_iterator<Vars> O>
auto diagram_manager<Data, Degree, Domain>::satisfy_all_g(
    int32 const value,
    diagram_t const& diagram,
    O out
) const -> void
{
    if constexpr (domains::is_fixed<Domain>()())
    {
        assert(value < Domain()());
    }

    auto varValues = [this] ()
    {
        if constexpr (utils::is_std_vector<Vars>)
        {
            return Vars(as_usize(this->get_var_count()));
        }
        else
        {
            return Vars {};
        }
    }();
    auto step
        = [this,
           &varValues,
           value,
           out] (auto& self, auto const currentLevel, node_t* const n) mutable
    {
        if (n->is_terminal() && value != n->get_value())
        {
            return;
        }

        if (currentLevel == nodes_.get_leaf_level() && value == n->get_value())
        {
            *out++ = varValues;
            return;
        }

        if (nodes_.get_level(n) > currentLevel)
        {
            auto const index  = nodes_.get_index(currentLevel);
            auto const domain = nodes_.get_domain(index);
            for (auto iv = 0; iv < domain; ++iv)
            {
                varValues[as_uindex(index)] = iv;
                self(self, currentLevel + 1, n);
            }
        }
        else
        {
            auto const index = n->get_index();
            nodes_.for_each_son(
                n,
                [=, &varValues, sonOrder = int32 {0}] (node_t* const son
                ) mutable
                {
                    varValues[as_uindex(index)] = sonOrder;
                    self(self, currentLevel + 1, son);
                    ++sonOrder;
                }
            );
        }
    };

    step(step, int32 {0}, diagram.unsafe_get_root());
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::get_cofactor(
    diagram_t const& diagram,
    int32 const varIndex,
    int32 const varValue
) -> diagram_t
{
    if (diagram.unsafe_get_root()->is_terminal())
    {
        return diagram;
    }

    auto const root = diagram.unsafe_get_root();
    if (root->get_index() == varIndex)
    {
        return diagram_t(root->get_son(varValue));
    }

    auto memo = std::unordered_map<node_t*, node_t*>();
    auto const step =
        [this, &memo, varIndex, varValue] (auto const& self, node_t* const node)
    {
        auto const memoIt = memo.find(node);
        if (memoIt != std::end(memo))
        {
            return memoIt->second;
        }

        if (node->is_terminal())
        {
            return node;
        }

        auto sons = node->get_index() == varIndex
                      ? nodes_.make_sons(
                          varIndex,
                          [son = node->get_son(varValue)] (auto const)
                          {
                              return son;
                          }
                      )
                      : nodes_.make_sons(
                          node->get_index(),
                          [node, &self] (auto const sonOrder)
                          {
                              return self(self, node->get_son(sonOrder));
                          }
                      );

        auto const newN
            = nodes_.make_internal_node(node->get_index(), std::move(sons));
        memo.emplace(node, newN);
        return newN;
    };

    auto const newRoot = step(step, root);
    return diagram_t(newRoot);
}

template<class Data, degree Degree, domain Domain>
template<int_to_int F>
auto diagram_manager<Data, Degree, Domain>::transform(
    diagram_t const& diagram,
    F transformer
) -> diagram_t
{
    auto const newRoot
        = this->transform_terminal(diagram.unsafe_get_root(), transformer);
    nodes_.run_deferred();
    return diagram_t(newRoot);
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::get_dependency_set(
    diagram_t const& diagram
) const -> std::vector<int32>
{
    auto indices = std::vector<int32>();
    indices.reserve(this->get_var_count());
    this->get_dependency_set_g(diagram, std::back_inserter(indices));
    indices.shrink_to_fit();
    return indices;
}

template<class Data, degree Degree, domain Domain>
template<std::output_iterator<int32> O>
auto diagram_manager<Data, Degree, Domain>::get_dependency_set_g(
    diagram_t const& diagram,
    O out
) const -> void
{
    auto memo = std::vector<bool>(this->get_var_count(), false);
    nodes_.traverse_pre(
        diagram.unsafe_get_root(),
        [&memo, out] (auto const n) mutable
        {
            if (n->is_internal())
            {
                auto const index = n->get_index();
                if (not memo[index])
                {
                    *out++ = index;
                }
                memo[index] = true;
            }
        }
    );
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::reduce(diagram_t const& diagram)
    -> diagram_t
{
    auto const newRoot
        = this->transform_terminal(diagram.unsafe_get_root(), utils::identity);
    return diagram_t(newRoot);
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::get_node_count() const -> int64
{
    return nodes_.get_node_count();
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::get_node_count(
    diagram_t const& diagram
) const -> int64
{
    return nodes_.get_node_count(diagram.unsafe_get_root());
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::to_dot_graph(std::ostream& out
) const -> void
{
    nodes_.to_dot_graph(out);
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::to_dot_graph(
    std::ostream& out,
    diagram_t const& diagram
) const -> void
{
    nodes_.to_dot_graph(out, diagram.unsafe_get_root());
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::get_var_count() const -> int32
{
    return nodes_.get_var_count();
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::get_order() const
    -> std::vector<int32> const&
{
    return nodes_.get_order();
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::get_domains() const
    -> std::vector<int32>
{
    return nodes_.get_domains();
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::set_cache_ratio(double ratio)
    -> void
{
    nodes_.set_cache_ratio(ratio);
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::set_gc_ratio(double ratio) -> void
{
    nodes_.set_gc_ratio(ratio);
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::set_auto_reorder(
    bool const doReorder
) -> void
{
    nodes_.set_auto_reorder(doReorder);
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::force_gc() -> void
{
    nodes_.force_gc();
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::force_reorder() -> void
{
    nodes_.sift_variables();
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::clear_cache() -> void
{
    nodes_.cache_clear();
}

template<class Data, degree Degree, domain Domain>
template<int_to_int F>
auto diagram_manager<Data, Degree, Domain>::transform_terminal(
    node_t* const root,
    F transformer
) -> node_t*
{
    auto memo = std::unordered_map<node_t*, node_t*>();
    auto const step
        = [this, transformer, &memo] (auto const& self, auto const n)
    {
        auto const memoIt = memo.find(n);
        if (memo.end() != memoIt)
        {
            return memoIt->second;
        }

        if (n->is_terminal())
        {
            auto const newVal = static_cast<int32>(transformer(n->get_value()));
            return nodes_.make_terminal_node(newVal);
        }

        auto const index   = n->get_index();
        auto const newNode = nodes_.make_internal_node(
            index,
            nodes_.make_sons(
                index,
                [&self, n] (auto const sonOrder)
                {
                    return self(self, n->get_son(sonOrder));
                }
            )
        );

        memo.emplace(n, newNode);
        return newNode;
    };
    return step(step, root);
}

template<class Data, degree Degree, domain Domain>
template<class Op, class Cache, class... Nodes>
auto diagram_manager<Data, Degree, Domain>::apply_detail(
    Cache& cache,
    Op operation,
    Nodes... roots
) -> diagram_t
{
    auto const get_next =
        [this] (int32 const minLevel, int32 const sonOrder, node_t* const node)
    {
        return nodes_.get_level(node) == minLevel ? node->get_son(sonOrder)
                                                  : node;
    };

    auto const get_node_value = [] (node_t* const node)
    {
        return node->is_terminal() ? node->get_value() : Nondetermined;
    };

    auto const step = [this, &cache, operation, get_next, get_node_value] (
                          auto const& self,
                          auto const... nodes
                      )
    {
        auto const cached = cache.lookup(nodes...);
        if (cached)
        {
            return cached;
        }

        auto const opVal = operation(get_node_value(nodes)...);
        auto result      = static_cast<node_t*>(nullptr);

        if (opVal != Nondetermined)
        {
            result = nodes_.make_terminal_node(opVal);
        }
        else
        {
            auto const minLevel = std::min({nodes_.get_level(nodes)...});
            auto const topIndex = nodes_.get_index(minLevel);
            auto sons           = nodes_.make_sons(
                topIndex,
                [=] (int32 const sonOrder)
                {
                    return self(self, get_next(minLevel, sonOrder, nodes)...);
                }
            );

            result = nodes_.make_internal_node(topIndex, std::move(sons));
        }

        cache.put(result, nodes...);
        return result;
    };

    auto const newRoot    = step(step, roots...);
    auto const newDiagram = diagram_t(newRoot);
    nodes_.run_deferred();
    return newDiagram;
}

namespace detail
{
inline auto default_or_fwd (int64 const n, std::vector<int32>& indices)
{
    return indices.empty() ? utils::fill_vector(n, utils::identity)
                           : std::vector<int32>(std::move(indices));
}
} // namespace detail

template<class Data, degree Degree, domain Domain>
diagram_manager<Data, Degree, Domain>::diagram_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order
)
requires(domains::is_fixed<Domain>()())
    :
    nodes_(
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        detail::default_or_fwd(varCount, order)
    )
{
}

template<class Data, degree Degree, domain Domain>
diagram_manager<Data, Degree, Domain>::diagram_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    domains::mixed domain,
    std::vector<int32> order
)
requires(domains::is_mixed<Domain>()())
    :
    nodes_(
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        detail::default_or_fwd(varCount, order),
        std::move(domain)
    )
{
}

// diagram_manager::local_cache_handle definitions:

template<class Data, degree Degree, domain Domain>
template<class Map>
diagram_manager<Data, Degree, Domain>::local_cache_handle<
    Map>::local_cache_handle(Map& map) :
    map_(&map)
{
}

template<class Data, degree Degree, domain Domain>
template<class Map>
template<std::same_as<node<Data, Degree>*>... Nodes>
auto diagram_manager<Data, Degree, Domain>::local_cache_handle<Map>::put(
    node_t* const result,
    Nodes... input
) -> void
{
    map_->emplace(
        std::piecewise_construct,
        std::make_tuple(input...),
        std::make_tuple(result)
    );
}

template<class Data, degree Degree, domain Domain>
template<class Map>
template<std::same_as<node<Data, Degree>*>... Nodes>
auto diagram_manager<Data, Degree, Domain>::local_cache_handle<Map>::lookup(
    Nodes... input
) const -> node_t*
{
    auto const cacheIt = map_->find(std::make_tuple(input...));
    return cacheIt != end(*map_) ? cacheIt->second : nullptr;
}

// diagram_manager::global_cache_handle definitions:

template<class Data, degree Degree, domain Domain>
template<class Op>
diagram_manager<Data, Degree, Domain>::global_cache_handle<
    Op>::global_cache_handle(node_manager<Data, Degree, Domain>& nodeManager) :
    nodes_(&nodeManager)
{
}

template<class Data, degree Degree, domain Domain>
template<class Op>
auto diagram_manager<Data, Degree, Domain>::global_cache_handle<Op>::put(
    node_t* const result,
    node_t* const lhs,
    node_t* const rhs
) -> void
{
    nodes_->template cache_put<Op>(result, lhs, rhs);
}

template<class Data, degree Degree, domain Domain>
template<class Op>
auto diagram_manager<Data, Degree, Domain>::global_cache_handle<Op>::lookup(
    node_t* const lhs,
    node_t* const rhs
) const -> node_t*
{
    return nodes_->template cache_find<Op>(lhs, rhs);
}
} // namespace teddy

#endif