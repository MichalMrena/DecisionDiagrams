add_library(tsl STATIC
  expressions.cpp
  generators.cpp
  iterators.cpp
  pla-description.cpp
  system-description.cpp
  truth-table-reliability.cpp
  truth-table.cpp
)

target_include_directories(tsl PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(tsl PRIVATE teddy subproject-interface)
