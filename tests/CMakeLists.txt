# Try find dependencies
find_package(
    # NOTE: will be replaced with boost::test
    librog REQUIRED
)

find_package(
    fmt REQUIRED
)

find_package(
    OpenMP
)

# Targets

## libteddy-core-test
add_executable(
    libteddy-core-test
        core.test.cpp
)

target_link_libraries(
    libteddy-core-test
    PRIVATE teddy
    PRIVATE tsl
    PRIVATE fmt::fmt
    # NOTE: will be replaced with boost::test
    PRIVATE rog::librog
)

target_compile_options(
    libteddy-core-test
    PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    libteddy-core-test
    PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

target_compile_features(
    libteddy-core-test
    PRIVATE cxx_std_20
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(
        libteddy-core-test
        PRIVATE OpenMP::OpenMP_CXX
    )

    target_compile_definitions(
        libteddy-core-test
        PRIVATE LIBTEDDY_TESTS_USE_OMP=1
    )
endif()

## libteddy-reliability-test
add_executable(
    libteddy-reliability-test
        reliability.test.cpp
)

target_link_libraries(
    libteddy-reliability-test
    PRIVATE teddy
    PRIVATE tsl
    PRIVATE fmt::fmt
    # NOTE: will be replaced with boost::test
    PRIVATE rog::librog
)

target_compile_options(
    libteddy-reliability-test
    PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    libteddy-reliability-test
    PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

target_compile_features(
    libteddy-reliability-test
    PRIVATE cxx_std_20
)

# Required standard and no compiler extensions for both targets
set_target_properties(
    libteddy-core-test
    libteddy-reliability-test
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS        NO
)