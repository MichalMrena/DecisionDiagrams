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

add_subdirectory(
    tests
)

install(
    DIRECTORY
        libteddy
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}
)