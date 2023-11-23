# Readme
add_executable(
    readme readme.cpp
)

target_link_libraries(
    readme PRIVATE teddy
)

target_compile_options(
    readme PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    readme PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

# N-Queens
add_executable(
    n-queens n_queens.cpp
)

target_link_libraries(
    n-queens PRIVATE teddy
)

target_compile_options(
    n-queens PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    n-queens PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

if(LIBTEDDY_SYMBOLIC_RELIABILITY)
    # Time reliability
    add_executable(
        time-reliability time_reliability.cpp
    )

    target_link_libraries(
        time-reliability PRIVATE teddy
    )

    target_compile_options(
        time-reliability PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
    )

    target_link_options(
        time-reliability PRIVATE ${LIBTEDDY_LINK_OPTIONS}
    )
endif()

# Soft-X-example
add_executable(
    soft-x soft_x.cpp
)

target_link_libraries(
    soft-x PRIVATE teddy
)

target_compile_options(
    soft-x PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    soft-x PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)