# 🧸 TeDDy

TeDDy is a C++ library for the creation and manipulation of decision diagrams. It is being developed as a project at the [Faculty of Management Science and Informatics](https://www.fri.uniza.sk/en/), the [University of Žilina](https://www.uniza.sk/index.php/en/) at the [Department of Informatics](https://ki.fri.uniza.sk/).  
This text assumes that the reader is familiar with decision diagrams to some extent. Our library supports [Binary Decision Diagrams](https://en.wikipedia.org/wiki/Binary_decision_diagram) (BDDs) and their generalization Multi-Valued Decision Diagrams (MDDs).

---
## Contents

  * [How to install](#how-to-install)
    + [Compiling](#compiling)
  * [Library API](#library-api)
  * [Basic usage](#basic-usage)
  * [Memory management](#memory-management)
    + [Node pool](#node-pool)
    + [Cache](#cache)
  * [Other](#other)
    + [Assertions](#assertions)
    + [Variable ordering](#variable-ordering)
  * [Reliability analysis](#reliability-analysis)
    + [Basic usage](#basic-usage-1)
    + [Example](#example)
  * [Publications](#publications)
      - [List of publications:](#list-of-publications-)

---

## How to install
The library is header-only so it is easy to incorporate it into your project. All you need to do is to place the contents of the [include](./include/) directory in your project files and include a header file in your code as is shown in the examples below.  
If you would like to use the library in multiple projects and you are a Linux user you can use the following commands to install TeDDy:
```bash
git clone git@github.com:MichalMrena/DecisionDiagrams.git
cd DecisionDiagrams
sudo make install
```
This installs library files to `/usr/local` which should be visible to your compiler. You can specify different path by setting the `PREFIX` variable: `make PREFIX=<path> install`. The installation script writes path to all installed files into the `install_manifest.txt` file.  
To uninstall the library go to the directory where the `install_manifest.txt` file is located and run `[sudo] xargs rm < install_manifest.txt`.

### Compiling
TeDDy uses features from `C++20` so you need to set your compiler for this version of the C++ language by using the `-std=c++20` flag for `clang++` and `g++` and `/std:c++20` for MSVC. It was tested with `g++ 11.1.0`, `clang++ 15.0.0` and `MSVC 19.31.31107`.  

## Library API
Functions from the library are accessible via the instance of a diagram manager. TeDDy offers four diagram managers for different kinds of decision diagrams.  
1. `bdd_manager` for Binary Decision Diagrams (BDDs).  

2. `mdd_manager<P>` for Multi-Valued Decision Diagrams (MDDs) representing Multiple-Valued logic functions. The domain of each variable is `{0, 1, ... P - 1}` and the set of values of the function is also `{0, 1, ... P - 1}`.  

3. `imdd_manager` for (integer) Multi-Valued Decision Diagrams (iMDDs) representing integer functions. The domain of each variable can be a different set of the form `{0, 1, 2, ..., di - 1}` where the `di` for each variable is specified in the constructor. The set of values of the function is a set of the form `{0, 1, 2, ...}`.

4. `ifmdd_manager<PMax>` for (integer) Multi-Valued Decision Diagrams (iMDDs) representing integer functions. The domain of each variable can be a different set of the form `{0, 1, 2, ..., min(di, PMax - 1)}` where the `PMax` is specified as the template parameter and `di` for each variable is specified in the constructor. The set of values of the function is a set of the form `{0, 1, 2, ...}`.  
  
Managers 1, 2, and 4 use nodes with more compact memory representation since the maximum of the domains is known at compile time. The only difference between 3 and 4 is in this property so if `PMax` is known it is better to use the manager 4.  

All diagram managers have the same API. **Full documentation** is available **[here](https://michalmrena.github.io/teddy.html)**.

## Basic usage
Typical usage of the library can be summarized in 3 steps:
1. Choose one of the four diagram managers.
2. Create a decision diagram representing desired function(s).
3. Examine properties of the function(s).  

The following example shows the above steps for the `bdd_manager` and Boolean function `f(x) = (x0 and x1) or (x2 and x3)`:

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

## Memory management
### Node pool
Teddy uses a pool of pre-allocated nodes. The initial number of allocated nodes (`nodePoolSize`) is provided by the user in the manager's constructor. It is hard to give general advice on how many nodes you should allocate. On modern hardware, it should not be a problem to allocate a couple of millions of nodes that will occupy roughly tens or hundreds of MiBs of memory depending on the type of the manager used. The more nodes you allocate at the beginning, the fewer allocations will be needed during computations resulting in a faster computation. Clearly, for small examples, hundreds or thousands of nodes are just enough.  

If there are no nodes left in the pool, automatic garbage collection (`gc`) is performed to recycle unused nodes. Let `gcCount` be the number of garbage collected nodes (returned to the pool). Then if `gcCount < gcThreshold * nodePoolSize` then an additional node poll of size `overflowNodePoolSize` is allocated. Default value of the parameter `gcThreshold` is `0.05`. The user can adjust the parameter using `set_gc_ratio` function. Size of the additional pool `overflowNodePoolSize` can be set in the manager's constructor alongside the `nodePoolSize`. The default value if `overflowNodePoolSize = nodePoolSize / 2`.

### Cache
The library uses cache to speed up diagram manipulation by avoiding expensive recomputations. The size of the cache depends on the number of currently used nodes. The size is calculated as `cacheRatio * uniqueNodeCount`, where the `uniqueNodeCount` is the number of unique nodes currently used by the manager. The default value of `cacheRatio` is `0.5`. The user can adjust the ratio using `set_cache_ratio` function. The bigger the cache the better the computation speed. However, a bigger ratio means higher memory consumption. It is up to the user to keep the two factors balanced. From the experience even cache ratios `1.0` of `2.0` are fine.

## Other

### Assertions
By default, the library contains runtime assertions that perform various checks such as bounds checking for indices of variables and similar. In case you want to ignore these assertions e.g. in some performance demanding use case, you need to put `#define NDEBUG` before you include the teddy header.  
***This feature will change in the future so that it does not rely on a macro.***

### Variable ordering
The user can specify the order of variables in the constructor of the manager. After that, the order stays the same. The user can explicitly invoke reordering the heuristic by using the function `sift`. The heuristic tries to minimize the number of nodes in all diagrams managed by the manager.

## Reliability analysis
The application of Decision Diagrams in reliability analysis was one of the motivations for the creation of the library. TeDDy has a separate API for reliability analysis. The API builds on the existing API for diagram manipulation to which it adds functions that use diagrams to evaluate the reliability of systems described by a structure function. As with diagram manipulation, the reliability API is accessible via an instance of a reliability manager. There are four reliability managers analogous to diagram managers.
1. `bss_manager` for Binary-State Systems (BSS). Uses BDDs.  

2. `mss_manager<P>` for homogenous Multi-State Systems (MSS). Domains of variables and sets of values of functions correspond to the number of component/system states. Uses MDDs.  

3. `imss_manager` for non-homogenous Multi-State Systems (MSS). Domains of variables and sets of values of functions correspond to the number of component/system states. Uses iMDDs.  

4. `ifmss_manager<PMax>` for non-homogenous Multi-State Systems (MSS). Domains of variables and sets of values of functions correspond to the number of component/system states. Uses ifMDDs.  
  
Note that each reliability manager is a child class of the corresponding diagram manager so advantages and disadvantages of the base managers apply e.g. node compactness in case of (if/i)MDDs.  
All reliability managers have the same API. **Full documentation** is available **[here](https://michalmrena.github.io/teddy.html)**.

### Basic usage
The usage of reliability managers is practically identical to diagram managers. Many of the reliability functions have a parameter that represents component state probabilities. The parameter does not have a specific type but instead uses a template. The reason is that probabilities can be stored in different combinations of containers such as `std::vector` or `std::array`. The important property of the parameter is that if `ps` is the name of the parameter then the expression `ps[i][k]` returns the probability that the `i`-th component is in the state `k`.

### Example
The following example shows the basic usage of the reliability manager. For a detailed description of the manager's API please see the [documentation](https://michalmrena.github.io/teddy.html).
```C++
#include "teddy/teddy_reliability.hpp"
// or use if you've installed the library
// #include <teddy/teddy_reliability.hpp>

int main()
{
    // First, we need to create a diagram for the structure function.
    // We use the truth vector of the function:
    std::vector<teddy::uint_t> vector
        { 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1
        , 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2 };

    // The truth vector describes a nonhomogenous system, so we also need
    // number of states for each component:
    std::vector<uint_t> domains({2, 3, 2, 3});
    teddy::ifmss_manager<3> manager(4, 1'000, domains);
    auto sf = manager.from_vector(vector);

    // You can use different combinations of std::vector, std::array or
    // similar containers. We chose a vector of arrays here to hold
    // component state probabilities.
    std::vector<std::array<double, 3>> ps
    ({
        {0.1, 0.9, 0.0},
        {0.2, 0.6, 0.2},
        {0.3, 0.7, 0.0},
        {0.1, 0.6, 0.3}
    });

    // To calculate system availability or unavailability for a given
    // system state (1) is as simple as:
    double A = manager.availability(1, ps, sf);
    double U = manager.unavailability(1, ps, sf);

    // We can also simply enumerate all Minimal Cut Vectors for a given system
    // state (1). We just need to specify a type that variables will be stored
    // into. In this case, we used the std::array:
    std::vector<std::array<unsigned int, 4>> MCVs
        = manager.mcvs<std::array<unsigned int, 4>>(sf, 1);

    // Importance measures are defined in terms of logic derivatives.
    // Since there are different types of derivatives the calculation of
    // the derivatives is separated from the calculation of importance measures.

    // To calculate Structural Importance we first need to calculate
    // the derivative.
    auto dpbd = manager.idpbd_type_3_decrease({1, 0}, 1, sf, 2);

    // Now, to calculate the Structural Importance of the second component,
    // we use the derivative.
    auto SI = manager.structural_importance(dpbd);
}
```

## Publications
We have published several papers on decision diagrams and reliability analysis. Most of these papers had an experimental section where we examined various properties of decision diagrams using TeDDy.

#### List of publications
Mrena, M., Sedlacek, P., &#38; Kvassay, M. (2021). **Linear Fold and Tree Fold in Creation of Binary Decision Diagrams of Standard Benchmarks.** *2021 11th IEEE International Conference on Intelligent Data Acquisition and Advanced Computing Systems: Technology and Applications (IDAACS)*, *2*, 1120–1125. https://doi.org/10.1109/IDAACS53288.2021.9660940
&nbsp;  

Mrena, M., &#38; Kvassay, M. (2021). **Comparison of Left Fold and Tree Fold Strategies in Creation of Binary Decision Diagrams**. *2021 International Conference on Information and Digital Technologies (IDT)*, 341–352. https://doi.org/10.1109/IDT52577.2021.9497593
&nbsp;  

Mrena, M., Kvassay, M., &#38; Stankovic, R. S. (2020). **Software Library for Teaching Applications of Binary Decision Diagrams in Reliability Analysis**. *2020 18th International Conference on Emerging ELearning Technologies and Applications (ICETA)*, 487–497. https://doi.org/10.1109/ICETA51985.2020.9379170