cmake_minimum_required(VERSION 3.20)
project(flash_tool LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(flash_core
  src/fd.cpp
  src/file_reader.cpp
  src/partition_writer.cpp
  src/flasher.cpp
  src/signals.cpp
)

target_include_directories(flash_core PUBLIC include)

add_executable(flash_tool src/main.cpp)
target_link_libraries(flash_tool PRIVATE flash_core)

include(CTest)
option(FLASH_TOOL_BUILD_TESTS "Build unit tests" ON)

if (FLASH_TOOL_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()