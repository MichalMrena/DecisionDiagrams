*We don't have a formal name yet so there is no cool heading here.*

C++ library for creation and manipulation of decision diagrams. It is being developed at the [Faculty of Management Science and Informatics](https://www.fri.uniza.sk/en/), [University of Žilina](https://www.uniza.sk/index.php/en/) as a student project at the [Department of Informatics](https://ki.fri.uniza.sk/).

## How to install
Library is header only so its easy to incorporate it in your project. All you need to do is to place contents of [this](./src/lib/) directory into your project files.

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
Simplest diagrams that you can create are a diagram of a Boolean constant (`false(0)` or `true(1)`)
```C++
auto cFalse = m.just_val(0);
auto cTrue  = m.just_val(1);
```
and a diagram of a single Boolean variable.
```C++
auto x0 = m.just_var(0);
auto x4 = m.just_var(4);
```
Useful trick for creating diagrams for variables is to create a reference to the manager with name `x`. Manager overloads the `operator()` that does the same thing as `just_var`.
```C++
auto& x = m;
auto x1 = x(1);
```
In order to create complemented (negated) variable you can use `just_var_not` or `operator()` with not flag.
```C++
auto x2_ = m.just_var_not(2);
auto x3_ = x(3, NOT());
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
Now that we have a diagram of a function we can do something with it. Since it is a function we can evaluate it for given values of variables. As you can see, these values can be specified in different ways. It is up to you which one you pick. `get_value` returns either `0` (false) or `1` (true).
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
Variable values for which the function evaluates to `1` can also be enumerated using output iterator.
```C++

```