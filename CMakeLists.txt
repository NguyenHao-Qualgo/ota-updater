
cmake_minimum_required(VERSION 3.16)
project(ota_updater LANGUAGES CXX VERSION 0.1.0)

option(ENABLE_ZSTD       "Enable libzstd for .zst decompression"        OFF)
option(ENABLE_LIBARCHIVE "Enable libarchive for tar extraction"         OFF)
option(ENABLE_OPENSSL    "Enable OpenSSL for signature verification"    OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (ENABLE_ZSTD)
  find_package(ZSTD QUIET)
endif()

if (ENABLE_LIBARCHIVE)
  find_package(LibArchive QUIET)
endif()

if (ENABLE_OPENSSL)
  find_package(OpenSSL QUIET)
endif()

set(OTA_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/artifact_handler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decompressors.h
    ${CMAKE_CURRENT_SOURCE_DIR}/errors.h
    ${CMAKE_CURRENT_SOURCE_DIR}/file_reader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/health_check.h
    ${CMAKE_CURRENT_SOURCE_DIR}/journal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/manifest_loader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/planner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/slot_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tar_extractor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/update_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/verifier.h
)

set(OTA_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/update_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/boot_control.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/file_reader.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/block_device_writer.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/decompressors.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/tar_extractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/verifier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slot_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/journal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/planner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/manifest_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/health_check.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/artifact_handler.cpp
)

add_library(ota_updater STATIC ${OTA_SOURCES})

target_include_directories(ota_updater PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (ENABLE_ZSTD AND ZSTD_FOUND)
  target_compile_definitions(ota_updater PRIVATE HAVE_ZSTD=1)
  target_link_libraries(ota_updater PRIVATE ZSTD::ZSTD)
endif()

if (ENABLE_LIBARCHIVE AND LibArchive_FOUND)
  target_compile_definitions(ota_updater PRIVATE HAVE_LIBARCHIVE=1)
  target_link_libraries(ota_updater PRIVATE LibArchive::LibArchive)
endif()

if (ENABLE_OPENSSL AND OpenSSL_FOUND)
  target_compile_definitions(ota_updater PRIVATE HAVE_OPENSSL=1)
  target_link_libraries(ota_updater PRIVATE OpenSSL::Crypto OpenSSL::SSL)
endif()

option(BUILD_DEMO "Build the simple demo executable" ON)
if (BUILD_DEMO AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/apps/main.cpp")
  add_executable(ota_demo ${CMAKE_CURRENT_SOURCE_DIR}/apps/main.cpp)
  target_link_libraries(ota_demo PRIVATE ota_updater)
endif()

install(TARGETS ota_updater
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

install(FILES ${OTA_PUBLIC_HEADERS} DESTINATION include/ota-updater)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ota_updaterConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
