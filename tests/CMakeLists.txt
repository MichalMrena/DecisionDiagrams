find_package(
    Boost
    COMPONENTS unit_test_framework REQUIRED
)

find_package(
    fmt REQUIRED
)

add_executable(
    libteddy-test
        main.cpp
        core.test.cpp
        reliability.test.cpp
)

target_link_libraries(
    libteddy-test
    PRIVATE teddy
    PRIVATE tsl
    PRIVATE fmt::fmt
    PRIVATE ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

target_compile_options(
    libteddy-test
    PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    libteddy-test
    PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

target_compile_features(
    libteddy-test
    PRIVATE cxx_std_20
)

set_target_properties(
    libteddy-test
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS        NO
)