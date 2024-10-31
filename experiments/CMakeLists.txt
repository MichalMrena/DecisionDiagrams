# DPLDs
add_executable(dplds nanobench.cpp dplds.cpp)

target_link_libraries(dplds PRIVATE tsl teddy subproject-interface)

# State-Frequency
add_executable(state-frequency nanobench.cpp state_frequency.cpp)

target_link_libraries(state-frequency PRIVATE tsl teddy subproject-interface)

# Time-Probs
if(LIBTEDDY_SYMBOLIC_RELIABILITY)
    add_executable(time-probs nanobench.cpp time_probs.cpp)

    target_link_libraries(time-probs PRIVATE tsl teddy subproject-interface)
endif()

# Precise-Probs
if(LIBTEDDY_ARBITRARY_PRECISION)
    find_package(MPFR REQUIRED)

    add_executable(precise-probs precise-probs.cpp)

    target_link_libraries(precise-probs PRIVATE teddy tsl subproject-interface ${MPFR_LIBRARIES})
endif()