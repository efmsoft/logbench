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
