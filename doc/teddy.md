# Constructors
```C++
bdd_manager ( std::size_t varCount
            , std::size_t initNodeCount
            , std::vector<index_t> order = default_oder() );

mdd_manager ( std::size_t varCount
            , std::size_t initNodeCount
            , std::vector<index_t> order = default_oder() );

imdd_manager ( std::size_t varCount
             , std::size_t initNodeCount
             , std::vector<uint_t> domains
             , std::vector<index_t> order = default_oder() );

ifmdd_manager ( std::size_t varCount
              , std::size_t initNodeCount
              , std::vector<uint_t> domains
              , std::vector<index_t> order = default_oder() )
```
|                   |                                                                                                                                                   |
|-------------------|---------------------------------------------------------------------------------------------------------------------------------------------------|
| **varCount**      | number of variables that you want to work with.                                                                                                   |
| **initNodeCount** | initial count of pre-allocated nodes.                                                                                                             |
| **domains**       | domains of variables                                                                                                                              |
| **order**         | initial order of variables. Variables are order by their indices by default. Order stays the same unless it is specifically asked for reordering. |

```C++
// example:
std::vector<teddy::index_t> bddOrder {5, 6, 7, 8, 9, 1, 2, 3, 4, 0};
teddy::bdd_manager manager(10, 10'000, bddOrder);

std::vector<teddy::index_t> imddOrder {1, 2, 3, 4, 0};
std::vector<teddy::uint_t> imddDomains {3, 3, 2, 2};
teddy::imdd_manager manager(4, 5'000, imddOrder, imddDomains);

// it is convenient to use initializer lists for smaller variable counts:
teddy::mdd_manager<3> manager(4, 5'000, {1, 2, 3, 4, 0});
teddy::ifmdd_manager<3> manager(4, 5'000, {1, 2, 3, 4, 0}, {3, 3, 2, 2});
```

# Common interface
This interface is common for all managers.

### constant
```C++
auto constant (uint_t v) -> diagram_t;
```
Creates diagram representing constant function with value `v`.

### variable / operator()
```C++
auto variable (index_t i) -> diagram_t;
auto operator() (index_t i) -> diagram_t;
```
Creates diagram representing function of single variable with index `i`.

### variables
```C++
// 1.
template<std::convertible_to<index_t> T>
auto variables (std::initializer_list<T> is) -> std::vector<diagram_t>;

// 2.
template<std::input_iterator I, std::sentinel_for<I> S>
auto variables (I first, S last) -> std::vector<diagram_t>;

// 3.
template<std::ranges::input_range Is>
auto variables (Is const& is) -> std::vector<diagram_t>;
```
Creates vector of diagrams representing variables with indices given by range `is` or `[first, last)`.
```C++
// examples:
std::vector<diagram_t> vars = manager.variables({0, 1, 2, 3}); // 1.
std::vector<index_t> is {0, 1, 2, 3};
std::vector<diagram_t> vars = manager.variables(is); // 2.
std::vector<diagram_t> vars = manager.variables(is.begin(), is.end()); // 3.
```

### from_vector
```C++
template<std::input_iterator I, std::sentinel_for<I> S>
auto from_vector (I first, S last) -> diagram_t;

template<std::ranges::input_range R>
auto from_vector (R&& range) -> diagram_t;
```
Creates diagram from a truth vector of a function. Vector can be given by `range` or by `[first, last)` of ints. Example below shows how to use this function to create diagram representing function `f(x) = max(x0, x1, x2)` with truth table shown below. Notice that the truth vector is the last column of the table and also notice the order of columns which must match order of variables specified in the managers constructor.
| x0 | x1 | x2 |   f   |  | x0 | x1 | x2 |   f   |
|:--:|:--:|:--:|:-----:|--|:--:|:--:|:--:|:-----:|
| 0  | 0  | 0  | **0** |  | 1  | 0  | 0  | **1** |
| 0  | 0  | 1  | **1** |  | 1  | 0  | 1  | **1** |
| 0  | 0  | 2  | **2** |  | 1  | 0  | 2  | **2** |
| 0  | 1  | 0  | **1** |  | 1  | 1  | 0  | **1** |
| 0  | 1  | 1  | **1** |  | 1  | 1  | 1  | **1** |
| 0  | 1  | 2  | **2** |  | 1  | 1  | 2  | **2** |
```C++
// example:
teddy::ifmdd_manager<3> manager(3, 100, {2, 2, 3});
std::vector<unsigned int> vec {0, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2};
diagram_t f = manager.from_vector(vec);
```

### apply
```C++
template<bin_op Op>
auto apply (diagram_t const& lhs, diagram_t const& rhs) -> diagram_t;
```
Merges two diagrams `lhs` and `rhs` using binary operation specified by template parameter `Op`. Following binary operations can be used.
| Binary operator | Description                     | Note                                  |
|:---------------:|---------------------------------|---------------------------------------|
|       AND       | Logical and.                    | 0 is false, everything else is true.  |
|        OR       | Logical or.                     | 0 is false, everything else is true.  |
|       XOR       | Logical xor.                    | 0 is false, everything else is true.  |
|       NAND      | Logical nand.                   | 0 is false, everything else is true.  |
|       NOR       | Logical nor.                    | 0 is false, everything else is true.  |
|     EQUAL_TO    | Equal to relation.              | Result is 1 for true and 0 for false. |
|   NOT_EQUAL_TO  | Not equal to relation.          | Result is 1 for true and 0 for false. |
|       LESS      | Less than relation              | Result is 1 for true and 0 for false. |
|    LESS_EQUAL   | Less than or equal relation.    | Result is 1 for true and 0 for false. |
|     GREATER     | Greater than relation.          | Result is 1 for true and 0 for false. |
|  GREATER_EQUAL  | Greater than or equal relation. | Result is 1 for true and 0 for false. |
|       MIN       | Minimum of two values.          |                                       |
|       MAX       | Maximum of two values.          |                                       |
|       PLUS      | Modular addition.               | (a + b) mod P                         |
|    MULTIPLIES   | Modular multiplication.         | (a * b) mod P                         |
```C++
// example:
teddy::mdd_manager<4> manager(3, 100);
auto& x = manager;

// f(x) = min((x0 + x1) mod 4, x2)
using namespace teddy::ops;
diagram_t f = manager.apply<MIN>(manager.apply<PLUS<4>>(x(0), x(1)), x(2));
```

### left_fold, tree_fold
```C++
template<bin_op Op, std::ranges::input_range R>
auto left_fold (R const& range) -> diagram_t;

template< bin_op               Op
        , std::input_iterator  I
        , std::sentinel_for<I> S >
auto left_fold (I first, S last) -> diagram_t;

template<bin_op Op, std::ranges::random_access_range R>
auto tree_fold (R& range) -> diagram_t;

template< bin_op                      Op
        , std::random_access_iterator I
        , std::sentinel_for<I>        S >
auto tree_fold (I first, S last) -> diagram_t;
```
Uses `apply` to merge all diagrams given by `range` or `[first, last)` using binary operation specified by template parameter `Op`. `left_fold` operates from left to right and `tree_fold` merges diagrams using a "tree order" of operations. See Wikipedia on [Fold](https://en.wikipedia.org/wiki/Fold_(higher-order_function)) for further details. `tree_fold` uses the input range to store intermediate results (notice the non-const reference). Hence, the range will be left in valid but unspecified state.
```C++
using namespace teddy::ops;
teddy::bdd_manager manager(10, 100);
std::vector<diagram_t> vars = manager.variables({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

// or using std::ranges:
std::vector<diagram_t> vars = manager.variables(std::ranges::views::iota(0, 10));

diagram_t product = manager.left_fold<AND>(vars);
diagram_t sum = manager.tree_fold<OR>(vars);
```
### evaluate
```C++
template<in_var_values Vars>
auto evaluate (diagram_t const& f, Vars const& vars) const -> uint_t;
```
Evaluates function represented by `f` using values of variables provided in `vars`. Vars must provide `operator[]` that returns variable value as int. See [Concepts](#concepts) section for more details.
```C++
teddy::mdd_manager<4> manager(3, 100);
auto& x = manager;

// f(x) = min((x0 + x1) mod 4, x2)
using namespace teddy::ops;
diagram_t f = manager.apply<MIN>(manager.apply<PLUS<4>>(x(0), x(1)), x(2));
unsigned int val = manager.evaluate(f, std::array {3, 3, 1});
```

### satisfy_count
```C++
auto satisfy_count (uint_t j, diagram_t& f) -> std::size_t;
```
Returns number of different variable assignments for which the function represented by `f` evaluates to `j`. Results is limited by the range of `std::size_t`. Big numbers will be supported in the future.


### satisfy_all
```C++
// 1.
template<out_var_values Vars>
auto satisfy_all (uint_t j, diagram_t const& f) const -> std::vector<Vars>;

// 2.
template< out_var_values             Vars
        , std::output_iterator<Vars> Out >
auto satisfy_all_g (uint_t j, diagram_t const& f, Out out) const -> void;
```
Enumerates all variable assignments for which the function represented by `f` evaluates to `j`. Variables are stored in a container specified by template parameter `Vars`. This container must provide `operator[]` that can be used to assign integer values to the index of a variable. See [Concepts](#concepts) section for more details.  
`1.` enumerates variable assignments in a vector that is returned.  
`2.` uses an output iterator that can be used to output variable assignments into container of choice, file, output stream, ...
```C++
// example:
teddy::ifmdd_manager<3> manager(3, 100, {2, 2, 3});
std::vector<unsigned int> vec {0, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2};
diagram_t f = manager.from_vector(vec);

std::vector<std::array<unsigned int, 3>> vars
    = manager.satisfy_all<std::array<unsigned int, 3>>(2, f); // 1.
assert(vars.size() == 4);

manager.satisfy_all<std::array<unsigned int, 3>>(
    2,
    f,
    std::ostream_iterator<std::array<unsigned int, 3>>(std::cout, "\n")
); // 2.
```

### cofactor
```C++
auto cofactor (diagram_t const& f, index_t i, uint_t j) -> diagram_t;
```
Returns diagram representing cofactor of function represented by `f`. It fixes value of variable with index `i` to value `j`.

### booleanize
```C++
template<uint_to_bool F>
auto booleanize (diagram_t const& f, F map = utils::not_zero) -> diagram_t;
```
Returns diagram representing transformation of values of a function represented by `f` to boolean values using mapping function `map`. By default it maps 0 to 0 and everything else to 1. See [Concepts](#concepts) section for more details.
```C++
// example:
diagram_t bf = manager.booleanize(f, [](unsigned int j)
{
    return j == 2;
});
```

### dependency_set
```C++
auto dependency_set (diagram_t const& f) const -> std::vector<index_t>; // 1.

template<std::output_iterator<index_t> O>
auto dependency_set_g (diagram_t const& f, O out) const -> void; // 2.
```
Enumerates indices of all variables that the function represented by `f` depends on.  
`1.` enumerates indices into vector that is returned from the function.  
`2.` uses an output iterator that can be used to output variable indices into container of choice, file, output stream, ...

### node_count
```C++
auto node_count () const -> std::size_t; // 1.
auto node_count (diagram_t const& f) const -> std::size_t; // 2.
```
`1.` Returns number of nodes that are currently used by the manager. This number does not include number of nodes that are allocated but are not in unique tables. See implementation for details.  
`2.` Returns number of nodes in diagram `f`.

### to_dot_graph
```C++
auto to_dot_graph (std::ostream& out) const -> void; // 1.

auto to_dot_graph (std::ostream& out, diagram_t const& f) const -> void; // 2.
```
Prints [dot](https://en.wikipedia.org/wiki/DOT_(graph_description_language)) representation of the diagram into output stream `out`.  
`1.` prints representation of all diagrams (multi-rooted oriented acyclic graph).  
`2.` prints representation of diagram `f`.

# Concepts