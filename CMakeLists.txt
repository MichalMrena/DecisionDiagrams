cmake_minimum_required(
    VERSION
        3.19
)

project(
        TeDDy
    VERSION
        3.0
    DESCRIPTION
        "C++ library for creating and manipulating decision diagrams"
    LANGUAGES
        CXX
)

include(GNUInstallDirs)

find_package(OpenMP)

#
# libteddy
#
add_library(
        libteddy
    INTERFACE
        libteddy/teddy.hpp
        libteddy/teddy_reliability.hpp
        libteddy/details/debug.hpp
        libteddy/details/diagram.hpp
        libteddy/details/diagram_manager.hpp
        libteddy/details/hash_tables.hpp
        libteddy/details/node.hpp
        libteddy/details/node_manager.hpp
        libteddy/details/node_pool.hpp
        libteddy/details/operators.hpp
        libteddy/details/pla_file.hpp
        libteddy/details/reliability_manager.hpp
        libteddy/details/types.hpp
        libteddy/details/utils.hpp
)

target_include_directories(
        libteddy
    INTERFACE
        .
)

#
# libteddy-test-core
#

add_executable(
    libteddy-test-core
    tests/core.cpp
)

target_include_directories(
        libteddy-test-core
    PUBLIC
        .
)

target_compile_options(
        libteddy-test-core
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
)

target_compile_features(
        libteddy-test-core
    PRIVATE
        cxx_std_20
)

set_target_properties(
        libteddy-test-core
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(
        libteddy-test-core
    PRIVATE
        OpenMP::OpenMP_CXX
    )
endif()

#
# libteddy-test-reliability
#

add_executable(
    libteddy-test-reliability
    tests/reliability.cpp
    tests/vector_function.hpp
)

target_include_directories(
        libteddy-test-reliability
    PUBLIC
        .
)

target_compile_options(
        libteddy-test-reliability
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
)

target_compile_features(
        libteddy-test-reliability
    PRIVATE
        cxx_std_20
)

set_target_properties(
        libteddy-test-reliability
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

#
# libteddy-install
#

