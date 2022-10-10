#
# libteddy-test-core
#

add_executable(
    libteddy-test-core
    core.cpp
    iterators.cpp
    expressions.cpp
)

target_link_libraries(
        libteddy-test-core
    PRIVATE
        libteddy
)

target_compile_options(
        libteddy-test-core
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
)

target_compile_features(
        libteddy-test-core
    PRIVATE
        cxx_std_20
)

set_target_properties(
        libteddy-test-core
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

find_package(OpenMP)

if(OpenMP_CXX_FOUND)

    target_link_libraries(
        libteddy-test-core
    PRIVATE
        OpenMP::OpenMP_CXX
    )

    target_compile_definitions(
            libteddy-test-core
        PRIVATE
            LIBTEDDY_TESTS_USE_OMP=1
    )

endif()

find_package(librog REQUIRED)

target_link_libraries(
    libteddy-test-core
PRIVATE
    rog::librog
)

#
# libteddy-test-reliability
#

add_executable(
    libteddy-test-reliability
    reliability.cpp
    truth_table.cpp
    table_reliability.cpp
    expressions.cpp
    iterators.cpp
)

target_link_libraries(
        libteddy-test-reliability
    PRIVATE
        libteddy
)

target_link_libraries(
    libteddy-test-reliability
PRIVATE
    rog::librog
)

target_compile_options(
        libteddy-test-reliability
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
)

target_compile_features(
        libteddy-test-reliability
    PRIVATE
        cxx_std_20
)

set_target_properties(
        libteddy-test-reliability
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)