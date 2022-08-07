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
)

target_include_directories(
        libteddy
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# TODO
add_subdirectory(
    tests
)

#
# libteddy-test-core
#

add_executable(
    libteddy-test-core
    tests/core.cpp
)

target_link_libraries(
        libteddy-test-core
    PRIVATE
        libteddy
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

target_link_libraries(
        libteddy-test-reliability
    PRIVATE
        libteddy
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

install(
    DIRECTORY
        libteddy
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}
)