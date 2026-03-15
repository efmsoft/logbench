cmake_minimum_required(VERSION 3.20)

project(logbench LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

option(BENCH_WITH_EASYLOGGING "Include easylogging++ (expected slower baseline)" ON)
option(BENCH_WITH_QUILL "Include quill" ON)

# Keep dependency build lean (no examples/tools/tests)
set(LOGME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LOGME_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LOGME_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(LOGME_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)


set(LOGME_SOURCE_DIR "" CACHE PATH "Path to local logme sources")

# Dependencies (pinned tags for reproducibility)
if (LOGME_SOURCE_DIR)
  message(STATUS "Using local logme from: ${LOGME_SOURCE_DIR}")

  if (NOT EXISTS "${LOGME_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
      "LOGME_SOURCE_DIR does not point to a valid logme source tree: ${LOGME_SOURCE_DIR}"
    )
  endif()

  add_subdirectory("${LOGME_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_deps/logme-build")
else()
  FetchContent_Declare(
    logme
    GIT_REPOSITORY https://github.com/efmsoft/logme.git
    GIT_TAG v2.4.13
  )
endif()

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.17.0
)

if (BENCH_WITH_QUILL)
  # Keep dependency build lean (no examples/tests)
  set(QUILL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(QUILL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(QUILL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    quill
    GIT_REPOSITORY https://github.com/odygrd/quill.git
    GIT_TAG v11.0.2
  )
endif()

if (BENCH_WITH_EASYLOGGING)
  FetchContent_Declare(
    easyloggingpp
    GIT_REPOSITORY https://github.com/amrayn/easyloggingpp.git
    GIT_TAG v9.96.7
  )
endif()

if (NOT LOGME_SOURCE_DIR)
  FetchContent_MakeAvailable(logme)
endif()

FetchContent_MakeAvailable(spdlog)
if (BENCH_WITH_QUILL)
  FetchContent_MakeAvailable(quill)
endif()
if (BENCH_WITH_EASYLOGGING)
  FetchContent_MakeAvailable(easyloggingpp)
endif()

add_executable(logbench
  src/main.cpp
)

target_compile_definitions(logbench PRIVATE BENCH_WITH_EASYLOGGING=0)

# Make sure logging is active in Release for logme
target_compile_definitions(logbench PRIVATE LOGME_INRELEASE)

# Avoid compile-time stripping of INFO in spdlog
# (spdlog uses SPDLOG_ACTIVE_LEVEL)
target_compile_definitions(logbench PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)

target_link_libraries(logbench PRIVATE logme spdlog::spdlog)

if (BENCH_WITH_QUILL)
  target_compile_definitions(logbench PRIVATE BENCH_WITH_QUILL=1)
  target_link_libraries(logbench PRIVATE quill::quill)
else()
  target_compile_definitions(logbench PRIVATE BENCH_WITH_QUILL=0)
endif()

if (WIN32)
  target_link_libraries(logbench PRIVATE ws2_32)
endif()

if (BENCH_WITH_EASYLOGGING)
  target_compile_definitions(logbench PRIVATE BENCH_WITH_EASYLOGGING=1)

  # easylogging++ is a headers+source library; add its single source file.
  target_sources(logbench PRIVATE
    ${easyloggingpp_SOURCE_DIR}/src/easylogging++.cc
  )
  target_include_directories(logbench PRIVATE
    ${easyloggingpp_SOURCE_DIR}/src
  )
  # Silence some warnings on MSVC for easylogging++
  if (MSVC)
    target_compile_definitions(logbench PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
  endif()
endif()

if (MSVC)
  target_compile_options(logbench PRIVATE /W4)

  # Generate PDB in Release (keep optimizations)
  target_compile_options(logbench PRIVATE
    $<$<CONFIG:Release>:/Zi>
  )
  target_link_options(logbench PRIVATE
    $<$<CONFIG:Release>:/DEBUG>
  )
else()
  target_compile_options(logbench PRIVATE -Wall -Wextra -Wpedantic)
endif()
