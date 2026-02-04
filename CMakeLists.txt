cmake_minimum_required(VERSION 3.20)
project(flash_tool LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FLASH_TOOL_USE_SUBMODULES "Use bundled third_party submodules" OFF)
option(FLASH_TOOL_BUILD_TESTS "Build unit tests" OFF)

find_package(ZLIB REQUIRED)
find_package(LibArchive REQUIRED)

if (FLASH_TOOL_USE_SUBMODULES)
  add_subdirectory(third_party/nlohmann_json EXCLUDE_FROM_ALL)
else()
  find_package(nlohmann_json CONFIG REQUIRED)
endif()

add_library(flash_core
  src/fd.cpp
  src/file_reader.cpp
  src/partition_writer.cpp
  src/flasher.cpp
  src/signals.cpp
  src/config_parser.cpp
  src/manifest.cpp
  src/gzip_reader.cpp
  src/update_module.cpp
  src/archive_extractor.cpp
)

target_include_directories(flash_core PUBLIC include)
target_link_libraries(flash_core PUBLIC nlohmann_json::nlohmann_json ZLIB::ZLIB LibArchive::LibArchive)
add_executable(flash_tool src/main.cpp)
target_link_libraries(flash_tool PRIVATE flash_core)

include(CTest)
if (FLASH_TOOL_BUILD_TESTS)
  enable_testing()
  if (FLASH_TOOL_USE_SUBMODULES)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)
  else()
    find_package(GTest CONFIG REQUIRED)
  endif()
  add_subdirectory(tests)
endif()
