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