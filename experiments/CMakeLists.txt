add_executable(dplds nanobench.cpp dplds.cpp)
target_link_libraries(dplds PRIVATE tsl)
target_link_libraries(dplds PRIVATE teddy)
target_include_directories(dplds PRIVATE ${PROJECT_SOURCE_DIR}/lib)