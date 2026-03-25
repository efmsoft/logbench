# What Does LOG_INFO() Really Cost?: Benchmark of C++ Logging Libraries

Logging exists in almost every C++ project. Almost any service, daemon or library sooner or later accumulates lines like LOG_INFO(...) or logger.debug(...).

Most often, a library is chosen based on habit or popularity — spdlog, quill, easylogging++, etc. At the same time, few people check what price the application pays for logging.

In high-load systems, logging can be performed:

- millions of times per second
- from multiple threads
- with string formatting
- with writing to a file or console

At this moment, the logging library begins to enter **the critical execution path** of the program.

Over time, a quite practical question arises:

> How much does a single LOG_INFO call actually cost?

To get a more concrete answer, a small benchmark was written.

Test sources are open:

https://github.com/efmsoft/logbench

Anyone can build it and reproduce the results.

## What exactly is being tested

The benchmark measures **the maximum number of logging messages per second**.

Four scenarios are tested:

| scenario       | description                     |
|----------------|---------------------------------|
| null           | logging disabled                |
| file           | writing to file                 |
| console        | output to console               |
| file + console | writing to file and console     |

The null scenario is especially interesting.

It shows the **cost of the logging call itself** when the message is not actually output.

That is, the library overhead is measured.

## Which libraries were involved

The following libraries participated in the testing (with links to repositories):

- logme — https://github.com/efmsoft/logme
- spdlog — https://github.com/gabime/spdlog
- quill — https://github.com/odygrd/quill
- easylogging++ — https://github.com/amrayn/easyloggingpp

For logme, different formatting APIs were additionally tested:

- C-style (printf)
- std::format
- iostream

This allows you to separately look at the impact of formatting and understand how strongly the choice of API affects the final performance.

## Test system configuration

Testing was performed on the following system:
- **CPU**: 13th Gen Intel(R) Core(TM) i9-13900HX (2.20 GHz)
- **OS**: Windows 11 Home
- **Version**: 25H2

It is important to note that the results presented below were obtained on a **release build** of the benchmark.

## Testing conditions

To make the comparison as correct as possible, **minimal output format** was used in all libraries.

Additional fields that are often used in real systems were excluded from the format:

- timestamps
- thread id
- log level
- logger name

The goal was to minimize the influence of formatting and additional computations and thereby make the testing conditions between different libraries as close as possible.

## Test parameters

By default, the benchmark is launched as follows:

```
--seconds=3
--repeat=5
--warmup-ms=300
--pause-ms=250
```

For more stable results it is recommended:

```
--seconds=15
```

Each test is executed several times, after which the **median value** is used as the final result.

## Results

| Library             |         null |         file |      console | file+console |
|--------------------|--------------|--------------|--------------|-------------|
| logme (c)          |    527280306 |    142808726 |       615908 |       650303 |
| logme (cpp-stream) |     30875385 |     26293202 |       637631 |       596787 |
| logme (std::format)|    148024581 |     89936987 |       640175 |       575164 |
| spdlog             |    245775694 |    119288244 |       677708 |       621846 |
| quill              |      1225050 |      1620915 |       219747 |       231010 |
| easylogging++      |     26779465 |      1654775 |       581394 |       394377 |

## Why null benchmark is important

At first glance, the null scenario may seem not very useful: if logging is disabled, why measure its cost?

In practice, this scenario often turns out to be critical. In many projects logging is called everywhere, but actual output depends on the logging level. For example,

```
LOG_DEBUG("Request id={}", id);
```

Even if DEBUG level is disabled, the library still has to do some work:

- check the logging level
- prepare arguments
- perform part of logical processing

If this path is implemented inefficiently, **even disabled logs can noticeably affect application performance**.

## Logging overhead

It is important to clarify one point: the null scenario **does not mean disabling logging at macro level** (for example via #define).
In this benchmark the situation is measured when the logging call actually happens, but the message is not written anywhere:

- in logme the channel has no backends
- in spdlog the logger has no sinks

Thus, the internal overhead of the logging library is measured.

![Null benchmark](images/null.png)

This test shows the cost of the logging call itself when output is disabled.
The difference between libraries is very large.
To better understand the scale, it is useful to look at the graph in logarithmic scale.

![Null log scale](images/null_log_scale.png)

Here it becomes clear that the differences reach almost two orders of magnitude.
This means that even disabled logging can have a noticeable cost.

## File benchmark

![File benchmark](images/file.png)

Intuitively, one might expect that when writing to a file the difference between libraries would disappear — since the main cost should be I/O.

However, the results show that the architecture of the library still plays a significant role.

Performance is affected by:

- buffering
- locking
- write strategy
- backend organization

## Console benchmark

![Console benchmark](images/console.png)

When outputting to the console, the differences decrease.

The reason is quite obvious — **the console itself becomes the bottleneck**.

Therefore most libraries show similar results.

## File + Console

![File + console benchmark](images/file_console.png)

The combined scenario shows approximately the same picture.

When the console participates in output, it begins to limit overall performance.

## Impact of formatting

| API          | file msgs/sec |
|--------------|---------------|
| C-style      | 19M           |
| std::format  | 6.7M          |
| iostream     | 4.1M          |

It turns out:

- std::format is approximately 3 times slower
- iostream is approximately 4–5 times slower

## Why results differ

Logging performance depends on several factors.

### Library architecture

Some libraries use:

- mutex
- lock-free queues
- buffering

Each approach has its own trade-offs.

### Logging level check

The best option is to check the level as early as possible.

### String formatting

Formatting is often one of the most expensive operations.

### Output buffering

If each message is written immediately, it is significantly slower.

## Why your results may differ

The benchmark is sensitive to:

- CPU
- file system
- terminal
- compiler
- optimization settings

Therefore absolute values may differ.

But the relative picture usually remains similar.

## How to reproduce

```
git clone https://github.com/efmsoft/logbench
```

```
cmake -B build
cmake --build build --config Release
```

```
logbench --seconds=15
```

## Summary

Several observations:

1. Logging cost may differ by orders of magnitude.
2. Even disabled logs may have noticeable overhead.
3. Formatting affects performance more than often expected.
4. Library architecture remains important even when writing to file.

Overall, the strongest results are shown by **logme** and **spdlog**.

- spdlog shows very stable results in all scenarios. 
- logme demonstrates minimal logging call overhead and in some tests significantly outperforms other libraries. 

The difference is especially noticeable in the null scenario.

It is worth noting separately that logme supports multiple formatting APIs (C-style, std::format, iostream). This allows choosing a balance between convenience and performance depending on the task.

## Conclusion

Logging is rarely considered part of the critical execution path.

However, in high-load systems it can significantly affect performance.

Therefore, it sometimes makes sense to run a simple benchmark and see how much logging actually costs in your application.
