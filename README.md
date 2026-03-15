# logbench

**logbench** is a benchmarking tool for measuring the performance of popular C++ logging libraries under different workloads and output modes.

The goal of the project is to provide a **fair and transparent comparison of logging performance**, including scenarios such as writing to files, console output, and disabled logging. The benchmark focuses on throughput, latency, and scalability across multiple threads.

The project is designed to help developers understand the **real runtime cost of logging** and evaluate which logging library best fits their needs.

## Compared Libraries

The benchmark currently includes the following logging libraries:

- **logme**
- **spdlog**
- **quill**
- **easylogging++**

Additional libraries may be added in the future.

## Benchmark Goals

The project aims to:

- Measure **logging throughput** under different workloads
- Evaluate **multithreaded scalability**
- Compare performance across **different sinks** (null, file, console)
- Provide **repeatable and reproducible tests**
- Avoid biased setups or library-specific optimizations

All libraries are configured as similarly as possible to ensure the comparison is meaningful.

## Benchmark Scenarios

Typical benchmark scenarios include:

- Logging disabled (baseline overhead)
- Logging to a file
- Logging to the console
- Single-threaded logging
- Multi-threaded logging
- Short and formatted log messages

The tests measure:

- messages per second
- total execution time
- CPU overhead

## Build

The project uses **CMake**.

```bash
git clone https://github.com/efmsoft/logbench.git
cd logbench
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Command Line Parameters

`logbench` supports several command line parameters that control benchmark execution.

If the program is started **without parameters**, the following defaults are used:

```
--seconds=3
--repeat=5
--warmup-ms=300
--pause-ms=250
--outdir=.
```

### Parameter description

- **--seconds** — duration of a single benchmark run.
- **--repeat** — number of repetitions for each test.
- **--warmup-ms** — warm-up time before the measurement begins.
- **--pause-ms** — pause between benchmark runs.
- **--outdir** — directory where benchmark results are written.

### Recommended settings for more accurate results

For more stable and statistically reliable measurements it is recommended to run the benchmark with longer durations, for example:

```
--seconds=15
--pause-ms=1000
```

Longer benchmark duration reduces noise caused by OS scheduling, CPU frequency changes, and background activity.
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
