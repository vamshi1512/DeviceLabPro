cmake_minimum_required(VERSION 3.24)
project(DeviceLabPro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

add_library(devicelab_warnings INTERFACE)
target_compile_options(
  devicelab_warnings
  INTERFACE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

add_library(
  devicelab_core
  core/src/device_profile.cpp
  core/src/scenario.cpp
  core/src/simulator.cpp
)
add_library(devicelab::core ALIAS devicelab_core)

target_include_directories(
  devicelab_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_features(devicelab_core PUBLIC cxx_std_20)
target_link_libraries(devicelab_core PUBLIC devicelab_warnings)

add_executable(devicelab_cli apps/simulator_cli/main.cpp)
target_link_libraries(devicelab_cli PRIVATE devicelab_core)

if(BUILD_TESTING)
  include(FetchContent)

  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(
    devicelab_tests
    tests/test_state_machine.cpp
    tests/test_faults.cpp
    tests/test_scenarios.cpp
    tests/test_validation.cpp
  )
  target_link_libraries(devicelab_tests PRIVATE devicelab_core GTest::gtest_main)
  target_compile_definitions(
    devicelab_tests
    PRIVATE DEVICELAB_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
  )

  include(GoogleTest)
  gtest_discover_tests(devicelab_tests)
endif()
