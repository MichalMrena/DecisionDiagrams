# Try find dependencies
find_package(
    librog REQUIRED
)

find_package(
    fmt REQUIRED
)

find_package(
    OpenMP
)

# Compile and link options
string(
    JOIN ";" TEDDY_COMPILE_OPTIONS
        "-Wall"
        "-Wextra"
        "-Wpedantic"
        "-Wconversion"
        "-Wsign-conversion"
        "-Wshadow"
)

string(
    JOIN ";" TEDDY_LINK_OPTIONS
        ""
)

## Enable sanitizers for Debug build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
    string(
        JOIN ";" TEDDY_COMPILE_OPTIONS
            ${TEDDY_COMPILE_OPTIONS}
            "-fsanitize=address"
            "-fsanitize=undefined"
    )

    string(
        JOIN ";" TEDDY_LINK_OPTIONS
            ${TEDDY_LINK_OPTIONS}
            "-fsanitize=address"
            "-fsanitize=undefined"
    )
endif()

# Targets

## libteddy-test-core
add_executable(
    libteddy-test-core
        core.test.cpp
        iterators.cpp
        expressions.cpp
)

target_link_libraries(
    libteddy-test-core
    PRIVATE teddy
    PRIVATE fmt::fmt
    PRIVATE rog::librog
)

target_compile_options(
    libteddy-test-core
    PRIVATE ${TEDDY_COMPILE_OPTIONS}
)

target_compile_features(
    libteddy-test-core
    PRIVATE cxx_std_20
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(
        libteddy-test-core
        PRIVATE OpenMP::OpenMP_CXX
    )

    target_compile_definitions(
        libteddy-test-core
        PRIVATE LIBTEDDY_TESTS_USE_OMP=1
    )
endif()

## libteddy-test-reliability
add_executable(
    libteddy-test-reliability
        reliability.test.cpp
        truth_table.cpp
        table_reliability.cpp
        expressions.cpp
        iterators.cpp
)

target_link_libraries(
    libteddy-test-reliability
    PRIVATE teddy
    PRIVATE fmt::fmt
    PRIVATE rog::librog
)

target_compile_options(
    libteddy-test-reliability
    PRIVATE ${TEDDY_COMPILE_OPTIONS}
)

target_compile_features(
    libteddy-test-reliability
    PRIVATE cxx_std_20
)

# Required standard and no compiler extensions for both targets
set_target_properties(
    libteddy-test-core
    libteddy-test-reliability
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS        NO
)