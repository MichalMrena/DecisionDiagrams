# Example 1
add_executable(
    example-1 example_1.cpp
)

target_link_libraries(
    example-1 PRIVATE teddy
)

target_compile_options(
    example-1 PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    example-1 PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

# Example 2
add_executable(
    example-2 example_2.cpp
)

target_link_libraries(
    example-2 PRIVATE teddy
)

target_compile_options(
    example-2 PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    example-2 PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

# N-Queens
add_executable(
    example-n-queens n_queens.cpp
)

target_link_libraries(
    example-n-queens PRIVATE teddy
)

target_compile_options(
    example-n-queens PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    example-n-queens PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

# Time reliability
add_executable(
    example-time-reliability time_reliability.cpp
)

target_link_libraries(
    example-time-reliability PRIVATE teddy
)

target_compile_options(
    example-time-reliability PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    example-time-reliability PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

# AST experiment
add_executable(
    example-ast experiment.cpp
)

target_link_libraries(
    example-ast PRIVATE teddy
)

target_compile_options(
    example-ast PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    example-ast PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)