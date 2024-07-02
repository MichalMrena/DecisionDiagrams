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
        node.test.cpp
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

add_test(
    NAME    teddy-test-node
    COMMAND libteddy-test --run_test=node
)

add_test(
    NAME    teddy-test-core
    COMMAND libteddy-test --run_test=core
)

add_test(
    NAME    teddy-test-reliability
    COMMAND libteddy-test --run_test=reliability
)