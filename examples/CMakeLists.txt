# Example 1
add_executable(
    libteddy-example-1 example_1.cpp
)

target_link_libraries(
    libteddy-example-1 PRIVATE teddy
)

# Example 2
add_executable(
    libteddy-example-2 example_2.cpp
)

target_link_libraries(
    libteddy-example-2 PRIVATE teddy
)