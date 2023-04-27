cmake_minimum_required(
    VERSION 3.19
)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/version")
    message(FATAL_ERROR "Missing project version file!")
endif()

file(
    READ "version" TEDDY_VERSION
)

project(
    TeDDy
    VERSION     ${TEDDY_VERSION}
    DESCRIPTION "C++ library for creating and manipulating decision diagrams"
    LANGUAGES   CXX
)

add_library(
    libteddy INTERFACE
)

target_include_directories(
    libteddy INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_subdirectory(
    tests
)

add_subdirectory(
    misc
)

include(
    GNUInstallDirs
)

install(
    DIRECTORY   libteddy
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)