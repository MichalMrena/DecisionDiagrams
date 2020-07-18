# Binary decision diagrams
NAME GOES HERE is a small library for manipulation of decision diagrams. It is written in C++17.    
For now, we focus on Binary Decision Diagrams as described by Bryant [here](https://ieeexplore.ieee.org/document/1676819) and [here](https://dl.acm.org/doi/10.1145/136035.136043). Diagrams and algorithms are implemented in a simple straightforward way using mostly the OOP paradigm.  
Interesting thing about our diagrams is that you can store some data in vertices and arcs. Type of the data is a template paramter of the diagram so that you can choose an arbitraty data type and you will have a member variable of that type in each vertex/arc. If you choose ```void``` there won't be any member variable. We use this feature in algoritms from Realiability theory where we store probabilities.

## How to install
Since we use templates in most files the whole library as header only. Using it is therefore very simple. All you need to do is to include [lib](./src/lib/) folder in your project and include [bdd_tools.hpp](./src/lib/bdd_tools.hpp) header. You might need to set your compiler to use C++17 explicitly. Also some compilers might require you to link [filesystem library](https://stackoverflow.com/questions/33149878/experimentalfilesystem-linker-error) explicitly.

## Basic usage
This piece of code is the same for all examples so we will just post it once here at the begining. It creates instances of two classes that provide different kinds of algorithms. The instance ```bdd_tools``` holds a simple memory pool. All diagrams created from it use this pool so their lifetime should not extend the lifetime of the tools instance.  
Note that we use so called "[~~Almost~~ Always Auto style](http://cginternals.github.io/guidelines/articles/almost-always-auto/)" because it looks nice and helps us to avoid typing heavily templated typenames. C++17 made it even better. See [copy elision](https://en.cppreference.com/w/cpp/language/copy_elision).
```
#include "/lib/bdd_tools.hpp"

int main()
{
    auto tools       = bdd_tools {};
    auto creator     = tools.creator();
    auto manipulator = tools.manipulator();

    /* Code from other examples goes here. */
}
```
### Creating diagrams
Simplest diagrams that you can create are ones that represent a Boolean constant or a single Boolean variable.
```
auto false = creator.just_val(1);
auto x1    = creator.just_var(1);
```
In the above example variables ```false``` and ```x1``` are instances of the ```bdd``` class. This class manages the lifecycle of the diagram. In its copy constructor it creates a deep copy of the diagram so it might be an expensive operation. However, its move constructor is cheap since the class only manages a pointer to the diagram.

Most of the times you probably want to work with a more complex functions e.g. ```x1 && x2 || x3```. One way to do this is to create a diagram from a truth vector of a function. Truth vector of this function is ```01010111```. This vector is a last column in a truth table of this function where variables are ordered as ```x1 x2 x3```.  
In order to create a diagram from the vector you first need to create the vector itself. You can load it from a file or from a string and the interesting thing 
is that you can generate it from a lambda expression. The lambda expression must have the same form shown in the example i.e. it must be a [generic lambda](https://en.cppreference.com/w/cpp/language/lambda) and it must bind its parameter to a reference.
```
auto v1 = truth_vector::from_text_file("truth_vector.txt");
auto v2 = truth_vector::from_string("01010111");
auto v3 = truth_vector::from_lambda([](auto&& x) 
{ 
    return (x(1) && x(2)) || x(3);
});

auto d1 = creator.from_vector(v1);
auto d2 = creator.from_vector(v2);
auto d3 = creator.from_vector(v3);

// d1 == d2 == d3
```
Another way to create a complex function is to load it from a PLA file which is a special kind of a truth table that can define multiple Boolean functions in an efficient way. As with the truth vector you first load the file and then you create diagrams from it. Since there can be multiple functions in the file the result is ```std::vector``` of diagrams.
```
auto file     = pla_file::load_file(fileName);
auto diagrams = creator.from_pla(file);
```
### Manipulating diagrams
Basic and probably the most important operation that you can peform with existing diagrams is called ```apply```. It takes two diagrams and a binary operation and creates a new digaram that is a result of *applying* given operation on these diagrams. Using this simple operation you can combine diagrams of simple functions into more complicated ones. ```apply``` is templated and takes instance of any binary operation in a form of a functor. We provide functors for basic [logical operations](./src/lib/diagrams/operators.hpp).
```
auto x1 = creator.just_var(1);
auto x2 = creator.just_var(2);
auto x3 = creator.just_var(3);
auto& m = manipulator; // just to keep the next line shorter
auto result = m.apply(m.apply(x1, AND{}, x2), OR{}, x3);
```
This however does not look very nice. First you need to manually create a diagram for each variable and then use nested calls to apply. That is why corresponding [operators](./src/lib/diagrams/bdd_manipulator.hpp) are overloaded for diagrams. Also creator overloads ```operator()``` that does the same thing as ```just_var```. With suitable name for the creator variable the above example can be also written like this:
```
auto& x     = creator;
auto result = (x(1) && x(2)) || x(3);
```
The only difference is that operators create a temporary instance of the manipulator. Effect on the performace is probably insignificant but in case you do some mission critical calculations it might be better to use one manipulator with direct calls to apply.  
In case you have a diagram in a variable and you won't need this variable after the call to the apply you can move the diagram from the variable and its memory will be released after finishing apply.
```
auto tmp    = creator.from_vector(truth_vector::from_string("01010111"));
auto& m     = manipulator; // just to keep the next line shorter
auto result = m.apply( std::move(tmp)
                     , XOR{}
                     , m.negate(creator.just_var(4)) ); 
```
Same code with operator syntax:
```
auto tmp    = creator.from_vector(truth_vector::from_string("01010111"));
auto& x     = creator;
auto result = tmp.move() ^ !x(4);
```
In these examples we have also showed another operation ```negate``` that performs logical not on the diagram. Notice that when using operators we have used ```.move()``` member function of the diagram to do the move. This is for technical reasons that are beyond the scope of this example. It is inconvenient and hopefully it will be replaced by uniform style in the future.

## Reliability theory
Applicability of decision diagrams in Reliability Theory was one of the motivations for creating this library. The following example shows how you can use NAME GOES HERE to analyze a system using its structure function.
```
auto tools    = bdd_tools {};
auto relTools = tools.reliability();
auto creator  = tools.creator();

auto sfVector = truth_vector::from_lambda([](auto&& x)
{
    return (x(0) && x(1)) || ((x(2) && x(3)) || x(4));
});
auto structureFunction = creator.from_vector(sfVector);
auto const ps = std::vector {0.9, 0.8, 0.7, 0.9, 0.9};

auto const A    = relTools.availability(structureFunction, ps);
auto const U    = relTools.unavailability(structureFunction, ps);
auto dpbds      = relTools.dpbds(std::move(structureFunction));
auto const SIs  = relTools.structural_importances(dpbds);
auto const BIs  = relTools.birnbaum_importances(dpbds, ps);
auto const CIs  = relTools.criticality_importances(dpbds, ps, U);
auto const FIs  = relTools.fussell_vesely_importances(dpbds, ps, U);
auto const MCVs = relTools.mcvs<std::bitset<5>>(std::move(dpbds));
```

## Licence
Will provide formal one really soon. For now, just do whatever you want with it, but please keep a reference to this page.