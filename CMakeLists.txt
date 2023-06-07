# Project

cmake_minimum_required(
    VERSION 3.19
)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/teddy-version")
    message(FATAL_ERROR "Missing project version file!")
endif()

file(
    READ "teddy-version" TEDDY_VERSION
)

project(
    TeDDy
    VERSION     ${TEDDY_VERSION}
    DESCRIPTION "C++ library for creating and manipulating decision diagrams"
    LANGUAGES   CXX
)

# Library target
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

# Use libc++ if requested and compiling with clang

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND LIBTEDDY_USE_CXX)
    message("Using libc++ and libc++abi")
    set(
        CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -stdlib=libc++"
    )
    set(
        CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi"
    )
endif()

# Build tests and examples if requested

if(LIBTEDDY_BUILD_TESTS)
    add_subdirectory(
        tests
    )
endif()

if(LIBTEDDY_BUILD_EXAMPLES)
    add_subdirectory(
        examples
    )
endif()

# Install config

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