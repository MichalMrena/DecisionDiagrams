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
endif()

#
# libteddy-test-reliability
#

add_executable(
    libteddy-test-reliability
    reliability.cpp
)

target_link_libraries(
        libteddy-test-reliability
    PRIVATE
        libteddy
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