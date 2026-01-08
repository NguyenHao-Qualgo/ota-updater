cmake_minimum_required(VERSION 3.10)
project(TegraUpdater VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Find Dependencies
find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 2. Create a library for shared logic 
# This prevents compiling UpdateManager.cpp twice and avoids linking conflicts
set(LIB_SOURCES
    UpdateManager.cpp
    # RawImageHandler is included in UpdateManager via header or 
    # should be compiled here if it's a separate .cpp
    RawImageHandler.cpp 
)

add_library(updater_logic STATIC ${LIB_SOURCES})
target_link_libraries(updater_logic PUBLIC nlohmann_json::nlohmann_json)

# 3. Main Executable
add_executable(tegra-updater main.cpp)
target_link_libraries(tegra-updater PRIVATE updater_logic)
target_compile_options(tegra-updater PRIVATE -Wall -Wextra -Wpedantic -O2)

# 4. Unit Tests
# We link against GTest::gtest_main so we don't need a main() in our test file
add_executable(unit_tests tests/test_update.cpp)
target_link_libraries(unit_tests 
    PRIVATE 
    updater_logic 
    GTest::GTest 
    GTest::gtest_main
)

enable_testing()
add_test(NAME AllTests COMMAND unit_tests)
