# DPLDs
add_executable(
    dplds nanobench.cpp dplds.cpp
)

target_link_libraries(
    dplds PRIVATE tsl
)

target_link_libraries(
    dplds PRIVATE teddy
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

# time-probs

add_executable(
    time-probs nanobench.cpp time_probs.cpp
)

target_link_libraries(
    time-probs PRIVATE tsl
)

target_link_libraries(
    time-probs PRIVATE teddy
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