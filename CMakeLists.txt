# Project

cmake_minimum_required(
    VERSION 3.19
)

set(
    LIBTEDDY_VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/teddy-version"
)

if(NOT EXISTS ${LIBTEDDY_VERSION_FILE})
    message(
        FATAL_ERROR "Missing project version file: ${LIBTEDDY_VERSION_FILE}"
    )
endif()

file(
    READ ${LIBTEDDY_VERSION_FILE} LIBTEDDY_VERSION
)

project(
    TeDDy
    VERSION     ${LIBTEDDY_VERSION}
    DESCRIPTION "C++ library for creating and manipulating decision diagrams"
    LANGUAGES   CXX
)

# TeDDy library target
add_library(
    teddy INTERFACE
)

target_include_directories(
    teddy INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(
    teddy
    INTERFACE cxx_std_20
)

set_target_properties(
    teddy
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS        NO
)

# TeDDy library install

include(
    GNUInstallDirs
)

include(
    CMakePackageConfigHelpers
)

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

# Internal subprojects

## Common compile and link options for subprojects
string(
    JOIN ";" LIBTEDDY_COMPILE_OPTIONS
        "-Wall"
        "-Wextra"
        "-Wpedantic"
        "-Wconversion"
        "-Wsign-conversion"
        "-Wshadow"
)

string(
    JOIN ";" LIBTEDDY_LINK_OPTIONS
        ""
)

## Use libc++ if requested and compiling with clang
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND LIBTEDDY_USE_LIBCXX)
    message(
        "=== Using libc++ and libc++abi"
    )

    string(
        JOIN ";" LIBTEDDY_COMPILE_OPTIONS
            "-stdlib=libc++"
    )

    string(
        JOIN ";" LIBTEDDY_LINK_OPTIONS
            "-stdlib=libc++"
            "-lc++abi"
    )
endif()

## Enable sanitizers for Debug build
if(CMAKE_BUILD_TYPE STREQUAL Debug AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(
        "=== Asan a Ubsan enabled for Debug build"
    )

    string(
        JOIN ";" LIBTEDDY_COMPILE_OPTIONS
            ${LIBTEDDY_COMPILE_OPTIONS}
            "-fsanitize=address"
            "-fsanitize=undefined"
    )

    string(
        JOIN ";" LIBTEDDY_LINK_OPTIONS
            ${LIBTEDDY_LINK_OPTIONS}
            "-fsanitize=address"
            "-fsanitize=undefined"
    )
endif()

## Build tests and examples if requested

if(LIBTEDDY_BUILD_TESTS)
    # NOTE: will be moved to libteddy-utils in the future
    add_subdirectory(libtsl)

    add_subdirectory(tests)
endif()

if(LIBTEDDY_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(LIBTEDDY_BUILD_EXPERIMENTS)
    add_subdirectory(experiments)
endif()