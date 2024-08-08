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

#################
# TeDDy library #
#################
option(LIBTEDDY_SYMBOLIC_RELIABILITY "Enable symbolic expressions" OFF)
option(LIBTEDDY_ARBITRARY_PRECISION  "Enable GMP library"          OFF)
option(LIBTEDDY_VERBOSE              "Enable verbose output"       OFF)
option(LIBTEDDY_COLLECT_STATS        "Enable stat collection"      OFF)

add_library(teddy INTERFACE)

add_library(teddy::teddy ALIAS teddy)

target_include_directories(
    teddy INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(teddy INTERFACE cxx_std_20)

########################
# Symbolic reliability #
########################
if(LIBTEDDY_SYMBOLIC_RELIABILITY)
    find_package(CLN REQUIRED)

    find_package(GINAC REQUIRED)

    target_compile_definitions(teddy INTERFACE LIBTEDDY_SYMBOLIC_RELIABILITY)

    target_link_libraries(teddy INTERFACE GINAC::GINAC)

    target_link_libraries(teddy INTERFACE CLN::CLN)
endif()

#######################
# Arbitrary precision #
#######################
if(LIBTEDDY_ARBITRARY_PRECISION)
    find_package(GMP REQUIRED)

    find_package(GMPXX REQUIRED)

    target_compile_definitions(teddy INTERFACE LIBTEDDY_ARBITRARY_PRECISION)

    target_link_libraries(teddy INTERFACE GMP::GMP GMP::GMPXX)
endif()

###########
# Verbose #
###########
if(LIBTEDDY_VERBOSE)
    target_compile_definitions(teddy INTERFACE LIBTEDDY_VERBOSE)
endif()

#########
# Stats #
#########
if(LIBTEDDY_COLLECT_STATS)
    target_compile_definitions(teddy INTERFACE LIBTEDDY_COLLECT_STATS)
endif()

################
# Installation #
################
include(GNUInstallDirs)

include(CMakePackageConfigHelpers)

install(
    TARGETS teddy
    EXPORT  ${PROJECT_NAME}_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

write_basic_package_version_file(
    "${PROJECT_NAME}ConfigVersion.cmake"
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
)

install(
    EXPORT      ${PROJECT_NAME}_Targets
    FILE        ${PROJECT_NAME}Targets.cmake
    NAMESPACE   teddy::
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
)

install(
    FILES
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
)

install(
    DIRECTORY   libteddy
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

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