# 🧸 TeDDy

TeDDy is a C++ library for the creation and manipulation of decision diagrams. It is being developed at the [Faculty of Management Science and Informatics](https://www.fri.uniza.sk/en/), the [University of Žilina](https://www.uniza.sk/index.php/en/) at the [Department of Informatics](https://ki.fri.uniza.sk/).  
This text assumes that the reader is familiar with decision diagrams to some extent. Our library supports [Binary Decision Diagrams](https://en.wikipedia.org/wiki/Binary_decision_diagram) (BDDs) and their generalization Multi-Valued Decision Diagrams (MDDs).

---
# Contents
  * [How to install](#how-to-install)
  * [How to use](#how-to-use)
  * [Publications and citation](#publications-and-citation)

---

# How to install
TeDDY is a header-only library. There are two principal ways to use it in your project.

## Copy the files
The simplest way is to download the library (using `git clone`, [one of the releases](https://github.com/MichalMrena/DecisionDiagrams/releases), or [download as zip](https://github.com/MichalMrena/DecisionDiagrams/archive/refs/heads/master.zip)) and place the [libteddy](./libteddy/) folder somewhere where your compiler can see it e.g., directly in your project.

## Using cmake
You can install the library using standard procedure.  
```sh
git clone git@github.com:MichalMrena/DecisionDiagrams.git
cd DecisionDiagrams
mkdir build
cd build
cmake ..
sudo make install
```
This installs library files to the default location which should be visible to your compiler. You can specify a different path by passing the `-DCMAKE_INSTALL_PREFIX=<your path>` to cmake. Cmake writes paths to all installed files into the `install_manifest.txt` file. To uninstall the library go to the directory where the `install_manifest.txt` file is located and run `[sudo] xargs rm < install_manifest.txt`.  
Subsequently, you can use the library in the following way:
```cmake
find_package(
    TeDDy REQUIRED
)

add_executable(
    yourtarget
        main.cpp
)

target_link_libraries(
    yourtarget PRIVATE teddy::teddy
)
```

## Compiling
TeDDy uses features from `C++20` so you may need to set your compiler to this version of the C++ language by using the `-std=c++20` flag for `clang++` and `g++` and `/std:c++20` for MSVC. If you are using `cmake` it should handle this for you. We tested it on Linux with `g++ 10.0.0`, `clang++ (libc++) 13.0.0`, `clang++ (libstdc++) 11.0.0`, and on Windows with `MSVC 19.31.31107`.  

## Running tests
To run the tests, you will need to install [Boost.Test](https://www.boost.org/doc/libs/1_82_0/libs/test/doc/html/index.html) and [fmt](https://github.com/fmtlib/fmt) (until `std::format` gets better support).
You can install them using your distribution package manager. For **Boost**, search for the following packages:
- `boost-devel`, `boost-test` (dnf),
- `libboost-test-dev` (apt),
- `boost-devel`, `libboost_unit_test_framework` (xbps),
- (or similar for other package managers).  
If no such package exists for your OS, follow the [Getting started](https://www.boost.org/doc/libs/1_82_0/more/getting_started/index.html) section of the Boost website.  
For fmt search for:
- `fmt-devel` (dnf, xbps),
- `libfmt-dev` (apt).  
After installing **Boost** and **fmt**, you can compile and run the tests using cmake in the following way (assuming you are in the `DecisionDiagrams` folder):
```sh
cmake -DCMAKE_BUILD_TYPE=Release -DLIBTEDDY_BUILD_TESTS=true -S . -B build
cmake --build build -j4
cmake --build build -t test
```

# How to use
TeDDy consists of two modules. The first module `teddy-core` contains algorithms for general manipulation and the creation of decision diagrams. The second module `teddy-reliability` contains algorithms aimed at reliability analysis utilizing decision diagrams.

## Core
All library functions are accessible via an instance of a diagram manager. TeDDy offers four diagram managers for different kinds of decision diagrams.  
1. `bdd_manager` for Binary Decision Diagrams (BDDs).  

2. `mdd_manager<M>` for Multi-valued Decision Diagrams (MDDs) representing Multiple-Valued logic functions. The domain of each variable is `{0,1,...,M-1}` and the set of values of the function is also `{0,1,...,M-1}`.  

3. `imdd_manager` for (integer) Multi-valued Decision Diagrams (iMDDs) representing integer functions. The domain of each variable can be a different set of the form `{0,1,2,...,di-1}` where `di` for each variable is specified in the constructor. The set of values of the function is a set of the form `{0,1,2,...}`.

4. `ifmdd_manager<M>` for (integer) Multi-valued Decision Diagrams (iMDDs) representing integer functions. The domain of each variable can be a different set of the form `{0,1,2,...,min(di,M-1)}` where the `M` is specified as the template parameter and `di` for each variable is specified in the constructor. The set of values of the function is a set of the form `{0,1,2,...}`.  
  
Managers 1, 2, and 4 use nodes with more compact memory representation since the maximum of the domains is known at compile time. The only difference between 3 and 4 is in this property so if `M` is known it is better to use the manager 4.  

Typical usage of the library can be summarized in 3 steps:
1. Choose one of the four diagram managers.
2. Create a decision diagram representing desired function(s).
3. Examine the properties of the function(s).  

The following example shows the above steps for the `bdd_manager` and Boolean function `f(x) = (x0 and x1) or (x2 and x3)`:

```C++
#include <libteddy/core.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

int main()
{
    // 4 variables, 1000 pre-allocated nodes (see memory management)
    teddy::bdd_manager manager(4, 1'000);

    // Alias for the type of the diagram, or you can just use auto.
    using diagram_t = teddy::bdd_manager::diagram_t;

    // Create diagram for a single variable (indices start at 0).
    diagram_t x0 = manager.variable(0);
    diagram_t x1 = manager.variable(1);

    // operator() serves the same purpose as .variable call.
    // It is convenient to create a reference to the manager with name x.
    teddy::bdd_manager& x = manager;
    diagram_t x2 = x(2);

    // Diagrams for multiple variables can be created at once.
    std::vector<diagram_t> xs = manager.variables({0, 1, 2, 3});

    // diagram_t is cheap handle type, multiple diagrams can point
    // to the same node, to test whether they do use .equals.
    assert(x1.equals(xs[1]));

    // (to simplify operator names)
    using namespace teddy::ops;
    // Finally, to create a diagram for the function:
    // f(x) = (x0 and x1) or (x2 and x3)
    // we use the apply function.
    diagram_t f1 = manager.apply<AND>(xs[0], xs[1]);
    diagram_t f2 = manager.apply<AND>(xs[2], xs[3]);
    diagram_t f  = manager.apply<OR>(f1, f2);

    // Now that we have diagram for the funtion f, we can test its properties
    // e.g., evaluate it for give variable assignment.
    const int val = manager.evaluate(f, std::array {1, 1, 0, 1});
    assert(val == 1);

    // We can see how the diagram looks like by printing its dot representation
    // into a file or console and visualizing it using e.g. graphviz.
    manager.to_dot_graph(std::cout, f);
    std::ofstream ofst("f.dot");
    manager.to_dot_graph(ofst, f);

    // To calculate number of different variable assignments for which the
    // function evaluates to 1 we can use .satisfy_count.
    long long sc = manager.satisfy_count(1, f);

    // We can also enumerate all variable assignments for which the
    // the function evaluates to 1.
    std::vector<std::array<int, 4>> sa
        = manager.satisfy_all<std::array<int, 4>>(1, f);
}
```
All diagram managers have the same API. **Full documentation** is available [here](https://michalmrena.github.io/teddy.html). For more examples see the [examples](./examples/) folder.

## Reliability
As with diagram manipulation, the reliability analysis functions are accessible via an instance of a reliability manager. There are four reliability managers analogous to diagram managers.
1. `bss_manager` for Binary-State Systems (BSS). Uses BDDs.  

2. `mss_manager<M>` for homogeneous Multi-State Systems (MSS). Domains of variables and sets of values of functions correspond to the number of component/system states. Uses MDDs.  

3. `imss_manager` for non-homogeneous Multi-State Systems (MSS). Domains of variables and sets of values of functions correspond to the number of component/system states. Uses iMDDs.  

4. `ifmss_manager<M>` for non-homogenous Multi-State Systems (MSS). Domains of variables and sets of values of functions correspond to the number of component/system states. Uses ifMDDs.  
  
Note that each reliability manager is a child class of the corresponding diagram manager, hence, the advantages and disadvantages of the base managers apply. All reliability managers have the same API. **Full documentation** is available [here](https://michalmrena.github.io/teddy.html).

The usage of reliability managers is analogous to diagram managers. Many of the reliability functions have a parameter that represents component state probabilities. The parameter does not have a specific type but instead uses a template. The reason is that probabilities can be stored in different combinations of containers such as `std::vector` or `std::array`. The type holding the probabilities must satisfy that if `ps` is the name of the parameter then the expression `ps[i][k]` returns the probability that the `i`-th component is in the state `k`. The following example shows the basic usage of the reliability manager:
```C++
#include <libteddy/reliability.hpp>
#include <array>
#include <iostream>
#include <vector>
#include "libteddy/details/reliability_manager.hpp"

int main()
{
    // First, we need to create a diagram for the structure function.
    // We can use the truth vector of the function.
    std::vector<int> vector(
        {0,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,2,2,2,2,2,1,2,2,2,2,2}
    );

    // The truth vector describes a nonhomogeneous system, so we also need.
    // The number of states of each component.
    std::vector<int> domains({2, 3, 2, 3});

    // 4 components, 1000 pre-allocated nodes (see memory management)
    teddy::ifmss_manager<3> manager(4, 1'000, domains);

    // Alias for the type of the diagram, or you can just use auto.
    using diagram_t = teddy::ifmss_manager<3>::diagram_t;
    diagram_t sf = manager.from_vector(vector);

    // We can use different combinations of std::vector, std::array or similar containers.
    // We chose vector of arrays here to hold component state probabilities.
    std::vector<std::array<double, 3>> ps
    ({
        {0.1, 0.9, 0.0},
        {0.2, 0.6, 0.2},
        {0.3, 0.7, 0.0},
        {0.1, 0.6, 0.3}
    });

    // To calculate system availability or unavailability for a given.
    // System state (1) is as simple as:
    const double A = manager.calculate_availability(1, ps, sf);
    const double U = manager.calculate_unavailability(1, ps, sf);
    std::cout << "A = " << A << "\n";
    std::cout << "U = " << U << "\n";

    // We can also simply enumerate all Minimal Cut Vectors for a given system
    // state (1). We just need to specify a type that will store the vectors.
    // In this case, we used std::array:
    std::vector<std::array<int, 4>> MCVs = manager.mcvs<std::array<int, 4>>(sf, 1);

    // Importance measures are defined in terms of logic derivatives.
    // Since there are different types of derivatives the calculation of
    // the derivatives is separated from the calculation of importance measures.

    // To calculate Structural Importance we first need to calculate
    // the derivative.
    diagram_t dpbd = manager.dpld({2, 1, 0}, teddy::dpld::type_3_decrease(1), sf);

    // Now, to calculate the Structural Importance of the second component,
    // we use the derivative.
    const double SI_2 = manager.structural_importance(dpbd);
    std::cout << "SI_2 = " << SI_2 << "\n";
}
```
For more examples see the [examples](./examples/) folder.

## Memory management
### Node pool
TeDDy uses a pool of pre-allocated nodes. The initial number of allocated nodes (`nodePoolSize`) is provided by the user in the manager's constructor. It is hard to give general advice on how many nodes you should allocate. On modern hardware, it should not be a problem to allocate a couple of millions of nodes that will occupy roughly tens or hundreds of MiBs of memory depending on the type of manager used. The more nodes you allocate at the beginning, the fewer allocations will be needed during computations resulting in a faster computation. Clearly, for small examples, hundreds or thousands of nodes are just enough.  

If there are no nodes left in the pool, automatic garbage collection (`gc`) is performed to recycle unused nodes. Let `gcCount` be the number of garbage collected nodes (returned to the pool). Then if `gcCount < gcThreshold * nodePoolSize` an additional node poll of size `overflowNodePoolSize` is allocated. The default value of the parameter `gcThreshold` is `0.05`. The user can adjust the parameter by using `set_gc_ratio` function. The size of the additional pool `overflowNodePoolSize` can be set in the manager's constructor alongside the `nodePoolSize`. The default value is `overflowNodePoolSize = nodePoolSize / 2`.

### Cache
The library uses a cache to speed up diagram manipulation by avoiding expensive recomputations. The size of the cache depends on the number of currently used nodes. The size is calculated as `cacheRatio * uniqueNodeCount`, where the `uniqueNodeCount` is the number of unique nodes currently used by the manager. The default value of `cacheRatio` is `0.5`. The user can adjust the ratio by using `set_cache_ratio` function. The bigger the cache the better the computation speed. However, a bigger ratio means higher memory consumption. It is up to the user to keep the two factors balanced. From the experience even cache ratios `1.0` of `2.0` are fine.

## Assertions
By default, the library contains runtime assertions that perform various checks such as bounds checking and similar. In case you want to ignore these assertions e.g. in some performance-demanding use case, you need to put `#define NDEBUG` before you include the TeDDy header.  

## Variable ordering
The user can specify the order of variables in the constructor of the manager. After that, the order stays the same. The user can explicitly invoke the reordering heuristic by using the `force_reorder` function. The heuristic tries to minimize the number of nodes in all diagrams managed by the manager.

# Publications and citation
*A paper describing TeDDy itself is in preparation.*  
Also, we have published several papers on decision diagrams and reliability analysis. The following papers had an experimental section where we examined various properties of decision diagrams using TeDDy:

Mrena, M., &#38; Kvassay, M. (2021). **Comparison of Left Fold and Tree Fold Strategies in Creation of Binary Decision Diagrams**. *2021 International Conference on Information and Digital Technologies (IDT)*, 341–352. https://doi.org/10.1109/IDT52577.2021.9497593
&nbsp;  

Mrena, M., Sedlacek, P., &#38; Kvassay, M. (2021). **Linear Fold and Tree Fold in Creation of Binary Decision Diagrams of Standard Benchmarks.** *2021 11th IEEE International Conference on Intelligent Data Acquisition and Advanced Computing Systems: Technology and Applications (IDAACS)*, *2*, 1120–1125. https://doi.org/10.1109/IDAACS53288.2021.9660940
&nbsp;  

Mrena, M., Kvassay, M., &#38; Czapp, S. (2022). **Single and Series of Multi-valued Decision Diagrams in Representation of Structure Function.** *Lecture Notes in Networks and Systems*, *484 LNNS*, 176–185. https://doi.org/10.1007/978-3-031-06746-4_17
&nbsp;  

Mrena, M., &#38; Kvassay, M. (2022). **Comparison of Single MDD and Series of MDDs in the Representation of Structure Function of Series-Parallel MSS.** *2022 IEEE 16th International Scientific Conference on Informatics (Informatics)*, 225–230. https://doi.org/10.1109/INFORMATICS57926.2022.10083458
&nbsp;  
