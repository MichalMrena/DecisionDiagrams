find_package(
    GMP
    REQUIRED
)

add_executable(
    experiment
        counter.cpp
        experiment.cpp
        generators.cpp
        trees.cpp
)

target_link_libraries(
        experiment
    PRIVATE
        libteddy
        ${GMP_C_LIBRARIES}
        ${GMP_CXX_LIBRARIES}
)

target_compile_options(
        experiment
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
)

target_compile_features(
        experiment
    PRIVATE
        cxx_std_20
)

set_target_properties(
        experiment
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)