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

## What logbench Measures

The benchmark supports two measurement modes:

- `throughput` — how many logging calls fit into a fixed time window
- `latency` — total producer-side latency for a fixed number of logging calls

For asynchronous libraries, the latency mode also reports drain time separately, so enqueue cost and backend completion cost do not get mixed together.

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
- tests are repeated multiple times
- the **median** result is used as the final value

The intention is to avoid library-specific tuning that would make the comparison less useful.

## Default Run Parameters

If `logbench` is started without parameters, the following defaults are used:

```text
--mode=throughput
--seconds=3
--cycles=200000
--repeat=5
--warmup-ms=300
--pause-ms=250
--outdir=.
```

Parameter summary:

- `--mode` — `throughput` or `latency`
- `--seconds` — duration of one throughput run
- `--cycles` — number of logging calls in latency mode
- `--repeat` — number of repetitions for each test
- `--warmup-ms` — warm-up duration before measurement
- `--pause-ms` — pause between repetitions
- `--outdir` — directory for generated benchmark results

For more stable numbers, a longer run is recommended, for example:

```text
--seconds=15
--pause-ms=1000
```

## Build

The project uses CMake and provides two explicit release configurations:

- `release-std` - `fmt` test cases are built through the standard library formatting path
- `release-fmt` - external `{fmt}` is enabled and libraries that support it are built against it

The easiest way to configure and build is through presets:

```bash
cmake --preset release-std
cmake --build --preset build-release-std

cmake --preset release-fmt
cmake --build --preset build-release-fmt
```

Equivalent manual configuration:

```bash
cmake -B build/std-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DUSE_FMT=OFF
cmake --build build/std-release

cmake -B build/fmt-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DUSE_FMT=ON
cmake --build build/fmt-release
```

Notes:

- `spdlog` switches to external `{fmt}` when `USE_FMT=ON` through `SPDLOG_FMT_EXTERNAL`.
- `quill` switches to external `{fmt}` when `USE_FMT=ON` through `QUILL_FMT_EXTERNAL`.
- Boost is fetched from the Boost Git repository rather than the official release archive because Boost's CMake support is available from the Git layout, while official release archives do not ship the top-level CMake entrypoint needed for `add_subdirectory` / `FetchContent`.

## Run

Example:

```bash
./build/std-release/logbench --seconds=15
./build/std-release/logbench --mode=latency --cycles=500000
./build/fmt-release/logbench --filter=boost.log,file,cpp
```

On Windows, depending on the generator, you may run the produced executable from the build directory instead.

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
