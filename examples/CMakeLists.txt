# Readme
add_executable(readme readme.cpp)

target_link_libraries(readme PRIVATE teddy subproject-interface)

# N-Queens
add_executable(n-queens n_queens.cpp)

target_link_libraries(n-queens PRIVATE teddy subproject-interface)

# Time reliability
if(LIBTEDDY_SYMBOLIC_RELIABILITY)
    add_executable(time-reliability time_reliability.cpp)

    target_link_libraries(time-reliability PRIVATE teddy subproject-interface)
endif()

# Soft-X-example
add_executable(soft-x soft_x.cpp)

target_link_libraries(soft-x PRIVATE teddy subproject-interface)