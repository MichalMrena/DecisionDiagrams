add_executable(
    experiment
    experiment.cpp
)

target_link_libraries(
        experiment
    PRIVATE
        libteddy
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