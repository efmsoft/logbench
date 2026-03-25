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

For **logme**, multiple formatting APIs are also benchmarked separately:

- C-style (`printf`-like)
- `std::format`
- iostream

This makes it possible to look not only at library architecture, but also at the cost of formatting style.

## What logbench Measures

The benchmark reports the **maximum number of log messages per second** for each scenario.

The main things it is intended to highlight are:

- logging call overhead when output is disabled
- throughput when writing to real sinks
- the impact of formatting API choice
- scalability across different workloads

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
--seconds=3
--repeat=5
--warmup-ms=300
--pause-ms=250
--outdir=.
```

Parameter summary:

- `--seconds` — duration of one benchmark run
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

The project uses CMake.

```bash
git clone https://github.com/efmsoft/logbench.git
cd logbench
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Run

Example:

```bash
./build/logbench --seconds=15
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
