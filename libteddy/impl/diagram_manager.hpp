#ifndef LIBTEDDY_DETAILS_DIAGRAM_MANAGER_HPP
#define LIBTEDDY_DETAILS_DIAGRAM_MANAGER_HPP

#include <libteddy/impl/diagram.hpp>
#include <libteddy/impl/memo.hpp>
#include <libteddy/impl/node_manager.hpp>
#include <libteddy/impl/operators.hpp>
#include <libteddy/impl/pla_file.hpp>
#include <libteddy/impl/stats.hpp>
#include <libteddy/impl/tools.hpp>
#include <libteddy/impl/types.hpp>

#include <cmath>

#include <initializer_list>
#include <iterator>
#include <optional>
#include <vector>

namespace teddy
{
template<class Vars>
concept in_var_values = requires(Vars values, int32 index) {
  { values[index] } -> std::convertible_to<int32>;
};

template<class Vars>
concept out_var_values
  = requires(Vars values, int32 index, int32 value) { values[index] = value; };

// TODO(michal): move to io
template<class Node>
concept expression_node = requires(Node node, int32 value) {
  { node.is_variable() } -> std::same_as<bool>;
  { node.is_constant() } -> std::same_as<bool>;
  { node.is_operation() } -> std::same_as<bool>;
  { node.get_index() } -> std::same_as<int32>;
  { node.get_value() } -> std::same_as<int32>;
  { node.evaluate(value, value) } -> std::same_as<int32>;
  { node.get_left() } -> std::same_as<Node const&>;
  { node.get_right() } -> std::same_as<Node const&>;
};

template<class Degree>
concept is_bdd = std::same_as<degrees::fixed<2>, Degree>;

template<class F>
concept int_to_int = requires(F function, int32 value) {
  { function(value) } -> std::convertible_to<int32>;
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
 *  \brief Struct containing interface of the io module
 *  Forward-declared here so that it can be made friend of diagram_manager
 */
struct io;

namespace details
{
  /**
   *  \brief Struct containing implementation of the io module
   *  Forward-declared here so that it can be made friend of diagram_manager
   */
  struct io_impl;
} // namespace details

/**
 *  \class diagram_manager
 *  \brief Base class for all diagram managers that generically
 *  implements all of the algorithms
 */
template<class Degree, class Domain>
class diagram_manager
{
public:
  /**
   *  \brief Alias for the diagram type
   */
  using diagram_t = diagram<Degree>;

  /**
   *  \brief IO module is our friend
   */
  friend struct io;

  /**
   *  \brief IO module is our friend
   */
  friend struct details::io_impl;

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
   *  \tparam S sentinel type for \p I (end iterator)
   *  \param first iterator to the first element of range of indices
   *  represented by integral type convertible to unsgined int
   *  \param last sentinel for \p first (end iterator)
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

  /**
   *  \brief TODO
   */
  // template<teddy_bin_op Op>
  // auto apply_own (diagram_t const& lhs, diagram_t const& rhs) -> diagram_t;

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
   *  \brief Calculates the number of variable assignments for which
   *  the function evaluates to certain value
   *
   *  Complexity is \c O(|d|) where \c |d| is the number of nodes.
   *
   *  \param value Value of the function
   *  \param diagram Diagram representing the function
   *  \return Number of different variable assignments for which the
   *  the function represented by \p d evaluates to \p val
   */
  auto satisfy_count (int32 value, diagram_t const& diagram) -> longint;

  // TODO(michal): radsej pridat overload co bude vracat longing a nechat henten
  // nech vracia int64

  /**
   *  \brief Calculates the logarithm of the number of variable assignments
   *  for which the function evaluates to certain value
   *
   *  Complexity is \c O(|d|) where \c |d| is the number of nodes.
   *
   *  \param value Value of the function
   *  \param diagram Diagram representing the function
   *  \return Base-2 logarithm of the number of different variable assignments
   */
  template<class Foo = void>
  requires(is_bdd<Degree>)
  auto satisfy_count_ln (diagram_t const& diagram
  ) -> utils::second_t<Foo, double>;

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
  auto get_cofactor (diagram_t const& diagram, int32 varIndex, int32 varValue)
    -> diagram_t;

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
   *  \return Diagram representing negated function
   */
  template<class Foo = void>
  requires(is_bdd<Degree>)
  auto negate (diagram_t const& diagram) -> utils::second_t<Foo, diagram_t>;

  /**
   *  \brief Enumerates indices of variables that the function depends on
   *  \param diagram Diagram representing the function
   *  \return Vector of indices
   */
  auto get_dependency_set (diagram_t const& diagram
  ) const -> std::vector<int32>;

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
  [[nodiscard]]
  auto get_node_count () const -> int64;

  /**
   *  \brief Returns number of nodes in the diagram including
   *  terminal nodes
   *  \param diagram Diagram
   *  \return Number of node
   */
  auto get_node_count (diagram_t const& diagram) const -> int64;

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
  [[nodiscard]]
  auto get_var_count () const -> int32;

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
  [[nodiscard]]
  auto get_order () const -> std::vector<int32> const&;

  /**
   * \brief Returns the size of the domain of \p index variable
   *
   * \param index Index of the variables
   * \return int32 Size of the domain of the variable
   */
  [[nodiscard]]
  auto get_domain (int32 index) const -> int32;

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
  [[nodiscard]]
  auto get_domains () const -> std::vector<int32>;

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
  using node_t        = typename diagram_t::node_t;
  using son_container = typename node_t::son_container;
  template<class ValueType>
  using node_memo = details::map_memo<ValueType, Degree>;
  // TODO(michal): clanok s porovnanim

private:
  template<int32 Size>
  struct node_pack
  {
    node_t* key_[static_cast<std::size_t>(Size)] {nullptr};
    node_t* result_ {nullptr};
  };

  // TODO(michal): tmp
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
  auto variable_impl (int32 index) -> node_t*;

  template<class Op>
  auto apply_impl (Op operation, node_t* lhs, node_t* rhs) -> node_t*;

  template<class Op, class... Node>
  auto apply_n_impl (
    std::vector<node_pack<sizeof...(Node)>>& cache,
    Op operation,
    Node... nodes
  ) -> node_t*;

  template<class Int>
  auto satisfy_count_impl (node_memo<Int>& memo, int32 value, node_t* node)
    -> Int;

  auto satisfy_count_ln_impl (node_memo<double>& memo, node_t* node) -> double;

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
  auto transform_impl (node_memo<node_t*>& memo, F transformer, node_t* node)
    -> node_t*;

  template<class ExprNode>
  auto from_expression_tree_impl (
    std::vector<node_pack<2>>& cache,
    ExprNode const& exprNode
  ) -> node_t*;

protected:
  template<class ValueType>
  auto make_node_memo (node_t* root) -> node_memo<ValueType>;

  auto get_node_manager () -> node_manager<Degree, Domain>&;

  auto get_node_manager () const -> node_manager<Degree, Domain> const&;

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

private:
  node_manager<Degree, Domain> nodes_;
};

using binary_manager = diagram_manager<degrees::fixed<2>, domains::fixed<2>>;

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::constant(int32 const val) -> diagram_t
{
  return diagram_t(nodes_.make_terminal_node(val));
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::variable(int32 const index) -> diagram_t
{
  return diagram_t(this->variable_impl(index));
}

template<class Degree, class Domain>
template<class Foo>
requires(is_bdd<Degree>)
auto diagram_manager<Degree, Domain>::variable_not(int32 const index
) -> utils::second_t<Foo, diagram_t>
{
  son_container sons = node_t::make_son_container(2);
  sons[0]            = nodes_.make_terminal_node(1);
  sons[1]            = nodes_.make_terminal_node(0);
  return diagram_t(nodes_.make_internal_node(index, TEDDY_MOVE(sons)));
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::operator() (int32 const index
) -> diagram_t
{
  return this->variable(index);
}

template<class Degree, class Domain>
template<std::ranges::input_range Is>
auto diagram_manager<Degree, Domain>::variables(Is const& indices
) -> std::vector<diagram_t>
{
  return this->variables(begin(indices), end(indices));
}

template<class Degree, class Domain>
template<std::convertible_to<int32> T>
auto diagram_manager<Degree, Domain>::variables(
  std::initializer_list<T> const indices
) -> std::vector<diagram_t>
{
  return this->variables(begin(indices), end(indices));
}

template<class Degree, class Domain>
template<std::input_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Degree, Domain>::variables(I first, S const last)
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

template<class Degree, class Domain>
template<expression_node Node>
auto diagram_manager<Degree, Domain>::from_expression_tree(Node const& root
) -> diagram_t
{
  int64 constexpr CacheCapacity = 100'000;
  std::vector<node_pack<2>> cache(as_usize(CacheCapacity));
  node_t* newRoot = this->from_expression_tree_impl(cache, root);
  nodes_.run_deferred();
  return diagram_t(newRoot);
}

template<class Degree, class Domain>
template<class ExprNode>
auto diagram_manager<Degree, Domain>::from_expression_tree_impl(
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

  node_t* const newRoot    = this->apply_n_impl(cache, operation, left, right);
  auto const cacheCapacity = cache.size();
  cache.clear();
  cache.resize(cacheCapacity);
  return newRoot;
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::variable_impl(int32 const index
) -> node_t*
{
  int32 const varDomain = nodes_.get_domain(index);
  son_container sons    = node_t::make_son_container(varDomain);
  for (int32 val = 0; val < varDomain; ++val)
  {
    sons[val] = nodes_.make_terminal_node(val);
  }
  return nodes_.make_internal_node(index, TEDDY_MOVE(sons));
}

template<class Degree, class Domain>
template<teddy_bin_op Op>
auto diagram_manager<Degree, Domain>::apply(
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

  node_t* const newRoot
    = this->apply_impl(OpType(), lhs.unsafe_get_root(), rhs.unsafe_get_root());
  nodes_.run_deferred();
  return diagram_t(newRoot);
}

template<class Degree, class Domain>
template<class Op>
auto diagram_manager<Degree, Domain>::apply_impl(
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
  son_container sons   = node_t::make_son_container(domain);
  for (int32 k = 0; k < domain; ++k)
  {
    sons[k] = this->apply_impl(
      operation,
      lhsLevel == topLevel ? lhs->get_son(k) : lhs,
      rhsLevel == topLevel ? rhs->get_son(k) : rhs
    );
  }

  node_t* const result = nodes_.make_internal_node(topIndex, TEDDY_MOVE(sons));
  nodes_.template cache_put<Op>(result, lhs, rhs);
  return result;
}

template<class Degree, class Domain>
template<teddy_bin_op Op, class... Diagram>
auto diagram_manager<Degree, Domain>::apply_n(Diagram const&... diagram
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

  // TODO(michal): capacity, this is temporary
  int32 const cacheSize = 100'000;
  std::vector<node_pack<sizeof...(Diagram)>> cache(cacheSize);
  node_t* const newRoot
    = this->apply_n_impl(cache, OpType(), diagram.unsafe_get_root()...);
  nodes_.run_deferred();
  return diagram_t(newRoot);
}

template<class Degree, class Domain>
template<class Op, class... Node>
auto diagram_manager<Degree, Domain>::apply_n_impl(
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

  int32 const opVal
    = operation((nodes->is_terminal() ? nodes->get_value() : Nondetermined)...);
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
    son_container sons   = node_t::make_son_container(domain);
    for (int32 k = 0; k < domain; ++k)
    {
      sons[k] = this->apply_n_impl(
        cache,
        operation,
        (nodes_.get_level(nodes) == minLevel ? nodes->get_son(k) : nodes)...
      );
    }
    result = nodes_.make_internal_node(topIndex, TEDDY_MOVE(sons));
  }

  cachePack = {{nodes...}, result};
  return result;
}

template<class Degree, class Domain>
template<teddy_bin_op Op, std::ranges::input_range R>
auto diagram_manager<Degree, Domain>::left_fold(R const& diagrams) -> diagram_t
{
  return this->left_fold<Op>(begin(diagrams), end(diagrams));
}

template<class Degree, class Domain>
template<teddy_bin_op Op, std::input_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Degree, Domain>::left_fold(I first, S const last)
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

template<class Degree, class Domain>
template<teddy_bin_op Op, std::ranges::random_access_range R>
auto diagram_manager<Degree, Domain>::tree_fold(R& diagrams) -> diagram_t
{
  return this->tree_fold<Op>(begin(diagrams), end(diagrams));
}

template<class Degree, class Domain>
template<teddy_bin_op Op, std::random_access_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Degree, Domain>::tree_fold(I first, S const last)
  -> diagram_t
{
  static_assert(std::same_as<std::iter_value_t<I>, diagram_t>);

  int64 const count     = std::distance(first, last);
  int64 currentCount    = count;
  auto const numOfSteps = static_cast<int64>(std::ceil(std::log2(count)));

  for (auto step = 0; step < numOfSteps; ++step)
  {
    auto const justMoveLast
      = static_cast<bool>(static_cast<uint64>(currentCount) & 1U);
    currentCount          = (currentCount / 2) + justMoveLast;
    int64 const pairCount = currentCount - justMoveLast;

    for (int64 i = 0; i < pairCount; ++i)
    {
      *(first + i) = this->apply<Op>(*(first + 2 * i), *(first + 2 * i + 1));
    }

    if (justMoveLast)
    {
      *(first + currentCount - 1)
        = TEDDY_MOVE(*(first + 2 * (currentCount - 1)));
    }
  }

  return diagram_t(TEDDY_MOVE(*first));
}

template<class Degree, class Domain>
template<in_var_values Vars>
auto diagram_manager<Degree, Domain>::evaluate(
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

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::satisfy_count(
  int32 const value,
  diagram_t const& diagram
) -> longint
{
  node_t* const root          = diagram.unsafe_get_root();
  int32 const rootLevel       = nodes_.get_level(root);
  longint stepResult          = 0;
  int32 const nativeThreshold = 63;
  if (this->get_var_count() < nativeThreshold)
  {
    node_memo<int64> memo = this->make_node_memo<int64>(root);
    stepResult            = this->satisfy_count_impl<int64>(memo, value, root);
    return stepResult * nodes_.template domain_product<int64>(0, rootLevel);
  }
  else
  {
    node_memo<longint> memo = this->make_node_memo<longint>(root);
    stepResult = this->satisfy_count_impl<longint>(memo, value, root);
    return stepResult * nodes_.template domain_product<longint>(0, rootLevel);
  }
}

template<class Degree, class Domain>
template<class Int>
auto diagram_manager<Degree, Domain>::satisfy_count_impl(
  node_memo<Int>& memo,
  int32 const value,
  node_t* const node
) -> Int
{
  if (node->is_terminal())
  {
    return node->get_value() == value ? 1 : 0;
  }

  Int* const memoized = memo.find(node);
  if (memoized)
  {
    return *memoized;
  }

  Int result             = 0;
  int32 const nodeLevel  = nodes_.get_level(node);
  int32 const nodeDomain = nodes_.get_domain(node);
  for (int32 k = 0; k < nodeDomain; ++k)
  {
    node_t* const son    = node->get_son(k);
    int32 const sonLevel = nodes_.get_level(son);
    Int const diff
      = nodes_.template domain_product<Int>(nodeLevel + 1, sonLevel);
    Int const sonResult = this->satisfy_count_impl(memo, value, son);
    result += sonResult * diff;
  }

  memo.put(node, result);
  return result;
}

template<class Degree, class Domain>
template<class Foo>
requires(is_bdd<Degree>)
auto diagram_manager<Degree, Domain>::satisfy_count_ln(diagram_t const& diagram
) -> utils::second_t<Foo, double>
{
  node_t* const root     = diagram.unsafe_get_root();
  node_memo<double> memo = this->make_node_memo<double>(root);
  double result          = this->satisfy_count_ln_impl(memo, root);
  if (result >= 0.0)
  {
    result += nodes_.get_level(root);
  }
  return result;
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::satisfy_count_ln_impl(
  node_memo<double>& memo,
  node_t* const node
) -> double
{
  // Implementation from https://sourceforge.net/projects/buddy/

  static double const Ln2 = std::log(2);

  if (node->is_terminal())
  {
    return node->get_value() == 0 ? -1.0 : 0.0;
  }

  double* const memoized = memo.find(node);
  if (memoized != nullptr)
  {
    return *memoized;
  }

  node_t* const son0 = node->get_son(0);
  node_t* const son1 = node->get_son(1);
  int32 const level  = nodes_.get_level(node);
  int32 const level0 = nodes_.get_level(son0);
  int32 const level1 = nodes_.get_level(son1);

  double s1          = this->satisfy_count_ln_impl(memo, son0);
  if (s1 >= 0)
  {
    s1 += level0 - level - 1;
  }

  double s2 = this->satisfy_count_ln_impl(memo, son1);
  if (s2 >= 0)
  {
    s2 += level1 - level - 1;
  }

  double result = -1;
  if (s1 < 0.0)
  {
    result = s2;
  }
  else if (s2 < 0.0)
  {
    result = s1;
  }
  else if (s1 < s2)
  {
    result = s2 + std::log1p(std::pow(2.0, s1 - s2)) / Ln2;
  }
  else
  {
    result = s1 + std::log1p(std::pow(2.0, s2 - s1)) / Ln2;
  }

  memo.put(node, result);
  return result;
}

template<class Degree, class Domain>
template<out_var_values Vars>
auto diagram_manager<Degree, Domain>::satisfy_one(
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

template<class Degree, class Domain>
template<class Vars>
auto diagram_manager<Degree, Domain>::satisfy_one_impl(
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

template<class Degree, class Domain>
template<out_var_values Vars>
auto diagram_manager<Degree, Domain>::satisfy_all(
  int32 const value,
  diagram_t const& diagram
) const -> std::vector<Vars>
{
  std::vector<Vars> result;
  this->satisfy_all_g<Vars>(value, diagram, std::back_inserter(result));
  return result;
}

template<class Degree, class Domain>
template<out_var_values Vars, std::output_iterator<Vars> OutputIt>
auto diagram_manager<Degree, Domain>::satisfy_all_g(
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

template<class Degree, class Domain>
template<class Vars, class OutputIt>
auto diagram_manager<Degree, Domain>::satisfy_all_impl(
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

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_cofactor(
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

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_cofactor(
  diagram_t const& diagram,
  std::vector<var_cofactor> const& vars
) -> diagram_t
{
  node_t* root = diagram.unsafe_get_root();
  if (root->is_terminal())
  {
    return diagram;
  }

  auto const it = utils::find_if(
    vars.begin(),
    vars.end(),
    [root] (var_cofactor var) { return var.index_ == root->get_index(); }
  );

  auto toCofactor = static_cast<int32>(vars.size());
  if (it != vars.end())
  {
    root = root->get_son(it->value_);
    --toCofactor;
  }

  std::unordered_map<node_t*, node_t*> memo;
  diagram_t result
    = diagram_t(this->get_cofactor_impl(memo, vars, root, toCofactor));
  nodes_.run_deferred();
  return result;
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_cofactor_impl(
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

  int32 const nodeLevel = nodes_.get_level(nodeIndex);
  int32 const varLevel  = nodes_.get_level(varIndex);
  if (nodeLevel > varLevel)
  {
    return node;
  }

  int32 const nodeDomain = nodes_.get_domain(node);
  son_container sons     = node_t::make_son_container(nodeDomain);
  for (int32 k = 0; k < nodeDomain; ++k)
  {
    node_t* const oldSon = node->get_son(k);
    sons[k] = this->get_cofactor_impl(memo, varIndex, varValue, oldSon);
  }

  node_t* const newNode
    = nodes_.make_internal_node(nodeIndex, TEDDY_MOVE(sons));
  memo.emplace(node, newNode);
  return newNode;
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_cofactor_impl(
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
  auto const it         = utils::find_if(
    vars.begin(),
    vars.end(),
    [nodeIndex] (var_cofactor var) { return var.index_ == nodeIndex; }
  );

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
    son_container sons     = node_t::make_son_container(nodeDomain);
    for (int32 k = 0; k < nodeDomain; ++k)
    {
      node_t* const oldSon = node->get_son(k);
      sons[k] = this->get_cofactor_impl(memo, vars, oldSon, toCofactor);
    }
    newNode = nodes_.make_internal_node(nodeIndex, TEDDY_MOVE(sons));
  }

  memo.emplace(node, newNode);
  return newNode;
}

template<class Degree, class Domain>
template<int_to_int F>
auto diagram_manager<Degree, Domain>::transform(
  diagram_t const& diagram,
  F transformer
) -> diagram_t
{
  node_t* const root      = diagram.unsafe_get_root();
  node_memo<node_t*> memo = this->make_node_memo<node_t*>(root);
  node_t* const newRoot   = this->transform_impl(memo, transformer, root);
  nodes_.run_deferred();
  return diagram_t(newRoot);
}

template<class Degree, class Domain>
template<class F>
auto diagram_manager<Degree, Domain>::transform_impl(
  node_memo<node_t*>& memo,
  F transformer,
  node_t* node
) -> node_t*
{
  if (node->is_terminal())
  {
    auto const newVal = static_cast<int32>(transformer(node->get_value()));
    return nodes_.make_terminal_node(newVal);
  }

  node_t** const memoized = memo.find(node);
  if (memoized)
  {
    return *memoized;
  }

  int32 const index  = node->get_index();
  int32 const domain = nodes_.get_domain(index);
  son_container sons = node_t::make_son_container(domain);
  for (int32 k = 0; k < domain; ++k)
  {
    node_t* const son = node->get_son(k);
    sons[k]           = this->transform_impl(memo, transformer, son);
  }
  node_t* const newNode = nodes_.make_internal_node(index, TEDDY_MOVE(sons));
  memo.put(node, newNode);
  return newNode;
}

template<class Degree, class Domain>
template<class Foo>
requires(is_bdd<Degree>)
auto diagram_manager<Degree, Domain>::negate(diagram_t const& diagram
) -> utils::second_t<Foo, diagram_t>
{
  return this->transform(diagram, [] (int32 const value) { return 1 - value; });
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_dependency_set(
  diagram_t const& diagram
) const -> std::vector<int32>
{
  std::vector<int32> indices;
  indices.reserve(this->get_var_count());
  this->get_dependency_set_g(diagram, std::back_inserter(indices));
  indices.shrink_to_fit();
  return indices;
}

template<class Degree, class Domain>
template<std::output_iterator<int32> O>
auto diagram_manager<Degree, Domain>::get_dependency_set_g(
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

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::reduce(diagram_t const& diagram
) -> diagram_t
{
  return this->transform(diagram, [] (int32 const val) { return val; });
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_node_count() const -> int64
{
  return nodes_.get_node_count();
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_node_count(diagram_t const& diagram
) const -> int64
{
  return nodes_.get_node_count(diagram.unsafe_get_root());
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_var_count() const -> int32
{
  return nodes_.get_var_count();
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_domain(int32 const index
) const -> int32
{
  return nodes_.get_domain(index);
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_order() const
  -> std::vector<int32> const&
{
  return nodes_.get_order();
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_domains() const -> std::vector<int32>
{
  return nodes_.get_domains();
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::set_cache_ratio(double ratio) -> void
{
  nodes_.set_cache_ratio(ratio);
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::set_gc_ratio(double ratio) -> void
{
  nodes_.set_gc_ratio(ratio);
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::set_auto_reorder(bool const doReorder
) -> void
{
  nodes_.set_auto_reorder(doReorder);
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::force_gc() -> void
{
  nodes_.force_gc();
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::force_reorder() -> void
{
  nodes_.sift_variables();
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::clear_cache() -> void
{
  nodes_.cache_clear();
}

template<class Degree, class Domain>
template<class ValueType>
auto diagram_manager<Degree, Domain>::make_node_memo(node_t* const
) -> node_memo<ValueType>
{
  // just map memo for now
  return node_memo<ValueType>(this->get_node_count());
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_node_manager()
  -> node_manager<Degree, Domain>&
{
  return nodes_;
}

template<class Degree, class Domain>
auto diagram_manager<Degree, Domain>::get_node_manager() const
  -> node_manager<Degree, Domain> const&
{
  return nodes_;
}

namespace detail
{
  inline auto default_or_fwd (int32 const varCount, std::vector<int32> indices)
    -> std::vector<int32>
  {
    if (not indices.empty())
    {
      return {TEDDY_MOVE(indices)};
    }

    std::vector<int32> defaultIndices;
    defaultIndices.reserve(as_usize(varCount));
    for (int32 index = 0; index < varCount; ++index)
    {
      defaultIndices.push_back(index);
    }

    return defaultIndices;
  }

} // namespace detail

template<class Degree, class Domain>
diagram_manager<Degree, Domain>::diagram_manager(
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
    detail::default_or_fwd(varCount, TEDDY_MOVE(order))
  )
{
}

template<class Degree, class Domain>
diagram_manager<Degree, Domain>::diagram_manager(
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
    detail::default_or_fwd(varCount, TEDDY_MOVE(order)),
    TEDDY_MOVE(domain)
  )
{
}
} // namespace teddy

#endif