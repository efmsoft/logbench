cmake_minimum_required(VERSION 3.21)

project(logbench LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CTest)

option(USE_FMT "Build fmt-style paths with external fmt and switch libraries that support it" OFF)
option(BENCH_WITH_EASYLOGGING "Include easylogging++" ON)
option(BENCH_WITH_QUILL "Include quill" ON)
option(BENCH_WITH_BOOST_LOG "Include Boost.Log" ON)
option(BENCH_WITH_G3LOG "Include g3log" ON)
option(BENCH_WITH_PLOG "Include plog" ON)

set(LOGME_SOURCE_DIR "" CACHE PATH "Path to local logme source tree")

# Keep third-party subprojects lean and avoid pulling their tests/examples.
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST OFF CACHE BOOL "" FORCE)

# Keep dependency builds lean.
set(LOGME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LOGME_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LOGME_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(LOGME_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_WARNINGS OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)

set(QUILL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QUILL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QUILL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

set(ADD_G3LOG_UNIT_TEST OFF CACHE BOOL "" FORCE)
set(ADD_G3LOG_BENCH_PERFORMANCE OFF CACHE BOOL "" FORCE)
set(ADD_FATAL_EXAMPLE OFF CACHE BOOL "" FORCE)
set(G3_SHARED_LIB OFF CACHE BOOL "" FORCE)
set(G3_SHARED_RUNTIME OFF CACHE BOOL "" FORCE)

if (USE_FMT)
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 12.1.0
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(fmt)

  set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
  set(QUILL_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
  set(LOGME_FMT_FORMAT AUTO CACHE STRING "" FORCE)
  set(LOGME_STD_FORMAT OFF CACHE STRING "" FORCE)
endif()

if (LOGME_SOURCE_DIR)
  if (NOT EXISTS "${LOGME_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "LOGME_SOURCE_DIR does not point to a valid source tree: ${LOGME_SOURCE_DIR}")
  endif()
  add_subdirectory("${LOGME_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_deps/logme-build")
else()
  FetchContent_Declare(
    logme
    GIT_REPOSITORY https://github.com/efmsoft/logme.git
    GIT_TAG v2.4.16
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(logme)
endif()

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.17.0
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(spdlog)

if (BENCH_WITH_QUILL)
  FetchContent_Declare(
    quill
    GIT_REPOSITORY https://github.com/odygrd/quill.git
    GIT_TAG v10.0.1
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(quill)
endif()

if (BENCH_WITH_EASYLOGGING)
  FetchContent_Declare(
    easyloggingpp
    GIT_REPOSITORY https://github.com/amrayn/easyloggingpp.git
    GIT_TAG v9.96.7
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(easyloggingpp)
endif()

if (BENCH_WITH_G3LOG)
  FetchContent_Declare(
    g3log
    GIT_REPOSITORY https://github.com/KjellKod/g3log.git
    GIT_TAG 2.6
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(g3log)
endif()

if (BENCH_WITH_PLOG)
  FetchContent_Declare(
    plog
    GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
    GIT_TAG 1.1.11
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(plog)

  if (NOT TARGET plog::plog)
    add_library(plog::plog INTERFACE IMPORTED)
    set_target_properties(plog::plog PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${plog_SOURCE_DIR}/include")
  endif()
endif()

if (BENCH_WITH_BOOST_LOG)
  # Boost 1.90.0 is the latest stable release listed on boost.org at the moment; 1.91.0 is beta-only.
  # Official release archives do not ship the CMake entrypoint, so use the Git superproject tag.
  set(BOOST_INCLUDE_LIBRARIES log CACHE STRING "" FORCE)
  set(BOOST_ENABLE_CMAKE ON CACHE BOOL "" FORCE)

  FetchContent_Declare(
    boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.91.0
    GIT_SHALLOW FALSE
    GIT_SUBMODULES_RECURSE TRUE)
  FetchContent_MakeAvailable(boost)
endif()

add_executable(logbench
  src/main.cpp
  src/bench_runner.cpp
  src/bench_util.cpp
  src/cli.cpp
  src/driver_registry.cpp
  src/filter.cpp
  src/results_printer.cpp
  src/modes/measure_runner.cpp
  src/drivers/logme_driver.cpp
  src/drivers/spdlog_driver.cpp
  src/drivers/quill_driver.cpp
  src/drivers/easylogging_driver.cpp
  src/drivers/boost_log_driver.cpp
  src/drivers/g3log_driver.cpp
  src/drivers/plog_driver.cpp)

target_compile_definitions(logbench PRIVATE LOGME_INRELEASE)
target_compile_definitions(logbench PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
if (MSVC)
  target_compile_definitions(logbench PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

target_compile_definitions(logbench PRIVATE
  $<$<BOOL:${USE_FMT}>:USE_FMT>
  $<$<BOOL:${BENCH_WITH_EASYLOGGING}>:BENCH_WITH_EASYLOGGING=1>
  $<$<NOT:$<BOOL:${BENCH_WITH_EASYLOGGING}>>:BENCH_WITH_EASYLOGGING=0>
  $<$<BOOL:${BENCH_WITH_QUILL}>:BENCH_WITH_QUILL=1>
  $<$<NOT:$<BOOL:${BENCH_WITH_QUILL}>>:BENCH_WITH_QUILL=0>
  $<$<BOOL:${BENCH_WITH_BOOST_LOG}>:BENCH_WITH_BOOST_LOG=1>
  $<$<NOT:$<BOOL:${BENCH_WITH_BOOST_LOG}>>:BENCH_WITH_BOOST_LOG=0>
  $<$<BOOL:${BENCH_WITH_G3LOG}>:BENCH_WITH_G3LOG=1>
  $<$<NOT:$<BOOL:${BENCH_WITH_G3LOG}>>:BENCH_WITH_G3LOG=0>
  $<$<BOOL:${BENCH_WITH_PLOG}>:BENCH_WITH_PLOG=1>
  $<$<NOT:$<BOOL:${BENCH_WITH_PLOG}>>:BENCH_WITH_PLOG=0>)

# logme target name is expected to be `logme`.
if (NOT TARGET logme)
  message(FATAL_ERROR "logme target was not created; check LOGME_SOURCE_DIR or FetchContent setup")
endif()

target_link_libraries(logbench PRIVATE logme spdlog::spdlog)

if (USE_FMT)
  target_link_libraries(logbench PRIVATE fmt::fmt)
endif()

if (BENCH_WITH_QUILL)
  target_link_libraries(logbench PRIVATE quill::quill)
endif()

if (BENCH_WITH_EASYLOGGING)
  target_sources(logbench PRIVATE ${easyloggingpp_SOURCE_DIR}/src/easylogging++.cc)
  target_include_directories(logbench PRIVATE ${easyloggingpp_SOURCE_DIR}/src)
  target_compile_definitions(logbench PRIVATE ELPP_THREAD_SAFE)
  if (MSVC)
    target_compile_definitions(logbench PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
  endif()
endif()

if (BENCH_WITH_BOOST_LOG)
  if (TARGET Boost::log)
    target_link_libraries(logbench PRIVATE Boost::log)
  endif()
  if (TARGET Boost::log_setup)
    target_link_libraries(logbench PRIVATE Boost::log_setup)
  endif()
  if (TARGET Boost::headers)
    target_link_libraries(logbench PRIVATE Boost::headers)
  endif()
endif()

if (BENCH_WITH_G3LOG)
  if (TARGET g3log)
    target_link_libraries(logbench PRIVATE g3log)
  elseif (TARGET g3logger)
    target_link_libraries(logbench PRIVATE g3logger)
  else()
    message(FATAL_ERROR "g3log target was not created by dependency build")
  endif()
endif()

if (BENCH_WITH_PLOG)
  target_link_libraries(logbench PRIVATE plog::plog)
endif()

if (WIN32)
  target_link_libraries(logbench PRIVATE ws2_32)
endif()

if (MSVC)
  target_compile_options(logbench PRIVATE /W4 /Zc:preprocessor)
  target_compile_options(logbench PRIVATE
    $<$<CONFIG:Release>:/Zi>
    $<$<CONFIG:Release>:/Zo>)
  target_link_options(logbench PRIVATE $<$<CONFIG:Release>:/DEBUG:FULL>)
else()
  target_compile_options(logbench PRIVATE -Wall -Wextra -Wpedantic)
endif()
