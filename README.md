# Binary decision diagrams
Welcome visitor. This repository contains a C++ library for creating and manipulating Ordered Binary Decision Diagrams. 
Library is still in the developement so everything is quite messy. This is public just because I want to show what it can do so far.

## Creating diagram
Diagram can be constructed in two different ways.

### From all inputs
As the name suggests this algorithm [?] iterates over all posible inputs of a function and builds the diagram in the process. Three function representations are supported by default. Examples of their use are in the following functions inside ```main.cpp```:

**create_from_truth_table();**  
Truth table is loaded from a simple text file in the following format:
```
000000 0
000001 0
000010 0
000011 1
...
```
There are *n* values representing values of *n* variables (indexed from left to right from n to 0) followed by value of the function correspondig to given input.

**create_from_lambda();**  
This way you can define the function programatically using logical operators of the language. ```lambda_bool_f```'s constructor has two parameters. First is the number of variables and the second is lambda function itself. Type of the the lambda function must match the type of the lambda in the example. Name of the parameter is up to you of course but it looks nice with *x*.

**create_from_pla_file();**  
This is just for the verification of the method described below.  

Examples given above use generic method ```bdd_creator::create_from```. You can use this method to create a diagram from your custom representation of a Boolean function. You just need to provide the second and third template parameter. They are simple [functors](https://stackoverflow.com/questions/356950/what-are-c-functors-and-their-uses) that are used to get a value of the function from its representation. Declarations of the ones that are used by default can be found in ```./src/bdd/bool_function.hpp```.

### By merging smaller diagrams
This method first creates small and simple diagrams and then merges them into the single, more complex one. Again there are different ways how to represent the input function. Examples are given in the following functions:  
  
**build_from_pla_file()**  
Pla is a specific format of a file that can store multiple Boolean functions. At the begining of the file **.i** denotes the number of variables, **.o** is the number of functions and **.p** is the number of following lines. Each line defines multiple functions whose values are **0** or **1** (defined in the right part of the line) for the variable values on the left. Variables and functions are indexed from left to right from 0 to n. - denotes that the function does not depent on the value of that variable.
```
.i 45   
.o 45   
.p 206  
---------0--------00011-----010-------0000001 000000000010110010000010000000000010010000000
---------0--------0-011-----010-------0000000 000000000000100001000010000010000000010000000
...
```
Above example uses method ```bdds_from_pla::create```. This method first creates diagrams for each line and then combines them into one for each function. Result is a vector of diagrams. This approach can be parallelized in multiple places. However diagram construction times were not a problem for functions whose diagrams are not exponential in the number of variables *(I will write about this later below)*.