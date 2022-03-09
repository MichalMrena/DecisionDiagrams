# 🧸 TeDDy

TeDDy is a C++ library for creation and manipulation of decision diagrams. It is being developed as a project at the [Faculty of Management Science and Informatics](https://www.fri.uniza.sk/en/), [University of Žilina](https://www.uniza.sk/index.php/en/) at the [Department of Informatics](https://ki.fri.uniza.sk/).

## Decision diagrams
This text assumes that the reader is familiar with decision diagrams to some extent. Our library supports [Binary Decision Diagrams](https://en.wikipedia.org/wiki/Binary_decision_diagram) (BDDs) and their generalization Multivalued Decision Diagrams (MDDs).

## How to install
Library is header only so it is easy to incorporate it in your project. All you need to do is to place contents of the [include](./include/) directory in your project files and include a header file in your code as is shown in the examples below.  
If you would like to use the library in multiple projects and you are a Linux user you can use the following commands to install TeDDy:
```bash
git clone git@github.com:MichalMrena/DecisionDiagrams.git
cd DecisionDiagrams
sudo make install
```
This installs library files to `/usr/local` which should be visible to your compiler. You can specify different path by setting `PREFIX` variable: `make PREFIX=<path> install`.  
To uninstall the library go to the directory where you've run the install command and run `[sudo] xargs rm < install_manifest.txt`.

### Compiling
TeDDy uses features from `C++20` so you might need to set your compiler for this version of the C++ language by using the `-std=c++20` flag for `clang++` and `g++` and `/std:c++latest` for MSVC. It was tested with `g++ 11.1.0` and `MSVC TODO`.  
***Support for C++17 is comming soon.***

## Library API
Functions from the library are accesed via instance of a diagram manager. TeDDy offeres four diagram managers for different kinds of decision diagrams.  
1. `bdd_manager` for Binary Decision Diagrams (BDDs).  

2. `mdd_manager<P>` for Multivalued Decision Diagrams (MDDs) representing Multiple-Valued logic functions. Variables and functions can have values from the set {0, 1, ... P - 1}.  

3. `imdd_manager` for (integer) Multivalued Decision Diagrams (iMDDs) representing integer functions. Domains of variables are specified in the constructor of the manager and are from the set {0, 1, 2, ...}. Functions can have values from the set {0, 1, 2, ...}.

4. ` ifmdd_manager<PMax>` for (integer) Multivalued Decision Diagrams (iMDDs) representing integer functions. Domains of variables are specified in the constructor of the manager and are from the set {0, 1, 2, ...,  PMax - 1}. Functions can have values from the set {0, 1, 2, ...}.  
  
Managers 1, 2 and 4 use nodes with more compact memory representation since maximum of the domains is known at compile time. The only difference between 3 and 4 is in this property so if `PMax` is known it is better to use the manager 4.  
**Doxygen generated documentation is available at [TODO](#).**

## Basic usage
Typical usage of the library can be summarized in 3 steps:
1. Choose one of the four diagram managers.
2. Create a decision diagram representing desired function(s).
3. Examine properties of the function(s).  

Following example shows above steps for the `bdd_manager` and Boolean function `f(x) = (x0 and x1) or (x2 and x3)`:

```C++
#include "teddy/teddy.hpp"
// or use if you've installed the library
// #include <teddy/teddy.hpp>

int main()
{
    // 4 variables, 1000 pre-allocated nodes (see memory management):
    teddy::bdd_manager manager(4, 1'000);

    // create diagram for a single variable (indices start at 0):
    auto x0 = manager.variable(0);

    // we recommend using auto, but if you want you can use an alias:
    using diagram_t = teddy::bdd_manager::diagram_t;
    diagram_t x1 = manager.variable(1);

    // operator() serves the same purpose as .variable call
    // it is useful to create a reference to the manager with name x
    auto& x = manager;
    diagram_t x2 = x(2);

    // diagrams for multiple variables can be created at once:
    std::vector<diagram_t> xs = manager.variables({0, 1, 2, 3, 4});

    // diagram_t is cheap handle type, multiple diagrams can point
    // to a same node, to test whether they do use .equals:
    assert(&x1 != &xs[1] && x1.equals(xs[1]));

    using namespace teddy::ops; // (to simplify operator names)
    // finally, to create a diagram for the function f
    // we use the apply function:
    diagram_t f1 = manager.apply<AND>(xs[0], xs[1]);
    diagram_t f2 = manager.apply<AND>(xs[2], xs[3]);
    diagram_t f  = manager.apply<OR>(f1, f2);

    // now that we have diagram for the funtion f, we can test its properties
    // e.g. evaluate it for give variable assignment
    unsigned int val = manager.evaluate(f, std::array {1, 1, 0, 1});
    // val will contain either 0 or 1 (1 in this case)

    // we can see how the diagram looks like by printing its dot representation
    // into a file or console and visualizing it using e.g. graphviz
    manager.to_dot_graph(std::cout, f); // console
    std::ofstream ofst("f.dot");
    manager.to_dot_graph(ofst, f); // file

    // to calculate number of different variable assignments for which the
    // function evaluates to 1 we can use .satisfy_count:
    std::size_t sc = manager.satisfy_count(f);

    // we can also enumerate all variable assignments for which the
    // the function evaluates to 1:
    std::vector<std::array<unsigned int, 4>> sa
        = manager.satisfy_all<std::array<unsigned int, 4>>(f);
}
```

### Memory management
Teddy uses pool of pre-allocated nodes. Size of the pool is provided by the user in the manager's constructor. If there are no nodes left in the pool, garbage collection s performed to recycle unused nodes. If the number of nodes collected during garbage collection is small then additional pool of nodes is allocated. Size of the additional pool and the threshold for its allocation can be configured. See the [TODO](doc/teddy.md) for details.

### Assertions
By default, the library contains runtime assertions that perform various checks such as bounds checking for indices of variables and similar. In case you want to ignore these assertions e.g. in some performance demanding use case, you need to put `#define NDEBUG` before you include the teddy header.  
***This feature will change in the future so that it does not rely on a macro.***

### Variable ordering
Order of variables can be specified in a constructor of a manager. After that, the order stays the same since the automatic variable reordering is turned off by default. TODO

### Big integers
***Comming soon.***

## Reliability analysis
### Examples
TODO

## Publications
TODO links to papers

## Implementation
TODO