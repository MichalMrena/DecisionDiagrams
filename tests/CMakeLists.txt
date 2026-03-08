find_package(Boost COMPONENTS unit_test_framework REQUIRED)

find_package(fmt REQUIRED)

add_executable(
    libteddy-test
        main.cpp
        core.test.cpp
        io.test.cpp
        node.test.cpp
        reliability.test.cpp
        tools.test.cpp
)

target_link_libraries(
    libteddy-test
    PRIVATE teddy
    PRIVATE tsl
    PRIVATE ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    PRIVATE subproject-interface
    PRIVATE fmt::fmt
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


add_executable(
    libteddy-test-zdd
        zdd.test.cpp
)

target_compile_options(libteddy-test-zdd PRIVATE -fdiagnostics-color=always)

target_link_libraries(
    libteddy-test-zdd
    PRIVATE teddy
    PRIVATE ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    PRIVATE fmt::fmt
)
