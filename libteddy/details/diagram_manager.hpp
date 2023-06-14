#ifndef LIBTEDDY_DETAILS_DIAGRAM_MANAGER_HPP
#define LIBTEDDY_DETAILS_DIAGRAM_MANAGER_HPP

#include <cmath>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <libteddy/details/diagram.hpp>
#include <libteddy/details/node_manager.hpp>
#include <libteddy/details/operators.hpp>
#include <libteddy/details/pla_file.hpp>
#include <libteddy/details/tools.hpp>
#include <ranges>
#include <tuple>

namespace teddy
{
// TODO move to standalone header

template<class Vars>
concept in_var_values = requires(Vars vs, int32 i) {
                            {
                                vs[i]
                            } -> std::convertible_to<int32>;
                        };

template<class Vars>
concept out_var_values = requires(Vars vs, int32 i, int32 v) { vs[i] = v; };

template<class T>
concept expression_node = requires(T t, int32 l, int32 r) {
                              {
                                  t.is_variable()
                              } -> std::same_as<bool>;
                              {
                                  t.is_constant()
                              } -> std::same_as<bool>;
                              {
                                  t.is_operation()
                              } -> std::same_as<bool>;
                              {
                                  t.get_index()
                              } -> std::same_as<int32>;
                              {
                                  t.get_value()
                              } -> std::same_as<int32>;
                              {
                                  t.evaluate(l, r)
                              } -> std::same_as<int32>;
                              {
                                  t.get_left()
                              } -> std::same_as<T const&>;
                              {
                                  t.get_right()
                              } -> std::same_as<T const&>;
                          };

template<class O>
concept any_bin_op = requires(O o, int32 l, int32 r) {
                         {
                             o(l, r)
                         } -> std::convertible_to<int32>;
                     };

template<class C, class Node>
concept cache_handle = requires(C c, Node* l, Node* r, Node* u) {
                           c.put(l, r, u);
                           {
                               c.lookup(l, r)
                           } -> std::same_as<Node*>;
                       };

template<class Degree>
concept is_bdd = std::same_as<degrees::fixed<2>, Degree>;

template<class T>
concept is_std_vector = std::
    same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>;

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
    auto constant(int32 v) -> diagram_t;

    /**
     *  \brief Creates diagram representing function of single variable.
     *  \param i Index of the variable.
     *  \return Diagram of a function of single variable.
     */
    auto variable(int32 i) -> diagram_t;

    /**
     *  \brief Creates BDD representing function of complemented variable.
     *  \param i Index of the variable.
     *  \return Diagram of a function of single variable.
     */
    template<class Foo = void>
    requires(is_bdd<Degree>)
    auto variable_not(int32 i) -> second_t<Foo, diagram_t>;

    /**
     *  \brief Creates diagram representing function of single variable.
     *  \param i Index of the variable.
     *  \return Diagram of a function of single variable.
     */
    auto operator()(int32 i) -> diagram_t;

    /**
     *  \brief Creates vector of diagrams representing functions
     *  of single variables.
     *
     *  \tparam T integral type convertible to unsgined int.
     *  \param is initializer list of indices.
     *  \return Vector of diagrams.
     */
    template<std::convertible_to<int32> T>
    auto variables(std::initializer_list<T> is) -> std::vector<diagram_t>;

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
    auto variables(I first, S last) -> std::vector<diagram_t>;

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
    auto variables(Is const& is) -> std::vector<diagram_t>;

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
    auto from_vector(I first, S last) -> diagram_t;

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
    auto from_vector(R&& vector) -> diagram_t;

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
    auto to_vector(diagram_t d) const -> std::vector<int32>;

    /**
     *  \brief Creates truth vector from the diagram.
     *
     *  \tparam O Output iterator type
     *  \param d Diagram.
     *  \param out Output iterator that is used to output the truth vector.
     */
    template<std::output_iterator<teddy::int32> O>
    auto to_vector_g(diagram_t d, O out) const -> void;

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
    auto from_pla(pla_file const& file, fold_type foldType = fold_type::Tree)
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
    auto from_expression_tree(Node const& root) -> diagram_t;

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
    auto apply(diagram_t l, diagram_t r) -> diagram_t;

    /**
     *  \brief Merges diagams in the range using the \c apply function
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
    auto left_fold(R const& range) -> diagram_t;

    /**
     *  \brief Merges diagams in the range using the \c apply function
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
    template<bin_op Op, std::input_iterator I, std::sentinel_for<I> S>
    auto left_fold(I first, S last) -> diagram_t;

    /**
     *  \brief Merges diagams in the range using the \c apply function
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
    auto tree_fold(R& range) -> diagram_t;

    /**
     *  \brief Merges diagams in the range using the \c apply function
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
    template<bin_op Op, std::random_access_iterator I, std::sentinel_for<I> S>
    auto tree_fold(I first, S last) -> diagram_t;

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
    auto evaluate(diagram_t d, Vars const& vs) const -> int32;

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
    template<class Foo = void>
    requires(is_bdd<Degree>)
    auto satisfy_count(diagram_t d) -> second_t<Foo, int64>;

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
    auto satisfy_count(int32 val, diagram_t d) -> int64;

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
    template<out_var_values Vars, class Foo = void>
    requires(is_bdd<Degree>)
    auto satisfy_all(diagram_t d) const -> second_t<Foo, std::vector<Vars>>;

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
     *  assigning unsigned integers. std::vector is also allowed.
     *  \param val Value of the function.
     *  \param d Diagram representing the function.
     *  \return Vector of \p Vars .
     */
    template<out_var_values Vars>
    auto satisfy_all(int32 val, diagram_t d) const -> std::vector<Vars>;

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
     *  assigning unsigned integers. std::vector is also allowed.
     *  \tparam O Output iterator type.
     *  \param val Value of the function.
     *  \param d Diagram representing the function.
     *  \param out Output iterator that is used to output instances
     *  of \p Vars .
     */
    template<out_var_values Vars, std::output_iterator<Vars> O>
    auto satisfy_all_g(int32 val, diagram_t d, O out) const -> void;

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
    auto cofactor(diagram_t d, int32 i, int32 val) -> diagram_t;

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
    auto transform(diagram_t d, F f = utils::not_zero) -> diagram_t;

    /**
     *  \brief Enumerates indices of variables that the function depends on.
     *
     *  \param d Diagram representing the function.
     *  \return Vector of indices.
     */
    auto dependency_set(diagram_t d) const -> std::vector<int32>;

    /**
     *  \brief Enumerates indices of variables that the function depends on.
     *
     *  \tparam O Output iterator type.
     *  \param d Diagram representing the function.
     *  \param out Output iterator that is used to output indices.
     */
    template<std::output_iterator<int32> O>
    auto dependency_set_g(diagram_t d, O out) const -> void;

    /**
     *  \brief Reduces diagrams to its canonical form.
     *
     *  You probably won't need to call this.
     *
     *  \param  d Diagram.
     *  \return Diagram in a reduced canonical form.
     */
    auto reduce(diagram_t) -> diagram_t;

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
    auto node_count() const -> int64;

    /**
     *  \brief Returns number of nodes in the diagram including
     *  terminal nodes.
     *  \param d Diagram.
     *  \return Number of node.
     */
    auto node_count(diagram_t d) const -> int64;

    /**
     *  \brief Prints dot representation of the graph.
     *
     *  Prints dot representation of the entire multi rooted graph to
     *  the output stream.
     *
     *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
     */
    auto to_dot_graph(std::ostream& out) const -> void;

    /**
     *  \brief Prints dot representation of the diagram.
     *
     *  Prints dot representation of the diagram to
     *  the output stream.
     *
     *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
     *  \param d Diagram.
     */
    auto to_dot_graph(std::ostream& out, diagram_t d) const -> void;

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
    auto force_gc() -> void;

    /**
     *  \brief Runs variable reordering heuristic.
     */
    auto force_reorder() -> void;

    /**
     *  \brief Returns number of variables for this manager
     *  set in the constructor.
     *  \return Number of variables.
     */
    auto get_var_count() const -> int32;

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
    auto get_order() const -> std::vector<int32> const&;

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
    auto get_domains() const -> std::vector<int32>;

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
    auto set_cache_ratio(double ratio) -> void;

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
    auto set_gc_ratio(double ratio) -> void;

    /**
     *  \brief Enables or disables automatic variable reordering.
     *
     *  Note that when automatic reordering is enabled the manager
     *  can't guarantee that all diagrams will remain canonical.
     *  To ensure that a diagram \c d is canonical
     *  (e.g. to compare two functions), you need to call \c reduce on them.
     *
     *  \param r Specifies whether to disable (false) or
     *           enable (true) automatic reordering.
     */
    auto set_auto_reorder(bool r) -> void;

protected:
    using node_t = typename diagram<Data, Degree>::node_t;

private:
    // TODO to local lambda
    struct cache_pair_hash_t
    {
        auto operator()(auto const& p) const noexcept
        {
            auto const hash1 = std::hash<node_t*>()(p.first);
            auto const hash2 = std::hash<node_t*>()(p.second);
            auto result      = std::size_t(0);
            result ^= hash1 + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= hash2 + 0x9e3779b9 + (result << 6) + (result >> 2);
            return result;
        }
    };

    struct cache_pair_equal_t
    {
        auto operator()(auto const l, auto const r) const noexcept
        {
            return l.first == r.first && l.second == r.second;
        }
    };

    template<class Map>
    class local_cache_handle
    {
    public:
        local_cache_handle(Map&);
        auto put(node_t*, node_t*, node_t*) -> void;
        auto lookup(node_t*, node_t*) const -> node_t*;

    private:
        Map& map_;
    };

    template<class Op>
    class global_cache_handle
    {
    public:
        global_cache_handle(node_manager<Data, Degree, Domain>&);
        auto put(node_t*, node_t*, node_t*) -> void;
        auto lookup(node_t*, node_t*) const -> node_t*;

    private:
        node_manager<Data, Degree, Domain>& nodes_;
    };

private:
    template<uint_to_uint F>
    auto transform_terminal(node_t*, F) -> node_t*;

    template<any_bin_op Op>
    auto apply_local(node_t*, node_t*, Op) -> diagram_t;

    template<bin_op Op>
    auto apply_global(node_t*, node_t*, Op) -> diagram_t;

    template<any_bin_op Op, cache_handle<node<Data, Degree>> Cache>
    auto apply_detail(node_t*, node_t*, Op, Cache&) -> diagram_t;

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
        domains::mixed ds,
        std::vector<int32> order
    )
    requires(domains::is_mixed<Domain>()());

public:
    diagram_manager(diagram_manager const&)                    = delete;
    diagram_manager(diagram_manager&&)                         = default;
    auto operator=(diagram_manager const&) -> diagram_manager& = delete;
    auto operator=(diagram_manager&&) -> diagram_manager&      = default;

protected:
    node_manager<Data, Degree, Domain> nodes_;
};

namespace detail
{
inline auto default_or_fwd(int64 const n, std::vector<int32>& is)
{
    return is.empty() ? utils::fill_vector(n, utils::identity)
                      : std::vector<int32>(std::move(is));
}
} // namespace detail

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::constant(int32 const v) -> diagram_t
{
    return diagram_t(nodes_.terminal_node(v));
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::variable(int32 const i) -> diagram_t
{
    return diagram_t(nodes_.internal_node(
        i,
        nodes_.make_sons(
            i,
            [this](auto const v)
            {
                return nodes_.terminal_node(v);
            }
        )
    ));
}

template<class Data, degree Degree, domain Domain>
template<class Foo>
requires(is_bdd<Degree>)
auto diagram_manager<Data, Degree, Domain>::variable_not(int32 const i)
    -> second_t<Foo, diagram_t>
{
    return diagram_t(nodes_.internal_node(
        i,
        nodes_.make_sons(
            i,
            [this](auto const v)
            {
                return nodes_.terminal_node(1 - v);
            }
        )
    ));
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::operator()(int32 const i)
    -> diagram_t
{
    return this->variable(i);
}

template<class Data, degree Degree, domain Domain>
template<std::ranges::input_range Is>
auto diagram_manager<Data, Degree, Domain>::variables(Is const& is)
    -> std::vector<diagram_t>
{
    return this->variables(std::ranges::begin(is), std::ranges::end(is));
}

template<class Data, degree Degree, domain Domain>
template<std::convertible_to<int32> T>
auto diagram_manager<Data, Degree, Domain>::variables(
    std::initializer_list<T> const is
) -> std::vector<diagram_t>
{
    return this->variables(std::ranges::begin(is), std::ranges::end(is));
}

template<class Data, degree Degree, domain Domain>
template<std::input_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Data, Degree, Domain>::variables(
    I const first, S const last
) -> std::vector<diagram_t>
{
    static_assert(std::convertible_to<std::iter_value_t<I>, int32>);
    return utils::fmap(
        first,
        last,
        [this](auto const i)
        {
            return this->variable(static_cast<int32>(i));
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
        return diagram_t(nodes_.terminal_node(*first));
    }

    auto const lastLevel = static_cast<int32>(this->get_var_count() - 1);
    auto const lastIndex = nodes_.get_index(lastLevel);

    if constexpr (std::random_access_iterator<I>)
    {
        [[maybe_unused]] auto const count =
            nodes_.domain_product(0, lastLevel + 1);
        [[maybe_unused]] auto const dist =
            static_cast<int64>(std::distance(first, last));
        assert(dist > 0 && dist == count);
    }

    using stack_frame = struct
    {
        node_t* node;
        int32 level;
    };
    auto stack              = std::vector<stack_frame>();
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
            auto count     = 0;
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

            auto newSons = nodes_.make_sons(
                newIndex,
                [&stack, newDomain](auto const o)
                {
                    return stack[as_uindex(ssize(stack) - newDomain + o)].node;
                }
            );
            auto const newNode =
                nodes_.internal_node(newIndex, std::move(newSons));
            stack.erase(std::end(stack) - newDomain, std::end(stack));
            stack.push_back(stack_frame {newNode, currentLevel - 1});
        }
    };

    while (first != last)
    {
        auto sons = nodes_.make_sons(
            lastIndex,
            [this, &first](auto const)
            {
                return nodes_.terminal_node(*first++
                ); // TODO add cast to unsigned
            }
        );
        auto const node = nodes_.internal_node(lastIndex, std::move(sons));
        stack.push_back(stack_frame {node, lastLevel});
        shrink_stack();
    }

    assert(stack.size() == 1);
    return diagram_t(stack.back().node);
}

template<class Data, degree Degree, domain Domain>
template<std::ranges::input_range R>
auto diagram_manager<Data, Degree, Domain>::from_vector(R&& r) -> diagram_t
{
    return this->from_vector(std::ranges::begin(r), std::ranges::end(r));
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::to_vector(diagram_t d) const
    -> std::vector<int32>
{
    auto vs = std::vector<int32>();
    vs.reserve(as_usize(nodes_.domain_product(0, this->get_var_count())));
    this->to_vector_g(d, std::back_inserter(vs));
    return vs;
}

template<class Data, degree Degree, domain Domain>
template<std::output_iterator<teddy::int32> O>
auto diagram_manager<Data, Degree, Domain>::to_vector_g(diagram_t d, O out)
    const -> void
{
    if (this->get_var_count() == 0)
    {
        assert(d.unsafe_get_root()->is_terminal());
        *out++ = d.unsafe_get_root()->get_value();
        return;
    }

    auto vars    = std::vector<int32>(as_usize(this->get_var_count()));
    auto wasLast = false;
    do
    {
        *out++        = this->evaluate(d, vars);

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
    pla_file const& file, fold_type const foldType
) -> second_t<Foo, std::vector<diagram_t>>
{
    auto const product = [this](auto const& cube)
    {
        auto vs = std::vector<diagram_t>();
        vs.reserve(cube.size());
        for (auto i = 0; i < cube.size(); ++i)
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

    auto const& plaLines     = file.get_lines();
    auto const lineCount     = file.line_count();
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
            if (plaLines[as_usize(li)].fVals.get(fi) == 1)
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
template<expression_node Node>
auto diagram_manager<Data, Degree, Domain>::from_expression_tree(
    Node const& root
) -> diagram_t
{
    auto const go = [this](auto& self, auto const& exprNode)
    {
        if (exprNode.is_constant())
        {
            return this->constant(exprNode.get_value());
        }
        else if (exprNode.is_variable())
        {
            return this->variable(exprNode.get_index());
        }
        else
        {
            assert(exprNode.is_operation());
            auto const lhs = self(self, exprNode.get_left());
            auto const rhs = self(self, exprNode.get_right());
            auto const op  = apply_op_wrap(
                [&](auto const l, auto const r)
                {
                    return exprNode.evaluate(l, r);
                }
            );
            return this->apply_local(
                lhs.unsafe_get_root(), rhs.unsafe_get_root(), op
            );
        }
    };
    return go(go, root);
}

template<class Data, degree Degree, domain Domain>
template<bin_op Op>
auto diagram_manager<Data, Degree, Domain>::apply(diagram_t d1, diagram_t d2)
    -> diagram_t
{
    return this->apply_global(d1.unsafe_get_root(), d2.unsafe_get_root(), Op());
}

template<class Data, degree Degree, domain Domain>
template<bin_op Op, std::ranges::input_range R>
auto diagram_manager<Data, Degree, Domain>::left_fold(R const& ds) -> diagram_t
{
    return this->left_fold<Op>(std::ranges::begin(ds), std::ranges::end(ds));
}

template<class Data, degree Degree, domain Domain>
template<bin_op Op, std::input_iterator I, std::sentinel_for<I> S>
auto diagram_manager<Data, Degree, Domain>::left_fold(I first, S const last)
    -> diagram_t
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
auto diagram_manager<Data, Degree, Domain>::tree_fold(R& ds) -> diagram_t
{
    return this->tree_fold<Op>(std::ranges::begin(ds), std::ranges::end(ds));
}

template<class Data, degree Degree, domain Domain>
template<bin_op Op, std::random_access_iterator I, std::sentinel_for<I> S>
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
            *(first + i) =
                this->apply<Op>(*(first + 2 * i), *(first + 2 * i + 1));
        }

        if (justMoveLast)
        {
            *(first + currentCount - 1) =
                std::move(*(first + 2 * (currentCount - 1)));
        }
    }

    return diagram_t(std::move(*first));
}

template<class Data, degree Degree, domain Domain>
template<in_var_values Vars>
auto diagram_manager<Data, Degree, Domain>::evaluate(
    diagram_t d, Vars const& vs
) const -> int32
{
    auto n = d.unsafe_get_root();

    while (not n->is_terminal())
    {
        auto const i = n->get_index();
        assert(nodes_.is_valid_var_value(i, vs[as_uindex(i)]));
        n = n->get_son(vs[as_uindex(i)]);
    }

    return n->get_value();
}

template<class Data, degree Degree, domain Domain>
template<class Foo>
requires(is_bdd<Degree>)
auto diagram_manager<Data, Degree, Domain>::satisfy_count(diagram_t d)
    -> second_t<Foo, int64>
{
    return this->satisfy_count(1, d);
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::satisfy_count(
    int32 const val, diagram_t d
) -> int64
{
    if constexpr (domains::is_fixed<Domain>()())
    {
        assert(val < Domain()());
    }

    auto constexpr CanUseDataMember =
        std::is_floating_point_v<Data> or std::is_integral_v<Data>;
    using T = std::conditional_t<CanUseDataMember, Data, int64>;

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
            return [map = std::unordered_map<node_t*, T>()](auto const n
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
        d.unsafe_get_root(),
        [this, val, &data](auto const n) mutable
        {
            if (n->is_terminal())
            {
                data(n) = n->get_value() == val ? 1 : 0;
            }
            else
            {
                data(n)           = 0;
                auto const nLevel = nodes_.get_level(n);
                nodes_.for_each_son(
                    n,
                    [=, this, &data](auto const son) mutable
                    {
                        auto const sonLevel = nodes_.get_level(son);
                        auto const diff =
                            nodes_.domain_product(nLevel + 1, sonLevel);
                        data(n) += data(son) * static_cast<T>(diff);
                    }
                );
            }
        }
    );

    auto const rootAlpha = static_cast<int64>(data(d.unsafe_get_root()));
    auto const rootLevel = nodes_.get_level(d.unsafe_get_root());
    return rootAlpha * nodes_.domain_product(0, rootLevel);
}

template<class Data, degree Degree, domain Domain>
template<out_var_values Vars, class Foo>
requires(is_bdd<Degree>)
auto diagram_manager<Data, Degree, Domain>::satisfy_all(diagram_t d) const
    -> second_t<Foo, std::vector<Vars>>
{
    return this->satisfy_all<Vars>(d);
}

template<class Data, degree Degree, domain Domain>
template<out_var_values Vars>
auto diagram_manager<Data, Degree, Domain>::satisfy_all(
    int32 const val, diagram_t d
) const -> std::vector<Vars>
{
    auto vs = std::vector<Vars>();
    this->satisfy_all_g<Vars>(val, d, std::back_inserter(vs));
    return vs;
}

template<class Data, degree Degree, domain Domain>
template<out_var_values Vars, std::output_iterator<Vars> O>
auto diagram_manager<Data, Degree, Domain>::satisfy_all_g(
    int32 const val, diagram_t d, O out
) const -> void
{
    if constexpr (domains::is_fixed<Domain>()())
    {
        assert(val < Domain()());
    }

    auto xs = [this]()
    {
        if constexpr (is_std_vector<Vars>)
        {
            return Vars(as_usize(this->get_var_count()));
        }
        else
        {
            return Vars {};
        }
    }();
    auto go =
        [this, &xs, val, out](auto& self, auto const l, auto const n) mutable
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
            for (auto iv = 0; iv < domain; ++iv)
            {
                xs[as_uindex(index)] = iv;
                self(self, l + 1, n);
            }
        }
        else
        {
            auto const index = n->get_index();
            nodes_.for_each_son(
                n,
                [=, &xs, iv = int32 {0}](auto const son) mutable
                {
                    xs[as_uindex(index)] = iv;
                    self(self, l + 1, son);
                    ++iv;
                }
            );
        }
    };

    go(go, int32 {0}, d.unsafe_get_root());
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::cofactor(
    diagram_t d, int32 const i, int32 const v
) -> diagram_t
{
    if (d.unsafe_get_root()->is_terminal())
    {
        return d;
    }

    auto const root = d.unsafe_get_root();
    if (root->get_index() == i)
    {
        return diagram_t(root->get_son(v));
    }

    auto memo     = std::unordered_map<node_t*, node_t*>();
    auto const go = [this, &memo, i, v](auto const& self, auto const n)
    {
        auto const memoIt = memo.find(n);
        if (memoIt != std::end(memo))
        {
            return memoIt->second;
        }

        if (n->is_terminal())
        {
            return n;
        }

        auto sons       = n->get_index() == i
                              ? nodes_.make_sons(
                              i,
                              [son = n->get_son(v)](auto const)
                              {
                                  return son;
                              }
                          )
                              : nodes_.make_sons(
                              n->get_index(),
                              [n, &self](auto const k)
                              {
                                  return self(self, n->get_son(k));
                              }
                          );

        auto const newN = nodes_.internal_node(n->get_index(), std::move(sons));
        memo.emplace(n, newN);
        return newN;
    };

    auto const newRoot = go(go, root);
    return diagram_t(newRoot);
}

template<class Data, degree Degree, domain Domain>
template<uint_to_bool F>
auto diagram_manager<Data, Degree, Domain>::transform(diagram_t d, F f)
    -> diagram_t
{
    auto const r = this->transform_terminal(d.unsafe_get_root(), f);
    nodes_.run_deferred();
    return diagram_t(r);
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::dependency_set(diagram_t d) const
    -> std::vector<int32>
{
    auto is = std::vector<int32>();
    is.reserve(this->get_var_count());
    this->dependency_set_g(d, std::back_inserter(is));
    is.shrink_to_fit();
    return is;
}

template<class Data, degree Degree, domain Domain>
template<std::output_iterator<int32> O>
auto diagram_manager<Data, Degree, Domain>::dependency_set_g(diagram_t d, O out)
    const -> void
{
    auto memo = std::vector<bool>(this->get_var_count(), false);
    nodes_.traverse_pre(
        d.unsafe_get_root(),
        [&memo, out](auto const n) mutable
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
        }
    );
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::reduce(diagram_t d) -> diagram_t
{
    auto const newRoot =
        this->transform_terminal(d.unsafe_get_root(), utils::identity);
    return diagram_t(newRoot);
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::node_count() const -> int64
{
    return nodes_.get_node_count();
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::node_count(diagram_t d) const
    -> int64
{
    return nodes_.get_node_count(d.unsafe_get_root());
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::to_dot_graph(std::ostream& ost
) const -> void
{
    nodes_.to_dot_graph(ost);
}

template<class Data, degree Degree, domain Domain>
auto diagram_manager<Data, Degree, Domain>::to_dot_graph(
    std::ostream& ost, diagram_t d
) const -> void
{
    nodes_.to_dot_graph(ost, d.unsafe_get_root());
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
auto diagram_manager<Data, Degree, Domain>::set_auto_reorder(bool const r)
    -> void
{
    nodes_.set_auto_reorder(r);
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
template<uint_to_uint F>
auto diagram_manager<Data, Degree, Domain>::transform_terminal(
    node_t* const root, F f
) -> node_t*
{
    auto memo     = std::unordered_map<node_t*, node_t*>();
    auto const go = [this, f, &memo](auto const& self, auto const n)
    {
        auto const it = memo.find(n);
        if (memo.end() != it)
        {
            return it->second;
        }

        if (n->is_terminal())
        {
            auto const newVal = static_cast<int32>(f(n->get_value()));
            return nodes_.terminal_node(newVal);
        }
        else
        {
            auto const i       = n->get_index();
            auto const newNode = nodes_.internal_node(
                i,
                nodes_.make_sons(
                    i,
                    [&self, n](auto const k)
                    {
                        return self(self, n->get_son(k));
                    }
                )
            );
            memo.emplace(n, newNode);
            return newNode;
        }
    };
    return go(go, root);
}

template<class Data, degree Degree, domain Domain>
template<any_bin_op Op>
auto diagram_manager<Data, Degree, Domain>::apply_local(
    node_t* const lhs, node_t* const rhs, Op const op
) -> diagram_t
{
    auto cache = std::unordered_map<
        std::pair<node_t*, node_t*>,
        node_t*,
        cache_pair_hash_t,
        cache_pair_equal_t>();

    auto cacheHandle = local_cache_handle<decltype(cache)>(cache);

    return this->apply_detail(lhs, rhs, op, cacheHandle);
}

template<class Data, degree Degree, domain Domain>
template<bin_op Op>
auto diagram_manager<Data, Degree, Domain>::apply_global(
    node_t* const l, node_t* const r, Op const op
) -> diagram_t
{
    auto cacheHandle = global_cache_handle<Op>(nodes_);
    return this->apply_detail(l, r, op, cacheHandle);
}

template<class Data, degree Degree, domain Domain>
template<any_bin_op Op, cache_handle<node<Data, Degree>> Cache>
auto diagram_manager<Data, Degree, Domain>::apply_detail(
    node_t* const lhs, node_t* const rhs, Op const op, Cache& cache
) -> diagram_t
{
    auto const go = [&, this](auto const self, auto const l, auto const r)
    {
        auto const cached = cache.lookup(l, r);
        if (cached)
        {
            return cached;
        }

        auto const lhsVal = node_value(l);
        auto const rhsVal = node_value(r);
        auto const opVal  = op(lhsVal, rhsVal);
        auto u            = static_cast<node_t*>(nullptr);

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
            auto sons           = nodes_.make_sons(
                topIndex,
                [=](auto const k)
                {
                    auto const fst = lhsLevel == topLevel ? l->get_son(k) : l;
                    auto const snd = rhsLevel == topLevel ? r->get_son(k) : r;
                    return self(self, fst, snd);
                }
            );

            u = nodes_.internal_node(topIndex, std::move(sons));
        }

        cache.put(l, r, u);
        return u;
    };

    auto const r = go(go, lhs, rhs);
    auto const d = diagram_t(r);
    nodes_.run_deferred();
    return d;
}

template<class Data, degree Degree, domain Domain>
diagram_manager<Data, Degree, Domain>::diagram_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order
)
requires(domains::is_fixed<Domain>()())
    : nodes_(
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
    domains::mixed ds,
    std::vector<int32> order
)
requires(domains::is_mixed<Domain>()())
    : nodes_(
          varCount,
          nodePoolSize,
          overflowNodePoolSize,
          detail::default_or_fwd(varCount, order),
          std::move(ds)
      )
{
}

template<class Data, degree Degree, domain Domain>
template<class Map>
diagram_manager<Data, Degree, Domain>::local_cache_handle<
    Map>::local_cache_handle(Map& m)
    : map_(m)
{
}

template<class Data, degree Degree, domain Domain>
template<class Map>
auto diagram_manager<Data, Degree, Domain>::local_cache_handle<Map>::put(
    node_t* const l, node_t* const r, node_t* const u
) -> void
{
    map_.emplace(
        std::piecewise_construct, std::make_tuple(l, r), std::make_tuple(u)
    );
}

template<class Data, degree Degree, domain Domain>
template<class Map>
auto diagram_manager<Data, Degree, Domain>::local_cache_handle<Map>::lookup(
    node_t* const l, node_t* const r
) const -> node_t*
{
    auto const it = map_.find(std::make_pair(l, r));
    return it != end(map_) ? it->second : nullptr;
}

template<class Data, degree Degree, domain Domain>
template<class Op>
diagram_manager<Data, Degree, Domain>::global_cache_handle<
    Op>::global_cache_handle(node_manager<Data, Degree, Domain>& ns)
    : nodes_(ns)
{
}

template<class Data, degree Degree, domain Domain>
template<class Op>
auto diagram_manager<Data, Degree, Domain>::global_cache_handle<Op>::put(
    node_t* const l, node_t* const r, node_t* const u
) -> void
{
    nodes_.template cache_put<Op>(l, r, u);
}

template<class Data, degree Degree, domain Domain>
template<class Op>
auto diagram_manager<Data, Degree, Domain>::global_cache_handle<Op>::lookup(
    node_t* const l, node_t* const r
) const -> node_t*
{
    return nodes_.template cache_find<Op>(l, r);
}
} // namespace teddy

#endif