add_library(
    tsl
        expressions.cpp
        iterators.cpp
        truth_table.cpp
        truth_table_reliability.cpp
        pla-description.cpp
)

target_include_directories(
    tsl PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(tsl PRIVATE subproject-interface)
