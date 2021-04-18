# 🧸 TeDDy

C++ library for creation and manipulation of decision diagrams. It is being developed at the [Faculty of Management Science and Informatics](https://www.fri.uniza.sk/en/), [University of Žilina](https://www.uniza.sk/index.php/en/) as a student project at the [Department of Informatics](https://ki.fri.uniza.sk/).

## Decision diagrams
Our library supports [Binary Decision Diagrams](https://en.wikipedia.org/wiki/Binary_decision_diagram) (BDD) as described by [Bryant](https://ieeexplore.ieee.org/document/1676819) and their generalization for multiple-valued functions. Those diagrams are usually referred to as Multivalued Decisionn Diagrams (MDD).

## How to install
Library is header only so its easy to incorporate it in your project. All you need to do is to place contents of [this](./src/lib/) directory into your project files.  
We use features from `C++20` so you might need to set your compiler for this version. (`-std=c++20` for `clang++` and `g++`, `/std:c++latest` for MSVC)

## Basic usage
 Before using any library functions you need to include either [bdd_manager.hpp](./src/lib/bdd_manager.hpp) or [mdd_manager.hpp](./src/lib/mdd_manager.hpp) depending on which diagrams you intent to use. Library API is accessed via instance of the manager. Manager can be created using `make_bdd_manager`/`make_mdd_manager` factory function, that takes number of variables you want to work with as an argument. Public interfaces of those managers contain full documentation of library API.  

### Binary Decision Diagrams
```C++
#include "bdd_manager.hpp"

int main()
{
    using namespace teddy;

    // We will use variables x0, x1, x2, x3, x4.
    auto m = make_bdd_manager(5);

    // Code from following examples goes here.
}
```

#### Creating diagrams

##### Diagram of a constant
```C++
auto cFalse = m.constant(0);
auto cTrue  = m.constant(1);
```

##### Diagram of a variable
```C++
auto x0 = m.variable(0);
auto x4 = m.variable(4);
```
Useful trick for creating diagram for a variable is to create a reference to the manager with name `x`. Manager defines an `operator()` that does the same thing as `variable`.
```C++
auto& x = m;
auto x1 = x(1);
```
If you want to create complemented (negated) variable you can use `variable_not` or `operator()` with not flag.
```C++
auto x2_ = m.variable_not(2);
auto x3_ = x(3, NOT());
```
You can also complement (negate) an existing diagram using function `negate`.
```C++
auto x1_ = m.negate(x1);
```

##### Diagram of a function of more variables
In order to create a diagram for more complex function e.g. `(x0 and x1) or ((x2 and x3) or x5)` you need to combine diagrams of individual variables using function template `apply`.
```C++
auto f = m.apply<OR>( m.apply<AND>(x0, x1)
                    , m.apply<OR>( m.apply<OR>(x(2), x(3))
                                 , x4 ) );
```
Expressions like this one look a bit complicated. That is why we overloaded corresponding operators to use `apply` for you in the background. Before using any overloaded operator you must register the manager you are using.  
This feature is intended only for testing and playing around. For a real world usage, we recommend you to stick with calling `apply` directly.
```C++
register_manager(m);
auto g = (x(0) && x(1)) || ((x(2) && x(3)) || x(4));
```
Basic operations that can be used in apply are: `AND, OR, XOR, NAND, NOR`. Full list of operations with corresponding overloads can be found in table below.

#### Using diagrams
Since a diagram is representation of a function you can evaluate it for given assignment of variables. Values of variables can be specified in different ways. `evaluate` returns either `0` (false) or `1` (true).
```C++
auto const val0 = m.evaluate(f, std::array  {0u, 1u, 1u, 0u, 1u});
auto const val1 = m.evaluate(f, std::vector {0u, 1u, 1u, 0u, 1u});
auto const val2 = m.evaluate(f, std::vector {false, true, true, false, true});
auto const val3 = m.evaluate(f, std::bitset<5> {0b10110});
auto const val4 = m.evaluate(f, 0b10110);
```
Function `satisfy_count` computes number of all different variable assignments for which the function evaluates to `1` (true). If you divide this number by the number of all possible variable assignments you get so called truth density of a Boolean function, which can also be calculated directly by using the `truth_density` function.
```C++
auto const sc = m.satisfy_count(f);
auto const td = m.truth_density(f);
```
Variable assignments for which the function `f` evaluates to `1` can also be enumerated using the function `satisfy_all`. In the example below we first declare an alias for the type that will hold values of variables. The most suitable type for Boolean variables is `std::bitset` (with N = 5, since we have 5 variables). `satisfy_all` then returns `std::vector` of `var_vals_t`.
```C++
using var_vals_t = std::bitset<5>;
auto const satisfyingSet = m.satisfy_all<var_vals_t>(f);
```
If you only want to print these variable assignments, you can use the generic version of this function called `satisfy_all_g`. This function takes an [output iterator](https://en.cppreference.com/w/cpp/named_req/OutputIterator) instead of returning a container. `std::ostream_iterator` is really elegant here since there is an overload of `operator<<` for `std::bitset`.
```C++
m.satisfy_all_g<var_vals_t>(f, std::ostream_iterator<var_vals_t>(std::cout, "\n"));
```
Diagrams can be tested for equality and inequality. From the above examples, it is obvious that `f` and `g` represent the same function since we used the same logical expression to construct them. In general, you can use diagrams to check whether two logical expressions represent the same function using diagrams member function `equals`. Notice that we used [alternative tokens](https://en.cppreference.com/w/cpp/language/operator_alternative) in this example. Also notice that you can use `operator!` (or its alternative token `not`) to negate a diagram after you registered a manager.
```C++
auto f1 = x(1) xor x(2);
auto f2 = (x(1) or x(2)) and (!x(1) or not x(2));
assert(f1.equals(f2));
```
Last but not least, you might want to check how the diagram looks like. For that purpose we use the [DOT language](https://en.wikipedia.org/wiki/DOT_(graph_description_language)). The function `to_dot_graph` prints a DOT representation of given diagram into given output stream (`std::cout` in the example below, you might also consider `std::ofstream` or `std::ostringstream`). DOT string can be visualized by different tools listed in the [Wikipedia](https://en.wikipedia.org/wiki/DOT_(graph_description_language)) page, the fastest ways is to use [Webgraphviz](http://www.webgraphviz.com/).
```C++
m.to_dot_graph(std::cout, f);
```

### Multi-valued decision diagrams
Using MDDs is almost identical to using BDDs. You just need to use `make_mdd_manager` which has a template parameter `P` that specifies how many logical levels you want to use. Here is an example of 4-valued logic function of 4 variables:
```C++
auto m  = make_mdd_manager<4>(4);
auto& x = m;

// f = (x0 * x1 + x2 * x3) mod 4
auto f = m.apply<PLUS>( m.apply<MULTIPLIES>(x(0), x(1))
                      , m.apply<MULTIPLIES>(x(2), x(3)) );

auto const val0 = m.evaluate(f, std::array  {0u, 1u, 2u, 3u});
auto const val1 = m.evaluate(f, std::vector {0u, 1u, 2u, 3u});
auto const sc2  = m.satisfy_count(2, f);

using var_vals_t = std::array<unsigned int, 4>;
auto sas = m.satisfy_all<var_vals_t>(2, f);

assert(val0 == val1);
assert(sc2 == sas.size());
```
As you can see the API is almost identical to the `bdd_manager`, however some functions need additional parameter that specifies logical level.  

### In conclusion
Above examples showed a basic functionality of the library. For further details see public interfaces of [bdd_manager](./src/lib/bdd_manager.hpp) and [mdd_manager](./src/lib/mdd_manager.hpp). From the implementation point of view, there is no difference between BDD and MDD. `bdd_manager` only exists because BDDs are more common and declarations of some functions can be simplified when `P = 2`.

### Operations supported by `apply`
| Binary operator | Description                     | Operator overload | Note                                  |
|:---------------:|---------------------------------|:-----------------:|---------------------------------------|
|       AND       | Logical and.                    |         &&        | 0 is false, everything else is true.  |
|        OR       | Logical or.                     |        \|\|       | 0 is false, everything else is true.  |
|       XOR       | Logical xor.                    |         ^         | 0 is false, everything else is true.  |
|       NAND      | Logical nand.                   |                   | 0 is false, everything else is true.  |
|       NOR       | Logical nor.                    |                   | 0 is false, everything else is true.  |
|     EQUAL_TO    | Equal to relation.              |         ==        | Result is 1 for true and 0 for false. |
|   NOT_EQUAL_TO  | Not equal to relation.          |         !=        | Result is 1 for true and 0 for false. |
|       LESS      | Less than relation              |         <         | Result is 1 for true and 0 for false. |
|    LESS_EQUAL   | Less than or equal relation.    |         <=        | Result is 1 for true and 0 for false. |
|     GREATER     | Greater than relation.          |         >         | Result is 1 for true and 0 for false. |
|  GREATER_EQUAL  | Greater than or equal relation. |         >=        | Result is 1 for true and 0 for false. |
|       MIN       | Minimum of two values.          |                   |                                       |
|       MAX       | Maximum of two values.          |                   |                                       |
|       PLUS      | Modular addition.               |         +         | (a + b) mod P                         |
|    MULTIPLIES   | Modular multiplication.         |         *         | (a * b) mod P                         |

## Reliability Theory
Main motivation for creating this library was the possibility of using decision diagrams in reliability analysis. Following example shows how different reliability indices and importance measures can be calculated using structure function and Direct Partial Boolean Derivatives.  

### Binary state systems
First, we create a manager for 5 variables and we register it so that we can use operator overloading in this example.
```C++
auto  m = make_bdd_manager(5);
auto& x = m;
register_manager(m);
```
Besides structure function we also need probabilities associated with variables.
```C++
auto const ps = std::vector {0.9, 0.8, 0.7, 0.9, 0.9};
auto sf = x(0) and x(1) or (x(2) and x(3) or x(4));
```
Availability can be calculated directly from the structure function. Unavailability is just ones complement.
```C++
auto const A = m.availability(ps, sf);
auto const U = 1 - A;
```
For further analysis of the structure function we need to calculate Direct Partial Boolean Derivatives, which is as simple as:
```C++
auto dpbds = m.dpbds(sf);
```
`dpbds` is now a `std::vector` of diagrams representing derivative of each variable. We can use them to calculate different importance measures:
```C++
auto const SIs = m.structural_importances(dpbds);
auto const BIs = m.birnbaum_importances(ps, dpbds);
auto const CIs = m.criticality_importances(BIs, ps, U);
auto const FIs = m.fussell_vesely_importances(dpbds, ps, U);
```
Each `*Is` is a `std::vector` of `double`s representing given importance measures for given variables.  
Finally we can enumerate all Minimal Cut Vectors. Like with `satisfy_all` we need to specify a data type which will hold values of variables. Again `std::bitset` seems to be the best choice.
```C++
auto const MCVs = m.mcvs<std::bitset<5>>(dpbds);
```
`MCVs` is now a `std::vector` of `std::bitset`s which represents MCVs.