add_executable(flash_tool_tests
  test_file_reader.cpp
  test_partition_writer.cpp
  test_fd.cpp
  test_manifest.cpp
  test_gzip_reader.cpp
  test_update_module.cpp
  test_archive_extractor.cpp
)

target_link_libraries(flash_tool_tests PRIVATE
  flash_core
  GTest::gtest
  GTest::gtest_main
)

target_include_directories(flash_tool_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}
)

include(GoogleTest)

# Only auto-run discovery when building for host (not cross compiling)
if (NOT CMAKE_CROSSCOMPILING)
  gtest_discover_tests(flash_tool_tests)
endif()
