# 🧸 TeDDy

TeDDy is a C++ library for creation and manipulation of decision diagrams. It is being developed as a project at the [Faculty of Management Science and Informatics](https://www.fri.uniza.sk/en/), [University of Žilina](https://www.uniza.sk/index.php/en/) at the [Department of Informatics](https://ki.fri.uniza.sk/).

## Decision diagrams
This text assumes that the reader is familiar with decision diagrams to some extent. Our library supports [Binary Decision Diagrams](https://en.wikipedia.org/wiki/Binary_decision_diagram) (BDDs) and their generalization Multivalued Decision Diagrams (MDDs).

## How to use
Library is header only so it is easy to incorporate it in your project. All you need to do is to place contents of the [include](./include/) directory in your project files and include a header file in your code as is shown int the [examples](#Examples) below.  
If you would like to use the library in multiple projects and you are a Linux user you can use the following commands:
```bash
git clone "TODO"
cd DecisionDiagrams
sudo make install
```
This copies includes file to `/usr/local/include/` which should be visible by your compiler. You can specify custom path by: `sudo make -DINSTALL_PREFIX=<path> install`.  
To uninstall the library go to `DecisionDiagrams` directory and run `xargs rm < install_manifest.txt`.

### Compiling
TeDDy uses features from `C++20` so you might need to set your compiler for this version by using the `-std=c++20` flag for `clang++` and `g++` and `/std:c++latest` for MSVC.

## Examples
The library API is accesed via instance of a manager. TeDDy offeres four managers for different kinds of decision diagrams:
- **BDD manager** 
- **MDD manager** 
- **iMDD manager** 
- **ifMDD manager** 
```C++
#include "teddy/teddy.hpp"

int main()
{
    
}
```