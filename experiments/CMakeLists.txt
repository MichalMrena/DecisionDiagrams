# DPLDs
add_executable(
    dplds nanobench.cpp dplds.cpp
)

target_link_libraries(
    dplds PRIVATE tsl teddy
)

target_include_directories(
    dplds PRIVATE ${PROJECT_SOURCE_DIR}/lib
)

target_compile_options(
    dplds PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    dplds PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

# State-Frequency
add_executable(
    state-frequency nanobench.cpp state_frequency.cpp
)

target_link_libraries(
    state-frequency PRIVATE tsl teddy
)

target_include_directories(
    state-frequency PRIVATE ${PROJECT_SOURCE_DIR}/lib
)

target_compile_options(
    state-frequency PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    state-frequency PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

# Time-Probs
if(LIBTEDDY_SYMBOLIC_RELIABILITY)
    add_executable(
        time-probs nanobench.cpp time_probs.cpp
    )

    target_link_libraries(
        time-probs PRIVATE tsl teddy
    )

    target_include_directories(
        time-probs PRIVATE ${PROJECT_SOURCE_DIR}/lib
    )

    target_compile_options(
        time-probs PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
    )

    target_link_options(
        time-probs PRIVATE ${LIBTEDDY_LINK_OPTIONS}
    )
endif()

# Dependent probabilities
add_executable(
    dep-prob dep_prob.cpp
)

target_link_libraries(
    dep-prob PRIVATE teddy
)

target_compile_options(
    dep-prob PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    dep-prob PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)