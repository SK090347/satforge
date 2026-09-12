cmake_minimum_required(VERSION 3.16)
project(satforge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(satforge_core
  src/parser.cpp
  src/solver.cpp
)
target_include_directories(satforge_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_options(satforge_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(satforge src/main.cpp)
target_link_libraries(satforge PRIVATE satforge_core)

enable_testing()
add_executable(test_solver tests/cpp/test_solver.cpp)
target_link_libraries(test_solver PRIVATE satforge_core)
add_test(NAME cpp_solver COMMAND test_solver)

install(TARGETS satforge RUNTIME DESTINATION bin)
