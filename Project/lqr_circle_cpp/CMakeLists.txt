cmake_minimum_required(VERSION 3.16)
project(lqr_circle_sim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(lqr_sim
    src/circle_trajectory.cpp
    src/lqr_controller.cpp
    src/point_mass.cpp
    src/simulation.cpp)
target_include_directories(lqr_sim PUBLIC include)

if(MSVC)
  target_compile_options(lqr_sim PRIVATE /W4 /permissive-)
else()
  target_compile_options(lqr_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(lqr_circle src/main.cpp)
target_link_libraries(lqr_circle PRIVATE lqr_sim)

include(CTest)
if(BUILD_TESTING)
  add_executable(lqr_tests tests/test_main.cpp)
  target_link_libraries(lqr_tests PRIVATE lqr_sim)
  add_test(NAME lqr_tests COMMAND lqr_tests)
endif()
