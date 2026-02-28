cmake_minimum_required(VERSION 3.20)
project(cpp_serial VERSION 0.2.0 LANGUAGES CXX)

option(CPP_SERIAL_BUILD_TESTS "Build cpp_serial tests" ON)
option(CPP_SERIAL_ALLOW_FETCH "Allow FetchContent fallback for dependencies" ON)
option(CPP_SERIAL_BUILD_EXAMPLES "Build cpp_serial usage examples" ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

find_package(spdlog CONFIG QUIET)
if(NOT spdlog_FOUND)
  if(CPP_SERIAL_ALLOW_FETCH)
    FetchContent_Declare(
      spdlog
      GIT_REPOSITORY https://github.com/gabime/spdlog.git
      GIT_TAG v1.14.1
    )
    FetchContent_MakeAvailable(spdlog)
  else()
    message(FATAL_ERROR "spdlog not found. Install spdlog or enable CPP_SERIAL_ALLOW_FETCH.")
  endif()
endif()

add_library(cpp_serial
  src/serial_port.cpp
  src/default_backend.cpp
)

if(WIN32)
  target_sources(cpp_serial PRIVATE src/windows_serial_backend.cpp)
endif()

target_include_directories(cpp_serial
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(cpp_serial PUBLIC spdlog::spdlog)
target_compile_features(cpp_serial PUBLIC cxx_std_20)

if(MINGW)
  target_compile_options(cpp_serial PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CPP_SERIAL_BUILD_TESTS)
  enable_testing()

  add_executable(cpp_serial_tests
    tests/serial_port_test.cpp
  )

  target_link_libraries(cpp_serial_tests
    PRIVATE
      cpp_serial
  )

  add_test(NAME cpp_serial_tests COMMAND cpp_serial_tests)
endif()
if(CPP_SERIAL_BUILD_EXAMPLES)
  add_executable(cpp_serial_example examples/basic_example.cpp)
  target_link_libraries(cpp_serial_example PRIVATE cpp_serial)
endif()
