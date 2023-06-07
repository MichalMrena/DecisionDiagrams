add_library(
    tsl
        expressions.cpp
        iterators.cpp
        table_reliability.cpp
        truth_table.cpp
)

target_include_directories(
    tsl PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_options(
    tsl
    PRIVATE ${LIBTEDDY_COMPILE_OPTIONS}
)

target_link_options(
    tsl
    PRIVATE ${LIBTEDDY_LINK_OPTIONS}
)

target_compile_features(
    tsl
    PRIVATE cxx_std_20
)

set_target_properties(
    tsl
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS        NO
)