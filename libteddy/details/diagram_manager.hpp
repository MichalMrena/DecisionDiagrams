#ifndef LIBTEDDY_DETAILS_DIAGRAM_MANAGER_HPP
#define LIBTEDDY_DETAILS_DIAGRAM_MANAGER_HPP

#include <libteddy/details/diagram.hpp>
#include <libteddy/details/node_manager.hpp>
#include <libteddy/details/operators.hpp>
#include <libteddy/details/pla_file.hpp>
#include <libteddy/details/stats.hpp>
#include <libteddy/details/tools.hpp>
#include <libteddy/details/types.hpp>

#include <cmath>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <ranges>
#include <vector>

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

template<class Degree>
concept is_bdd = std::same_as<degrees::fixed<2>, Degree>;

template<class F>
concept int_to_int = requires(F function, int32 value) {
                         {
                             function(value)
                         } -> std::convertible_to<int32>;
                     };

enum class fold_type
{
    Left,
    Tree
};

struct var_cofactor
{
    int32 index_;
    int32 value_;
};

/**
 *  \class diagram_manager
 *  \brief Base class for all diagram managers that generically
 *  implements all of the algorithms.
 */
template<class Data, class Degree, class Domain>
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
    auto variable_not (int32 index) -> utils::second_t<Foo, diagram_t>;

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
        -> utils::second_t<Foo, std::vector<diagram_t>>;

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
     *  | AND               | Logical and. ^ $                      |
     *  | OR                | Logical or. ^ $                       |
     *  | XOR               | Logical xor. ^ $                      |
     *  | NAND              | Logical nand. ^ $                     |
     *  | NOR               | Logical nor. ^ $                      |
     *  | XNOR              | Logical xnor. ^ $                     |
     *  | IMPLIES           | Logical implication. ^ $              |
     *  | EQUAL_TO          | Equal to relation. ^                  |
     *  | NOT_EQUAL_TO      | Not equal to relation. ^              |
     *  | LESS              | Less than relation. ^                 |
     *  | LESS_EQUAL        | Less than or equal relation. ^        |
     *  | GREATER           | Greater than relation. ^              |
     *  | GREATER_EQUAL     | Greater than or equal relation. ^     |
     *  | MIN               | Minimum of two values.                |
     *  | MAX               | Maximum of two values.                |
     *  | PLUS<M>           | Modular addition: (a + b) mod M.      |
     *  | MULTIPLIES<M>     | Modular multiplication: (a * b) mod M.|
     *  +-------------------+---------------------------------------+
     *  ^ 0 is false and 1 is true
     *  $ assumes that arguments are 0 or 1
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
     *  \brief TODO
     */
    template<teddy_bin_op Op, class... Diagram>
    auto apply_n (Diagram const&... diagrams) -> diagram_t;

    // TODO apply_own

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
    auto satisfy_one (int32 value, diagram_t const& diagram)
        -> std::optional<Vars>;

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
    auto get_cofactor (
        diagram_t const& diagram,
        int32 varIndex,
        int32 varValue
    ) -> diagram_t;

    auto get_cofactor (
        diagram_t const& diagram,
        std::vector<var_cofactor> const& vars
    ) -> diagram_t;

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
     *  \brief Negates Boolean function
     *  \param diagram Diagram representing the function
     *  \param transformer Transformation function that is applied
     *  to values of the function.
     *  \return Diagram representing transformed function
     */
    template<class Foo = void>
    requires(is_bdd<Degree>)
    auto negate (diagram_t const& diagram) -> utils::second_t<Foo, diagram_t>;

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
    using node_t        = typename diagram<Data, Degree>::node_t;
    using son_container = typename node_t::son_container;

private:
    template<int32 Size>
    struct node_pack
    {
        node_t* key_[static_cast<std::size_t>(Size)] {nullptr};
        node_t* result_ {nullptr};
    };

    // TODO tmp
    template<int32 Size, class... Node>
    static auto pack_equals (node_pack<Size> const& pack, Node... nodes)
    {
        node_t* nodeArray[] {nodes...};
        for (int32 k = 0; k < Size; ++k)
        {
            if (pack.key_[k] != nodeArray[k])
            {
                return false;
            }
        }
        return true;
    }

private:
    // TODO namiesto mema by sa dali pouzit data,
    // idealne keby data bolo iba pole bytov a dalo by sa tam ulozit cokolvek

    auto variable_impl (int32 index) -> node_t*;

    template<class Op>
    auto apply_impl (Op operation, node_t* lhs, node_t* rhs) -> node_t*;

    template<class Op, class... Node>
    auto apply_n_impl (
        std::vector<node_pack<sizeof...(Node)>>& cache,
        Op operation,
        Node... nodes
    ) -> node_t*;

    template<class Vars>
    auto satisfy_one_impl (int32 value, Vars& vars, node_t* node) -> bool;

    template<class Vars, class OutputIt>
    auto satisfy_all_impl (
        int32 value,
        Vars& vars,
        OutputIt out,
        node_t* node,
        int32 level
    ) const -> void;

    auto get_cofactor_impl (
        std::unordered_map<node_t*, node_t*>& memo,
        int32 varIndex,
        int32 varValue,
        node_t* node
    ) -> node_t*;

    auto get_cofactor_impl (
        std::unordered_map<node_t*, node_t*>& memo,
        std::vector<var_cofactor> const& vars,
        node_t* node,
        int32 toCofactor
    ) -> node_t*;

    template<class F>
    auto transform_impl (
        std::unordered_map<node_t*, node_t*>& memo,
        F transformer,
        node_t* node
    ) -> node_t*;

    template<class ExprNode>
    auto from_expression_tree_impl (
        std::vector<node_pack<2>>& cache,
        ExprNode const& exprNode
    ) -> node_t*;

protected:
    /**
     *  \brief Initializes diagram manager.
     *
     *  This overload is for managers that have fixed domains
     *  (known at copile time).
     *
     *  \param varCount Number of variables.
     *  \param nodePoolSize Number of nodes that is pre-allocated.
     *  \param extraNodePoolSize Size of the additional node pools.
     *  \param order Order of variables.
     */
    diagram_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 extraNodePoolSize,
        std::vector<int32> order
    )
    requires(domains::is_fixed<Domain>::value);

    /**
     *  \brief Initializes diagram manager.
     *
     *  This overload is for managers that have mixed domains
     *  specified by the \p ds paramter.
     *
     *  \param varCount Number of variables.
     *  \param nodePoolSize Number of nodes that is pre-allocated.
     *  \param extraNodePoolSize Size of the additional node pools.
     *  \param ds Domains of varibales.
     *  \param order Order of variables.
     */
    diagram_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 extraNodePoolSize,
        domains::mixed domain,
        std::vector<int32> order
    )
    requires(domains::is_mixed<Domain>::value);

public:
    diagram_manager(diagram_manager&&) noexcept                     = default;
    ~diagram_manager()                                              = default;
    auto operator= (diagram_manager&&) noexcept -> diagram_manager& = default;
    diagram_manager(diagram_manager const&)                         = delete;
    auto operator= (diagram_manager const&) -> diagram_manager&     = delete;

protected:
    node_manager<Data, Degree, Domain> nodes_;
};

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::constant(int32 const val)
    -> diagram_t
{
    return diagram_t(nodes_.make_terminal_node(val));
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::variable(int32 const index)
    -> diagram_t
{
    return diagram_t(this->variable_impl(index));
}

template<class Data, class Degree, class Domain>
template<class Foo>
requires(is_bdd<Degree>)
auto diagram_manager<Data, Degree, Domain>::variable_not(int32 const index)
    -> utils::second_t<Foo, diagram_t>
{
    son_container sons = nodes_.make_son_container(2);
    sons[0]            = nodes_.make_terminal_node(1);
    sons[1]            = nodes_.make_terminal_node(0);
    return diagram_t(nodes_.make_internal_node(index, sons));
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::operator() (int32 const index)
    -> diagram_t
{
    return this->variable(index);
}

template<class Data, class Degree, class Domain>
template<std::ranges::input_range Is>
auto diagram_manager<Data, Degree, Domain>::variables(Is const& indices)
    -> std::vector<diagram_t>
{
    return this->variables(begin(indices), end(indices));
}

template<class Data, class Degree, class Domain>
template<std::convertible_to<int32> T>
auto diagram_manager<Data, Degree, Domain>::variables(
    std::initializer_list<T> const indices
) -> std::vector<diagram_t>
{
    return this->variables(begin(indices), end(indices));
}

template<class Data, class Degree, class Domain>
template<std::input_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Data, Degree, Domain>::variables(I first, S const last)
    -> std::vector<diagram_t>
{
    static_assert(std::convertible_to<std::iter_value_t<I>, int32>);
    std::vector<diagram_t> result;
    while (first != last)
    {
        result.push_back(this->variable(static_cast<int32>(*first)));
        ++first;
    }
    return result;
}

template<class Data, class Degree, class Domain>
template<std::input_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Data, Degree, Domain>::from_vector(I first, S last)
    -> diagram_t
{
    if (0 == this->get_var_count())
    {
        assert(first != last && ++I(first) == last);
        return diagram_t(nodes_.make_terminal_node(*first));
    }

    int32 const lastLevel = this->get_var_count() - 1;
    int32 const lastIndex = nodes_.get_index(lastLevel);

    if constexpr (std::random_access_iterator<I>)
    {
        [[maybe_unused]] int64 const count
            = nodes_.domain_product(0, lastLevel + 1);
        [[maybe_unused]] auto const dist = static_cast<int64>(last - first);
        assert(dist > 0 && dist == count);
    }

    using stack_frame = struct
    {
        node_t* node;
        int32 level;
    };

    std::vector<stack_frame> stack;
    auto const shrink_stack = [this, &stack] ()
    {
        for (;;)
        {
            int32 const currentLevel = stack.back().level;
            if (0 == currentLevel)
            {
                break;
            }

            auto const endId = rend(stack);
            auto stackIt     = rbegin(stack);
            int64 count      = 0;
            while (stackIt != endId and stackIt->level == currentLevel)
            {
                ++stackIt;
                ++count;
            }
            int32 const newIndex  = nodes_.get_index(currentLevel - 1);
            int32 const newDomain = nodes_.get_domain(newIndex);

            if (count < newDomain)
            {
                break;
            }

            son_container newSons = nodes_.make_son_container(newDomain);
            for (int32 k = 0; k < newDomain; ++k)
            {
                newSons[k]
                    = stack[as_uindex(ssize(stack) - newDomain + k)].node;
            }
            node_t* const newNode
                = nodes_.make_internal_node(newIndex, newSons);
            stack.erase(end(stack) - newDomain, end(stack));
            stack.push_back(stack_frame {newNode, currentLevel - 1});
        }
    };

    while (first != last)
    {
        int32 const lastDomain = nodes_.get_domain(lastIndex);
        son_container sons     = nodes_.make_son_container(lastDomain);
        for (int32 k = 0; k < lastDomain; ++k)
        {
            sons[k] = nodes_.make_terminal_node(*first++);
        }
        node_t* const node = nodes_.make_internal_node(lastIndex, sons);
        stack.push_back(stack_frame {node, lastLevel});
        shrink_stack();
    }

    assert(ssize(stack) == 1);
    return diagram_t(stack.back().node);
}

template<class Data, class Degree, class Domain>
template<std::ranges::input_range R>
auto diagram_manager<Data, Degree, Domain>::from_vector(R&& vector) -> diagram_t
{
    return this->from_vector(begin(vector), end(vector));
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::to_vector(diagram_t const& diagram
) const -> std::vector<int32>
{
    std::vector<int32> vector;
    vector.reserve(as_usize(nodes_.domain_product(0, this->get_var_count())));
    this->to_vector_g(diagram, std::back_inserter(vector));
    return vector;
}

template<class Data, class Degree, class Domain>
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

    std::vector<int32> vars(as_usize(this->get_var_count()));
    bool wasLast = false;
    do
    {
        *out++        = this->evaluate(diagram, vars);
        bool overflow = true;
        int32 level   = nodes_.get_leaf_level();
        while (level > 0 && overflow)
        {
            --level;
            int32 const index = nodes_.get_index(level);
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

template<class Data, class Degree, class Domain>
template<class Foo>
requires(is_bdd<Degree>)
auto diagram_manager<Data, Degree, Domain>::from_pla(
    pla_file const& file,
    fold_type const foldType
) -> utils::second_t<Foo, std::vector<diagram_t>>
{
    auto const product = [this] (auto const& cube)
    {
        std::vector<diagram_t> variables;
        variables.reserve(cube.size());
        for (int32 i = 0; i < cube.size(); ++i)
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

    auto const& plaLines      = file.get_lines();
    int64 const lineCount     = file.get_line_count();
    int64 const functionCount = file.get_function_count();

    // Create a diagram for each function.
    std::vector<diagram_t> functionDiagrams;
    functionDiagrams.reserve(functionCount);
    for (int32 fi = 0; fi < functionCount; ++fi)
    {
        // First create a diagram for each product.
        std::vector<diagram_t> products;
        // products.reserve(lineCount);
        for (int32 li = 0; li < lineCount; ++li)
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

template<class Data, class Degree, class Domain>
template<expression_node Node>
auto diagram_manager<Data, Degree, Domain>::from_expression_tree(
    Node const& root
) -> diagram_t
{
    int64 constexpr CacheCapacity = 100'000;
    std::vector<node_pack<2>> cache(as_usize(CacheCapacity));
    node_t* newRoot = this->from_expression_tree_impl(cache, root);
    nodes_.run_deferred();
    return diagram_t(newRoot);
}

template<class Data, class Degree, class Domain>
template<class ExprNode>
auto diagram_manager<Data, Degree, Domain>::from_expression_tree_impl(
    std::vector<node_pack<2>>& cache,
    ExprNode const& exprNode
) -> node_t*
{
    if (exprNode.is_constant())
    {
        return nodes_.make_terminal_node(exprNode.get_value());
    }

    if (exprNode.is_variable())
    {
        return this->variable_impl(exprNode.get_index());
    }

    assert(exprNode.is_operation());

    node_t* const left
        = this->from_expression_tree_impl(cache, exprNode.get_left());
    node_t* const right
        = this->from_expression_tree_impl(cache, exprNode.get_right());
    auto const operation = [&exprNode] (auto const lhs, auto const rhs)
    {
        if (lhs == Nondetermined || rhs == Nondetermined)
        {
            return Nondetermined;
        }
        return static_cast<int32>(exprNode.evaluate(lhs, rhs));
    };

    node_t* const newRoot = this->apply_n_impl(cache, operation, left, right);
    auto const cacheCapacity = cache.size();
    cache.clear();
    cache.resize(cacheCapacity);
    return newRoot;
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::variable_impl(int32 const index)
    -> node_t*
{
    int32 const varDomain = nodes_.get_domain(index);
    son_container sons    = nodes_.make_son_container(varDomain);
    for (int32 val = 0; val < varDomain; ++val)
    {
        sons[val] = nodes_.make_terminal_node(val);
    }
    return nodes_.make_internal_node(index, sons);
}

template<class Data, class Degree, class Domain>
template<teddy_bin_op Op>
auto diagram_manager<Data, Degree, Domain>::apply(
    diagram_t const& lhs,
    diagram_t const& rhs
) -> diagram_t
{
    /*
     * Use bounded MAX if the max value is known.
     * This should perform better since it can short-circuit.
     */
    using OpType = utils::type_if<
        utils::is_same<Op, ops::MAX>::value && domains::is_fixed<Domain>::value,
        ops::MAXB<Domain::value>,
        Op>::type;

    node_t* const newRoot = this->apply_impl(
        OpType(),
        lhs.unsafe_get_root(),
        rhs.unsafe_get_root()
    );
    nodes_.run_deferred();
    return diagram_t(newRoot);
}

template<class Data, class Degree, class Domain>
template<class Op>
auto diagram_manager<Data, Degree, Domain>::apply_impl(
    Op operation,
    node_t* const lhs,
    node_t* const rhs
) -> node_t*
{
#ifdef LIBTEDDY_COLLECT_STATS
    ++stats::get_stats().applyStepCalls_;
#endif

    node_t* const cached = nodes_.template cache_find<Op>(lhs, rhs);
    if (cached)
    {
        return cached;
    }

    int32 const lhsVal = lhs->is_terminal() ? lhs->get_value() : Nondetermined;
    int32 const rhsVal = rhs->is_terminal() ? rhs->get_value() : Nondetermined;
    int32 const opVal  = operation(lhsVal, rhsVal);

    if (opVal != Nondetermined)
    {
        node_t* const result = nodes_.make_terminal_node(opVal);
        nodes_.template cache_put<Op>(result, lhs, rhs);
        return result;
    }

    int32 const lhsLevel = nodes_.get_level(lhs);
    int32 const rhsLevel = nodes_.get_level(rhs);
    int32 const topLevel = utils::min(lhsLevel, rhsLevel);
    int32 const topIndex = nodes_.get_index(topLevel);
    int32 const domain   = nodes_.get_domain(topIndex);
    son_container sons   = nodes_.make_son_container(domain);
    for (int32 k = 0; k < domain; ++k)
    {
        sons[k] = this->apply_impl(
            operation,
            lhsLevel == topLevel ? lhs->get_son(k) : lhs,
            rhsLevel == topLevel ? rhs->get_son(k) : rhs
        );
    }

    node_t* const result = nodes_.make_internal_node(topIndex, sons);
    nodes_.template cache_put<Op>(result, lhs, rhs);
    return result;
}

template<class Data, class Degree, class Domain>
template<teddy_bin_op Op, class... Diagram>
auto diagram_manager<Data, Degree, Domain>::apply_n(Diagram const&... diagram)
    -> diagram_t
{
    /*
     * Use bounded MAX if the max value is known.
     * This should perform better since it can short-circuit.
     */
    using OpType = utils::type_if<
        utils::is_same<Op, ops::MAX>::value && domains::is_fixed<Domain>::value,
        ops::MAXB<Domain::value>,
        Op>::type;

    // TODO capacity
    std::vector<node_pack<sizeof...(Diagram)>> cache(100'000);
    node_t* const newRoot
        = this->apply_n_impl(cache, OpType(), diagram.unsafe_get_root()...);
    nodes_.run_deferred();
    return diagram_t(newRoot);
}

template<class Data, class Degree, class Domain>
template<class Op, class... Node>
auto diagram_manager<Data, Degree, Domain>::apply_n_impl(
    std::vector<node_pack<sizeof...(Node)>>& cache,
    Op operation,
    Node... nodes
) -> node_t*
{
    std::size_t const hash                = utils::pack_hash(nodes...);
    std::size_t const cacheIndex          = hash % cache.size();
    node_pack<sizeof...(Node)>& cachePack = cache[cacheIndex];
    if (pack_equals(cachePack, nodes...))
    {
        return cachePack.result_;
    }

    int32 const opVal = operation(
        (nodes->is_terminal() ? nodes->get_value() : Nondetermined)...
    );
    node_t* result = nullptr;

    if (opVal != Nondetermined)
    {
        result = nodes_.make_terminal_node(opVal);
    }
    else
    {
        int32 const minLevel = utils::pack_min(nodes_.get_level(nodes)...);
        int32 const topIndex = nodes_.get_index(minLevel);
        int32 const domain   = nodes_.get_domain(topIndex);
        son_container sons   = nodes_.make_son_container(domain);
        for (int32 k = 0; k < domain; ++k)
        {
            sons[k] = this->apply_n_impl(
                cache,
                operation,
                (nodes_.get_level(nodes) == minLevel ? nodes->get_son(k) : nodes
                )...
            );
        }
        result = nodes_.make_internal_node(topIndex, sons);
    }

    cachePack = {{nodes...}, result};
    return result;
}

template<class Data, class Degree, class Domain>
template<teddy_bin_op Op, std::ranges::input_range R>
auto diagram_manager<Data, Degree, Domain>::left_fold(R const& diagrams)
    -> diagram_t
{
    return this->left_fold<Op>(begin(diagrams), end(diagrams));
}

template<class Data, class Degree, class Domain>
template<teddy_bin_op Op, std::input_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Data, Degree, Domain>::left_fold(I first, S const last)
    -> diagram_t
{
    static_assert(std::same_as<std::iter_value_t<I>, diagram_t>);

    diagram_t result = *first;
    ++first;

    while (first != last)
    {
        result = this->apply<Op>(result, *first);
        ++first;
    }

    return result;
}

template<class Data, class Degree, class Domain>
template<teddy_bin_op Op, std::ranges::random_access_range R>
auto diagram_manager<Data, Degree, Domain>::tree_fold(R& diagrams) -> diagram_t
{
    return this->tree_fold<Op>(begin(diagrams), end(diagrams));
}

template<class Data, class Degree, class Domain>
template<teddy_bin_op Op, std::random_access_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Data, Degree, Domain>::tree_fold(I first, S const last)
    -> diagram_t
{
    static_assert(std::same_as<std::iter_value_t<I>, diagram_t>);

    int64 const count     = std::distance(first, last);
    int64 currentCount    = count;
    auto const numOfSteps = static_cast<int64>(std::ceil(std::log2(count)));

    for (auto step = 0; step < numOfSteps; ++step)
    {
        auto const justMoveLast = static_cast<bool>(currentCount & 1);
        currentCount            = (currentCount / 2) + justMoveLast;
        int64 const pairCount   = currentCount - justMoveLast;

        for (int64 i = 0; i < pairCount; ++i)
        {
            *(first + i)
                = this->apply<Op>(*(first + 2 * i), *(first + 2 * i + 1));
        }

        if (justMoveLast)
        {
            *(first + currentCount - 1)
                = static_cast<diagram_t&&>(*(first + 2 * (currentCount - 1)));
        }
    }

    return diagram_t(static_cast<diagram_t&&>(*first));
}

template<class Data, class Degree, class Domain>
template<in_var_values Vars>
auto diagram_manager<Data, Degree, Domain>::evaluate(
    diagram_t const& diagram,
    Vars const& values
) const -> int32
{
    node_t* node = diagram.unsafe_get_root();

    while (not node->is_terminal())
    {
        int32 const index = node->get_index();
        assert(nodes_.is_valid_var_value(index, values[as_uindex(index)]));
        node = node->get_son(values[as_uindex(index)]);
    }

    return node->get_value();
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::satisfy_count(
    int32 const value,
    diagram_t const& diagram
) -> int64
{
    if constexpr (domains::is_fixed<Domain>::value)
    {
        assert(value < Domain::value);
    }

    auto constexpr CanUseDataMember = not utils::is_void<Data>::value;
    using T = utils::type_if<CanUseDataMember, Data, int64>::type;

    // A function that returns reference to
    // the data associated with given node.
    auto data = [] ()
    {
        if constexpr (CanUseDataMember)
        {
            // Simply return reference to the data member.
            return [] (node_t* const node) mutable -> T&
            { return (node->get_data()); };
        }
        else
        {
            // Return reference to the data that is stored in the map.
            return [map = std::unordered_map<node_t*, T>()] (node_t* const node
                   ) mutable -> T&
            {
                // If there is no value for given key [] creates new pair
                // and value-initializes the value (0 for primitive types).
                return map[node];
            };
        }
    }();

    node_t* const root = diagram.unsafe_get_root();

    // Actual satisfy count algorithm.
    nodes_.traverse_post(
        root,
        [this, value, &data] (node_t* const node) mutable
        {
            if (node->is_terminal())
            {
                data(node) = node->get_value() == value ? 1 : 0;
            }
            else
            {
                data(node)             = 0;
                int32 const nodeLevel  = nodes_.get_level(node);
                int32 const nodeDomain = nodes_.get_domain(node);
                for (int32 k = 0; k < nodeDomain; ++k)
                {
                    node_t* const son    = node->get_son(k);
                    int32 const sonLevel = nodes_.get_level(son);
                    int64 const diff
                        = nodes_.domain_product(nodeLevel + 1, sonLevel);
                    data(node) += data(son) * static_cast<T>(diff);
                }
            }
        }
    );

    auto const rootAlpha  = static_cast<int64>(data(root));
    int32 const rootLevel = nodes_.get_level(root);
    return rootAlpha * nodes_.domain_product(0, rootLevel);
}

template<class Data, class Degree, class Domain>
template<out_var_values Vars>
auto diagram_manager<Data, Degree, Domain>::satisfy_one(
    int32 const value,
    diagram_t const& diagram
) -> std::optional<Vars>
{
    if constexpr (domains::is_fixed<Domain>::value)
    {
        assert(value < Domain::value);
    }

    Vars vars;
    if constexpr (utils::is_std_vector<Vars>)
    {
        vars.resize(as_usize(this->get_var_count()));
    }

    node_t* const root = diagram.unsafe_get_root();
    return this->satisfy_one_impl(value, vars, root) ? std::make_optional(vars)
                                                     : std::nullopt;
}

template<class Data, class Degree, class Domain>
template<class Vars>
auto diagram_manager<Data, Degree, Domain>::satisfy_one_impl(
    int32 const value,
    Vars& vars,
    node_t* const node
) -> bool
{
    if (node->is_terminal())
    {
        return node->get_value() == value;
    }

    int32 const nodeIndex  = node->get_index();
    int32 const nodeDomain = nodes_.get_domain(nodeIndex);
    for (auto k = 0; k < nodeDomain; ++k)
    {
        node_t* const son          = node->get_son(k);
        vars[as_uindex(nodeIndex)] = k;
        if (this->satisfy_one_impl(value, vars, son))
        {
            return true;
        }
    }

    return false;
}

template<class Data, class Degree, class Domain>
template<out_var_values Vars>
auto diagram_manager<Data, Degree, Domain>::satisfy_all(
    int32 const value,
    diagram_t const& diagram
) const -> std::vector<Vars>
{
    std::vector<Vars> result;
    this->satisfy_all_g<Vars>(value, diagram, std::back_inserter(result));
    return result;
}

template<class Data, class Degree, class Domain>
template<out_var_values Vars, std::output_iterator<Vars> OutputIt>
auto diagram_manager<Data, Degree, Domain>::satisfy_all_g(
    int32 const value,
    diagram_t const& diagram,
    OutputIt out
) const -> void
{
    if constexpr (domains::is_fixed<Domain>::value)
    {
        assert(value < Domain::value);
    }

    Vars vars;
    if constexpr (utils::is_std_vector<Vars>)
    {
        vars.resize(as_usize(this->get_var_count()));
    }

    node_t* const root = diagram.unsafe_get_root();
    this->satisfy_all_impl(value, vars, out, root, 0);
}

template<class Data, class Degree, class Domain>
template<class Vars, class OutputIt>
auto diagram_manager<Data, Degree, Domain>::satisfy_all_impl(
    int32 const value,
    Vars& vars,
    OutputIt out,
    node_t* const node,
    int32 const level
) const -> void
{
    if (node->is_terminal() && value != node->get_value())
    {
        return;
    }

    if (level == nodes_.get_leaf_level() && value == node->get_value())
    {
        *out++ = vars;
        return;
    }

    if (nodes_.get_level(node) > level)
    {
        int32 const index  = nodes_.get_index(level);
        int32 const domain = nodes_.get_domain(index);
        for (auto k = 0; k < domain; ++k)
        {
            vars[as_uindex(index)] = k;
            this->satisfy_all_impl(value, vars, out, node, level + 1);
        }
    }
    else
    {
        int32 const index  = node->get_index();
        int32 const domain = nodes_.get_domain(index);
        for (auto k = 0; k < domain; ++k)
        {
            vars[as_uindex(index)] = k;
            node_t* const son      = node->get_son(k);
            this->satisfy_all_impl(value, vars, out, son, level + 1);
        }
    }
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::get_cofactor(
    diagram_t const& diagram,
    int32 const varIndex,
    int32 const varValue
) -> diagram_t
{
    node_t* const root = diagram.unsafe_get_root();
    if (root->is_terminal())
    {
        return diagram;
    }

    if (root->get_index() == varIndex)
    {
        return diagram_t(root->get_son(varValue));
    }

    std::unordered_map<node_t*, node_t*> memo;
    diagram_t result
        = diagram_t(this->get_cofactor_impl(memo, varIndex, varValue, root));
    nodes_.run_deferred();
    return result;
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::get_cofactor(
    diagram_t const& diagram,
    std::vector<var_cofactor> const& vars
) -> diagram_t
{
    node_t* root = diagram.unsafe_get_root();
    if (root->is_terminal())
    {
        return diagram;
    }

    auto const it = utils::find_if(vars.begin(), vars.end(),
        [root](var_cofactor var)
    {
        return var.index_ == root->get_index();
    });

    int32 toCofactor = static_cast<int32>(vars.size());
    if (it != vars.end())
    {
        root = root->get_son(it->value_);
        --toCofactor;
    }

    std::unordered_map<node_t*, node_t*> memo;
    diagram_t result = diagram_t(this->get_cofactor_impl(
        memo,
        vars,
        root,
        toCofactor
    ));
    nodes_.run_deferred();
    return result;
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::get_cofactor_impl(
    std::unordered_map<node_t*, node_t*>& memo,
    int32 const varIndex,
    int32 const varValue,
    node_t* const node
) -> node_t*
{
    auto const memoIt = memo.find(node);
    if (memoIt != memo.end())
    {
        return memoIt->second;
    }

    if (node->is_terminal())
    {
        return node;
    }

    int32 const nodeIndex = node->get_index();
    if (nodeIndex == varIndex)
    {
        return node->get_son(varValue);
    }

    int32 const nodeDomain = nodes_.get_domain(node);
    son_container sons     = nodes_.make_son_container(nodeDomain);
    for (int32 k = 0; k < nodeDomain; ++k)
    {
        node_t* const oldSon = node->get_son(k);
        sons[k] = this->get_cofactor_impl(memo, varIndex, varValue, oldSon);
    }

    node_t* const newNode = nodes_.make_internal_node(nodeIndex, sons);
    memo.emplace(node, newNode);
    return newNode;
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::get_cofactor_impl(
    std::unordered_map<node_t*, node_t*>& memo,
    std::vector<var_cofactor> const& vars,
    node_t* const node,
    int32 const toCofactor
) -> node_t*
{
    if (toCofactor == 0)
    {
        return node;
    }

    auto const memoIt = memo.find(node);
    if (memoIt != memo.end())
    {
        return memoIt->second;
    }

    if (node->is_terminal())
    {
        return node;
    }

    int32 const nodeIndex = node->get_index();
    auto const it = utils::find_if(vars.begin(), vars.end(),
        [nodeIndex](var_cofactor var)
    {
        return var.index_ == nodeIndex;
    });

    node_t* newNode = nullptr;
    if (it != vars.end())
    {
        newNode = this->get_cofactor_impl(
            memo,
            vars,
            node->get_son(it->value_),
            toCofactor - 1
        );
    }
    else
    {
        int32 const nodeDomain = nodes_.get_domain(node);
        son_container sons     = nodes_.make_son_container(nodeDomain);
        for (int32 k = 0; k < nodeDomain; ++k)
        {
            node_t* const oldSon = node->get_son(k);
            sons[k] = this->get_cofactor_impl(memo, vars, oldSon, toCofactor);
        }
        newNode = nodes_.make_internal_node(nodeIndex, sons);
    }

    memo.emplace(node, newNode);
    return newNode;
}

template<class Data, class Degree, class Domain>
template<int_to_int F>
auto diagram_manager<Data, Degree, Domain>::transform(
    diagram_t const& diagram,
    F transformer
) -> diagram_t
{
    std::unordered_map<node_t*, node_t*> memo;
    node_t* const newRoot
        = this->transform_impl(memo, transformer, diagram.unsafe_get_root());
    nodes_.run_deferred();
    return diagram_t(newRoot);
}

template<class Data, class Degree, class Domain>
template<class F>
auto diagram_manager<Data, Degree, Domain>::transform_impl(
    std::unordered_map<node_t*, node_t*>& memo,
    F transformer,
    node_t* node
) -> node_t*
{
    auto const memoIt = memo.find(node);
    if (memo.end() != memoIt)
    {
        return memoIt->second;
    }

    if (node->is_terminal())
    {
        int32 const newVal = static_cast<int32>(transformer(node->get_value()));
        return nodes_.make_terminal_node(newVal);
    }

    int32 const index  = node->get_index();
    int32 const domain = nodes_.get_domain(index);
    son_container sons = nodes_.make_son_container(domain);
    for (int32 k = 0; k < domain; ++k)
    {
        node_t* const son = node->get_son(k);
        sons[k]           = this->transform_impl(memo, transformer, son);
    }
    node_t* const newNode = nodes_.make_internal_node(index, sons);
    memo.emplace(node, newNode);
    return newNode;
}

template<class Data, class Degree, class Domain>
template<class Foo>
requires(is_bdd<Degree>)
auto diagram_manager<Data, Degree, Domain>::negate(diagram_t const& diagram)
    -> utils::second_t<Foo, diagram_t>
{
    return this->transform(
        diagram,
        [] (int32 const value) { return 1 - value; }
    );
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::get_dependency_set(
    diagram_t const& diagram
) const -> std::vector<int32>
{
    std::vector<int32> indices;
    indices.reserve(this->get_var_count());
    this->get_dependency_set_g(diagram, std::back_inserter(indices));
    indices.shrink_to_fit();
    return indices;
}

template<class Data, class Degree, class Domain>
template<std::output_iterator<int32> O>
auto diagram_manager<Data, Degree, Domain>::get_dependency_set_g(
    diagram_t const& diagram,
    O out
) const -> void
{
    std::vector<bool> memo(as_usize(this->get_var_count()), false);
    nodes_.traverse_pre(
        diagram.unsafe_get_root(),
        [&memo, out] (node_t* const node) mutable
        {
            if (node->is_internal())
            {
                int32 const index = node->get_index();
                if (not memo[as_uindex(index)])
                {
                    *out++ = index;
                }
                memo[as_uindex(index)] = true;
            }
        }
    );
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::reduce(diagram_t const& diagram)
    -> diagram_t
{
    return this->transform(diagram, [] (int32 const val) { return val; });
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::get_node_count() const -> int64
{
    return nodes_.get_node_count();
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::get_node_count(
    diagram_t const& diagram
) const -> int64
{
    return nodes_.get_node_count(diagram.unsafe_get_root());
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::to_dot_graph(std::ostream& out
) const -> void
{
    nodes_.to_dot_graph(out);
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::to_dot_graph(
    std::ostream& out,
    diagram_t const& diagram
) const -> void
{
    nodes_.to_dot_graph(out, diagram.unsafe_get_root());
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::get_var_count() const -> int32
{
    return nodes_.get_var_count();
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::get_order() const
    -> std::vector<int32> const&
{
    return nodes_.get_order();
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::get_domains() const
    -> std::vector<int32>
{
    return nodes_.get_domains();
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::set_cache_ratio(double ratio)
    -> void
{
    nodes_.set_cache_ratio(ratio);
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::set_gc_ratio(double ratio) -> void
{
    nodes_.set_gc_ratio(ratio);
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::set_auto_reorder(
    bool const doReorder
) -> void
{
    nodes_.set_auto_reorder(doReorder);
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::force_gc() -> void
{
    nodes_.force_gc();
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::force_reorder() -> void
{
    nodes_.sift_variables();
}

template<class Data, class Degree, class Domain>
auto diagram_manager<Data, Degree, Domain>::clear_cache() -> void
{
    nodes_.cache_clear();
}

namespace detail
{
inline auto default_or_fwd (int32 const varCount, std::vector<int32>& indices)
{
    if (indices.empty())
    {
        std::vector<int32> defaultIndices;
        defaultIndices.reserve(as_usize(varCount));
        for (int32 index = 0; index < varCount; ++index)
        {
            defaultIndices.push_back(index);
        }
        return defaultIndices;
    }
    else
    {
        return std::vector<int32>(static_cast<std::vector<int32>&&>(indices));
    }
}
} // namespace detail

template<class Data, class Degree, class Domain>
diagram_manager<Data, Degree, Domain>::diagram_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const extraNodePoolSize,
    std::vector<int32> order
)
requires(domains::is_fixed<Domain>::value)
    :
    nodes_(
        varCount,
        nodePoolSize,
        extraNodePoolSize,
        detail::default_or_fwd(varCount, order)
    )
{
}

template<class Data, class Degree, class Domain>
diagram_manager<Data, Degree, Domain>::diagram_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const extraNodePoolSize,
    domains::mixed domain,
    std::vector<int32> order
)
requires(domains::is_mixed<Domain>::value)
    :
    nodes_(
        varCount,
        nodePoolSize,
        extraNodePoolSize,
        detail::default_or_fwd(varCount, order),
        static_cast<domains::mixed&&>(domain)
    )
{
}
} // namespace teddy

#endif