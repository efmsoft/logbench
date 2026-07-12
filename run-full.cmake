if (NOT DEFINED LOGBENCH OR LOGBENCH STREQUAL "")
  message(FATAL_ERROR "LOGBENCH is not specified")
endif()

if (NOT EXISTS "${LOGBENCH}")
  message(FATAL_ERROR "logbench executable not found: ${LOGBENCH}")
endif()

if (NOT DEFINED RESULTS_FILE OR RESULTS_FILE STREQUAL "")
  set(RESULTS_FILE "${CMAKE_CURRENT_LIST_DIR}/results.log")
endif()

set(BENCHMARK_LOGS
  easylogging_bench.log
  logme_bench.log
  myeasylog.log
  quill_bench.log
  spdlog_bench.log
  boost.log_bench.log
  easylogging++_bench.log
  g3log_bench.log
  plog_bench.log)

function(CleanupLogs)
  foreach(log_file IN LISTS BENCHMARK_LOGS)
    file(REMOVE "${CMAKE_CURRENT_LIST_DIR}/${log_file}")
  endforeach()
endfunction()

function(RunCase measure payload console_output)
  set(status_text
    "Running: measure=${measure} payload=${payload} console-output=${console_output}")

  file(APPEND "${RESULTS_FILE}"
    "========================================================================\n"
    "${status_text}\n")

  message(STATUS "${status_text}")

  CleanupLogs()

  set(arguments
    "--mode=${measure}"
    "--payload=${payload}"
    "--results=${RESULTS_FILE}")

  if (console_output STREQUAL "redirected")
    execute_process(
      COMMAND "${LOGBENCH}" ${arguments}
      WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
      OUTPUT_QUIET
      RESULT_VARIABLE status)
  else()
    execute_process(
      COMMAND "${LOGBENCH}" ${arguments}
      WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
      RESULT_VARIABLE status)
  endif()

  CleanupLogs()

  if (NOT status EQUAL 0)
    message(FATAL_ERROR
      "Run failed: measure=${measure} payload=${payload} console-output=${console_output}")
  endif()
endfunction()

file(WRITE "${RESULTS_FILE}" "")

if (WIN32)
  message(STATUS
    "Watch progress in another console: powershell -NoProfile -Command \"Get-Content .\\results.log -Wait\"")
else()
  message(STATUS
    "Watch progress in another console: tail -f results.log")
endif()

foreach(measure throughput latency)
  foreach(payload integer dynamic-string)
    RunCase("${measure}" "${payload}" direct)
    RunCase("${measure}" "${payload}" redirected)
  endforeach()
endforeach()

message(STATUS "Results written to ${RESULTS_FILE}")
