# Dependencies
Files `cmake/modules/Find*.cmake` work in the following way:
1. try to find path to *linkable* file (`find_library`)
2. try to find directory path containing given header (`find_path`)
3. throw error if the above lookup was not succesfull
(`find_package_handle_standard_args`)
4. define library target with given paths as properties
5. the target can be used with `target_link_libraries`  

If there is a problem with steps 1 and 2, one can set the
variables manually instead of using `find_*` functions
-- just make sure that the path/file exist. For example in case of GiNaC:
```
set(GINAC_INCLUDES /usr/local/include/ginac)
set(GINAC_LIBRARY /usr/local/lib/libginac.so)
```