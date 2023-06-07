# Test support library
Simple library that contains types that we use in tests
Could be part of libteddy-utils in the future, then, it might have the following structure:
- libteddy-core
    - bdd
    - mdd
    - imdd
    - ifmdd
    - zdd (student bachelor/master)
    - add (student bachelor/master)

- libteddy-reliability
    - reliability tools

- libteddy-utils (libtsl now)
    - truth vector/table
    - truth vector/table reliability
    - series-parallel system generator

Filestructure:
- libteddy\
    - core\
    - reliability\
    - utils\
    - teddy.hpp