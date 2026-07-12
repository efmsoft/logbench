# logbench

**logbench** is a C++ benchmark for measuring the runtime cost of popular logging libraries under several practical output modes.

The project focuses on a simple question:

> How much does a single logging call actually cost?

Instead of testing only one synthetic case, `logbench` compares libraries in scenarios that matter in real code:

- logging disabled (`null`)
- writing to a file
- writing to the console
- writing to both file and console

The goal is not to crown a universal winner, but to provide a **fair, transparent, and reproducible comparison** that helps evaluate the real overhead of logging in hot paths.

## Included Article

A detailed write-up with methodology, charts, and discussion is available here:

- [docs/article.md](docs/article.md)

## Compared Libraries

The benchmark currently covers:

- [logme](https://github.com/efmsoft/logme)
- [spdlog](https://github.com/gabime/spdlog)
- [quill](https://github.com/odygrd/quill)
- [easylogging++](https://github.com/amrayn/easyloggingpp)
- [Boost.Log](https://www.boost.org/doc/libs/release/libs/log/)
- [g3log](https://github.com/KjellKod/g3log)
- [plog](https://github.com/SergiusTheBest/plog)

`logbench` treats message construction as a global benchmark dimension with three formats:

- `c`
- `cpp`
- `fmt`

Each library participates only in the formats it actually supports. `std::format` versus `fmt` is not a runtime test parameter; it is a build-time project configuration, which naturally produces different result tables for different builds.

`logbench` also supports message payload profiles:

- `integer` — the original small scalar argument case, for example `value is 42`
- `dynamic-string` — every logging call receives a real `std::string` built on the producer thread

The `dynamic-string` payload exists because deferred-formatting async loggers can handle scalar values and string literals very cheaply, but dynamic strings still have to be captured/copied before the producer returns. This mode makes that cost visible instead of measuring only the best case for deferred formatting.

## What logbench Measures

The benchmark supports two measurement modes:

- `throughput` — how many logging calls fit into a fixed time window
- `latency` — total producer-side latency for a fixed number of logging calls

For asynchronous libraries, the latency mode uses a bounded lossless async setup where the library exposes such controls. The comparable latency table includes only drivers that can be configured to the benchmark async profile: bounded queue, lossless delivery, and blocking/backpressure on overflow. Drivers that cannot expose a compatible queue profile are skipped in this mode instead of being silently ranked against incompatible producer paths.

The benchmark uses the same queue budget in the unit exposed by the library: `8192` records for record-count queues and `4194304` bytes for byte/buffer queues. Libraries that expose both limits receive both limits. It also reports drain time separately, so producer-side cost and backend completion cost do not get mixed together.

Result tables are grouped by output scenario. Each scenario (`null`, `file`, `console`, `file+console`) is printed as a separate side-by-side mini-table, so every scenario has its own unambiguous ranking. Throughput rows are sorted from higher to lower cycle count. Latency rows are sorted from lower to higher producer latency, with drain time used only as a tie-breaker.

## Benchmark Scenarios

The current scenarios are:

| Scenario | Description |
|---|---|
| `null` | Logging call is executed, but nothing is written |
| `file` | Messages are written to a file |
| `console` | Messages are written to the console |
| `file + console` | Messages are written to both outputs |

The `null` case is especially useful because it shows the overhead of the logging path itself when output is disabled at runtime rather than compiled out.

## Benchmark Philosophy

The benchmark is intentionally kept simple and comparable across libraries.

To reduce unrelated noise:

- a **minimal output format** is used
- extra fields such as timestamps, thread id, logger name, and level are excluded
- latency async queues are configured through one common bounded/lossless/block profile
- tests are repeated multiple times
- the **median** result is used as the final value

The intention is to avoid library-specific tuning that would make the comparison less useful, while also avoiding unfair comparisons between a bounded queue with backpressure and a much larger or effectively unbounded enqueue path.

## Default Run Parameters

If `logbench` is started without parameters, the following defaults are used:

```text
--mode=throughput
--payload=integer
--seconds=3
--cycles=8192
--repeat=3
--warmup-ms=100
--pause-ms=150
--outdir=.
```

In latency mode the output includes the async queue policy, for example:

```text
async: bounded-lossless/block
included: compatible bounded async drivers only
cycles: 8192
queue: records=8192 where record queues are configurable, bytes=4194304 where byte queues are configurable
```

Parameter summary:

- `--mode` — `throughput` or `latency`
- `--payload` — `integer` or `dynamic-string`
- `--seconds` — duration of one throughput run
- `--cycles` — number of logging calls in latency mode
- `--repeat` — number of repetitions for each test
- `--warmup-ms` — warm-up duration before measurement
- `--pause-ms` — pause between repetitions
- `--outdir` — directory for generated benchmark output files
- `--results` — append result tables directly to a file without redirecting console output

For more stable numbers, a longer run is recommended, for example:

```text
--seconds=15
--pause-ms=1000
```

## Build

Use one explicit Release build command sequence:

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DUSE_FMT=ON
cmake --build build/release --config Release
```

This form is intentionally used instead of CMake presets. It does not force the Ninja generator on Windows, so CMake can use the installed Visual Studio generator when that is the normal compiler environment.

Both Release selectors are kept on purpose:

- `-DCMAKE_BUILD_TYPE=Release` selects Release for single-configuration generators such as Ninja and Unix Makefiles.
- `--config Release` selects Release for multi-configuration generators such as Visual Studio.

On Windows, `No CMAKE_CXX_COMPILER could be found` means CMake cannot see a C++ toolchain in the current environment. Install Visual Studio / Build Tools with the C++ workload, or run the command from a Visual Studio Developer PowerShell / Developer Command Prompt.

Notes:

- `USE_FMT=ON` enables the external `{fmt}` build path used by the benchmark.
- `logme` switches to external `{fmt}` when `USE_FMT=ON` through `LOGME_FMT_FORMAT=ON`.
- `spdlog` switches to external `{fmt}` when `USE_FMT=ON` through `SPDLOG_FMT_EXTERNAL`.
- `quill` switches to external `{fmt}` when `USE_FMT=ON` through `QUILL_FMT_EXTERNAL`.
- Boost is fetched from the Boost Git repository rather than the official release archive because Boost's CMake support is available from the Git layout, while official release archives do not ship the top-level CMake entrypoint needed for `add_subdirectory` / `FetchContent`.

## Run

For single-configuration generators, the executable is normally created here:

```bash
./build/release/logbench --seconds=15
./build/release/logbench --mode=latency
./build/release/logbench --mode=latency --payload=dynamic-string --filter=null,fmt
./build/release/logbench --filter=boost.log,file,cpp
```

For Visual Studio builds on Windows, the executable is normally under the selected configuration directory, for example:

```bat
build\release\Release\logbench.exe --seconds=15
```

## Full benchmark run

The repository includes an automated full run covering:

- `throughput` and `latency`
- `integer` and `dynamic-string` payloads
- direct console output and redirected console output

The automation terminates any `logbench` processes left by an interrupted previous run, configures a Release build when needed, builds `logbench`, creates a new `results.log`, runs all eight benchmark combinations, and removes known benchmark log files before and after every run. Redirected runs suppress stdout without affecting the result tables written through `--results`.

For a complete reproducible check, run the command from the repository root.

Linux and macOS:

```bash
./run.sh
```

Windows:

```bat
run.bat
```

To clean and rebuild the configured project before running the benchmark, use the optional `--rebuild` parameter:

```bash
./run.sh --rebuild
```

```bat
run.bat --rebuild
```

This performs a clean build without deleting or explicitly reconfiguring `build/release`.

The same full run is also available from an already configured build directory:

```bash
cmake --build build/release --config Release --target run
```

The results are written to `results.log`. During a run, both the main console and the results file show the current library, sink mode, format, pass number, and the elapsed time for completed passes. Progress can be watched from another terminal.

Linux and macOS:

```bash
tail -f results.log
```

Windows command prompt or PowerShell:

```bat
powershell -NoProfile -Command "Get-Content .\results.log -Wait"
```

## Why This Repository Exists

Logging is often treated as infrastructure code and evaluated mostly by API convenience or popularity.

In practice, it can become part of the critical execution path:

- in high-frequency code
- in multi-threaded services
- in low-latency systems
- in applications with large amounts of disabled debug logging

`logbench` exists to make that cost visible and easy to reproduce.


## Source layout

The benchmark is now split into small modules instead of one oversized translation unit.

- `src/main.cpp` - command-line entry point and top-level orchestration
- `src/bench_types.h` - shared enums and common result/config structs
- `src/bench_driver.h` - common driver interface
- `src/cli.*` - command-line parsing
- `src/filter.*` - filter token parsing and matching
- `src/bench_runner.*` - common per-case execution logic
- `src/results_printer.*` - result tables
- `src/modes/measure_runner.*` - mode-specific measurement routines
- `src/drivers/*.h` and `src/drivers/*.cpp` - one module per logging library

This keeps `main` focused on orchestration, keeps each library implementation isolated, and makes it much easier to add new benchmark modes or new drivers.
