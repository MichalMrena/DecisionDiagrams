*We don't have a formal name yet so there is no cool heading here.*

C++ library for creation and manipulation of decision diagrams. It is being developed at the [Faculty of Management Science and Informatics](https://www.fri.uniza.sk/en/), [University of Žilina](https://www.uniza.sk/index.php/en/) as a student project at the [Department of Informatics](https://ki.fri.uniza.sk/).

## How to install
Library is header only so its easy to incorporate it in your project. All you need to do is to place contents of [this](./src/lib/) directory into your project files.  
We use features from `C++20` so you might need to set your compiler for this version. (`-std=c++20` for `clang++` and `g++`, `/std:c++latest` for MSVC)

## Basic usage
 Before using any library functions you need to include either [bdd_manager.hpp](./src/lib/bdd_manager.hpp) or [mdd_manager.hpp](./src/lib/mdd_manager.hpp) depending on which diagrams you intent to use. Library API is accessed via instance of a manager. Manager can be created using `make_bdd_manager`/`make_mdd_manager` function, that takes number of variables that you intent to use as an argument.  

### Binary Decision Diagrams
```C++
#include "bdd_manager.hpp"

int main()
{
    using namespace mix::dd;

    auto m = make_bdd_manager(5);

    // code from following examples goes here
}
```

#### Creating diagrams
Simplest diagram that you can create is a diagram of a Boolean constant `0` (false) or `1` (true).
```C++
auto cFalse = m.just_val(0);
auto cTrue  = m.just_val(1);
```
and a diagram of a single Boolean variable.
```C++
auto x0 = m.just_var(0);
auto x4 = m.just_var(4);
```
Useful trick for creating diagram for a variable is to create a reference to the manager with name `x`. Manager defines an `operator()` that does the same thing as `just_var`.
```C++
auto& x = m;
auto x1 = x(1);
```
In order to create complemented (negated) variable you can use `just_var_not` or `operator()` with not flag.
```C++
auto x2_ = m.just_var_not(2);
auto x3_ = x(3, NOT());
```
You can also complement (negate) an existing diagram using function `negate`.
```C++
auto x1_ = m.negate(x1);
```

Lets say you want to create a diagram for a more complex function `(x0 * x1) + (x2 * x3 + x5)` where `*` denotes logical conjunction (and) and `+` denotes logical disjunction (or). What we can do is to merge diagrams of individual variables using function `apply`. Apply takes two diagrams and a binary operation and returns a result of applying given operation on those diagrams.
```C++
auto f = m.apply( m.apply(x0, AND(), x1)
                , OR()
                , m.apply( m.apply(x(2), AND(), x(3))
                         , OR()
                         , x4 ) );
```
Following operations can be used in apply: `AND(), OR(), XOR(), NAND(), NOR()`. There are a few more which we will cover later.

Even with sophisticated formatting and indentation, expressions like this might seem a bit complicated. That is why we overloaded corresponding operators to use `apply` for us in the background. Before using any overloaded operator you need to register the manager you are using so that operators know which one to use. This feature is intended only for testing and playing around. For a real world usage, please stick with calling `apply` directly.
```C++
register_manager(m);
auto g = x(0) * x(1) + (x(2) * x(3) + x(4));
```

#### Using diagrams
Now that we have the diagram of the function we can do something with it. Since the diagram is basically a function we can evaluate it for given values of variables. As you can see, these values can be specified in different ways. It is up to you which one you pick. `get_value` returns either `0` (false) or `1` (true).
```C++
auto const val0 = m.get_value(f, std::array  {0, 1, 1, 0, 1});
auto const val1 = m.get_value(f, std::vector {0, 1, 1, 0, 1});
auto const val2 = m.get_value(f, std::vector {false, true, true, false, true});
auto const val3 = m.get_value(f, std::bitset<5> {0b10110});
auto const val4 = m.get_value(f, 0b10110);
```
In some applications you want to know, for how many different variable assignments the function evaluates to `1` (true). Functions that computes this value is called `satisfy_count`. If we divide this number by number of all possible variable assignments we get so called truth density which can be calculated by *wait for it* `truth_density` function.
```C++
auto const sc = m.satisfy_count(f);
auto const td = m.truth_density(f);
```
Variable values for which the function evaluates to `1` can also be enumerated using the function `satisfy_all` and an output iterator. In the example below we first declare an alias for a type that will hold values of variables. The most suitable type for Boolean variables is `std::bitset` (with N = 5, since we have 5 variables). Then we need some container that will hold these values. `std::vector` seems to be the best choice. Finally we need an output iterator for that container, we can use `std::back_inserter` for that.
```C++
using bool_v = std::bitset<5>;

auto vars = std::vector<bool_v>();
m.satisfy_all<bool_v>(f, std::back_inserter(vars));
```
Vector `vars` now holds all values of variables for which the function evaluates to `1`. We can verify that with following asserts:
```C++
for (auto const& v : vars)
{
    assert(1 == m.get_value(f, v));
}
```
If we only want to print these variable values, we can use `std::ostream_iterator` since there is an overload of `operator<<` for `std::bitset`.
```C++
m.satisfy_all<bool_v>(f, std::ostream_iterator<bool_v>(std::cout, "\n"));
```
Diagrams can be compared for equality and inequality. From above examples, it is obvious that `f` and `g` represent the same function since we used the same logical expression to construct them. In general, you can use diagrams to check whether two logical expressions represent the same function using `operator==`. Notice that we used [alternative tokens](https://en.cppreference.com/w/cpp/language/operator_alternative) in this example which gives it quite an elegant look. Also notice that you can use `operator!` (or its alternative token `not`) to negate a diagram after you registered diagram.
```C++
auto f1 = x(1) xor x(2);
auto f2 = (x(1) or x(2)) and (!x(1) or not x(2));
assert(f1 == f2);
```

Last but not least, we might want to check how the diagram looks like. For that purpose we use the [DOT language](https://en.wikipedia.org/wiki/DOT_(graph_description_language)). Function `to_dot_graph` prints DOT representation of given diagram into given output stream (`std::cout` in the example below, you might also consider `std::ofstream` or `std::ostringstream`). DOT string can be visualized by different tools listed in the Wikepedia page, the fastest ways is to use [Webgraphviz](http://www.webgraphviz.com/).
```C++
m.to_dot_graph(std::cout, f);
```
This is how a DOT string might look like. If you paste this to the Webgraphviz you will get a picture of a diagram.
```
digraph DD {
    node [shape = square] 6995904 6995952;
    node [shape = circle];

    6998736 [label = "x0"];
    6998064 [label = "x2"];
    6997520 [label = "x4"];
    6995904 [label = "0"];
    6995952 [label = "1"];
    6997920 [label = "x3"];
    6998592 [label = "x1"];

    6998736 -> 6998064 [style = dashed];
    6998736 -> 6998592 [style = solid];
    6998064 -> 6997520 [style = dashed];
    6998064 -> 6997920 [style = solid];
    6997520 -> 6995904 [style = dashed];
    6997520 -> 6995952 [style = solid];
    6997920 -> 6997520 [style = dashed];
    6997920 -> 6995952 [style = solid];
    6998592 -> 6998064 [style = dashed];
    6998592 -> 6995952 [style = solid];

    { rank = same; 6998736; }
    { rank = same; 6998592; }
    { rank = same; 6998064; }
    { rank = same; 6997920; }
    { rank = same; 6997520; }
    { rank = same; 6995904; 6995952; }
}
```

### Multi-valued decision diagrams
*comming soon*

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
auto const SIs  = m.structural_importances(dpbds);
auto const BIs  = m.birnbaum_importances(ps, dpbds);
auto const CIs  = m.criticality_importances(BIs, ps, U);
auto const FIs  = m.fussell_vesely_importances(dpbds, ps, U);
```
Each `*Is` is a `std::vector` of `double`s representing given importance measures for given variables.  
Finally we can enumerate all Minimal Cut Vectors. Like with `satisfy_all` we need to specify a data type which will hold values of variables. Again `std::bitset` seems to be the best choice.
```C++
auto const MCVs = m.mcvs<std::bitset<5>>(dpbds);
```
`MCVs` is now a `std::vector` of `std::bitset`s which represents MCVs.