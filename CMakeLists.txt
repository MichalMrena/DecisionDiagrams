############
# Versions #
############
cmake_minimum_required(VERSION 3.19)

set(LIBTEDDY_VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/teddy-version")

if(NOT EXISTS ${LIBTEDDY_VERSION_FILE})
    message(WARNING "Missing project version file: ${LIBTEDDY_VERSION_FILE}")
    message(WARNING "Using dummy version 0.0.1")
    set(LIBTEDDY_VERSION 0.0.1)
else()
    file(READ ${LIBTEDDY_VERSION_FILE} LIBTEDDY_VERSION)
endif()

project(
    TeDDy
    VERSION     ${LIBTEDDY_VERSION}
    DESCRIPTION "C++ library for creating and manipulating decision diagrams"
    LANGUAGES   CXX
)

###########
# Modules #
###########
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

###############
#    TeDDy    #
###############
add_subdirectory(libteddy)

###############
# Subprojects #
###############
option(LIBTEDDY_USE_LIBCXX        "Use libc++"            OFF)
option(LIBTEDDY_USE_SANITIZERS    "Enable asan and ubsan" OFF)
option(LIBTEDDY_USE_MOLD          "Use Mold linker"       OFF)
option(LIBTEDDY_BUILD_TESTS       "Build tests"           OFF)
option(LIBTEDDY_BUILD_EXAMPLES    "Build examples"        OFF)
option(LIBTEDDY_BUILD_EXPERIMENTS "Build experiments"     OFF)

add_library(subproject-interface INTERFACE)

target_compile_features(subproject-interface INTERFACE cxx_std_20)

target_compile_options(
    subproject-interface INTERFACE
        "-Wall"
        "-Wextra"
        "-Wpedantic"
        "-Wconversion"
        "-Wsign-conversion"
        "-Wshadow"
)

# Mold linker
if(LIBTEDDY_USE_MOLD)
    message("=== Using Mold linker")

    target_link_options(subproject-interface INTERFACE "-fuse-ld=mold")
endif()

# libc++
if(LIBTEDDY_USE_LIBCXX AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message("=== Using libc++ and libc++abi")

    target_compile_options(subproject-interface INTERFACE "-stdlib=libc++")

    target_link_options(
        subproject-interface INTERFACE
            "-stdlib=libc++"
            "-lc++abi"
    )
endif()

# asan and ubsan
if(LIBTEDDY_USE_SANITIZERS AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message("=== Asan and Ubsan enabled")

    target_compile_options(
        subproject-interface INTERFACE
            "-fsanitize=address"
            "-fsanitize=undefined"
    )

    target_link_options(
        subproject-interface INTERFACE
            "-fsanitize=address"
            "-fsanitize=undefined"
    )
endif()

#########
# Tests #
#########
if(LIBTEDDY_BUILD_TESTS)
    # NOTE: will be moved to libteddy-utils in the future
    add_subdirectory(libtsl)

    enable_testing()
    add_subdirectory(tests)
endif()

############
# Examples #
############
if(LIBTEDDY_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

###############
# Experiments #
###############
if(LIBTEDDY_BUILD_EXPERIMENTS)
    # NOTE: will be moved to standalone repo
    add_subdirectory(experiments)
endif()