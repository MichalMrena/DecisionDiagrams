add_executable(
    libteddy-example-1
    example_1.cpp
)

target_link_libraries(
    libteddy-example-1
    PRIVATE libteddy
)

target_compile_options(
    libteddy-example-1
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
)

target_compile_features(
    libteddy-example-1
    PRIVATE cxx_std_20
)

set_target_properties(
    libteddy-example-1
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)