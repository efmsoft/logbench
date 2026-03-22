# What Does LOG_INFO() Really Cost? Benchmarking C++ Logging Libraries

Logging exists in almost every C++ project. Almost any service, daemon, or library eventually accumulates calls like LOG_INFO(...) or logger.debug(...).

Most often, a logging library is chosen based on habit or popularity — spdlog, quill, easylogging++, etc. At the same time, few people actually check what cost logging introduces.

In high-load systems, logging may be performed:

- millions of times per second
- from multiple threads
- with string formatting
- with output to a file or console

At this point, the logging library becomes part of the critical execution path.

A practical question arises:

**What is the real cost of a single LOG_INFO() call?**

To get a concrete answer, a small benchmark was created.

Source code:
https://github.com/efmsoft/logbench

---

## What is being tested

The benchmark measures the maximum number of log messages per second.

Scenarios:

- null — logging disabled
- file — writing to file
- console — output to console
- file + console — both

The null scenario is especially important.

It shows the cost of the logging call itself when nothing is written.

---

## Libraries tested

- logme
- spdlog
- quill
- easylogging++


For logme, different formatting APIs were also tested:

- C-style (printf)
- std::format
- iostream

---

## Test configuration

CPU: Intel i9-13900HX  
OS: Windows 11  

Release build was used.

---

## Testing conditions

Minimal formatting:

- no timestamps
- no thread id
- no log level
- no logger name

---

## Parameters

Default:

--seconds=3  
--repeat=5  
--warmup-ms=300  
--pause-ms=250  

Recommended:

--seconds=15  

Median is used.

---

## Results

| Library             | null       | file       | console   | file+console |
|--------------------|-----------:|-----------:|----------:|-------------:|
| logme (c)          | 527M       | 142M       | 615K      | 650K         |
| logme (cpp-stream) | 30M        | 26M        | 637K      | 596K         |
| logme (std::format)| 148M       | 89M        | 640K      | 575K         |
| spdlog             | 245M       | 119M       | 677K      | 621K         |
| quill              | 1.2M       | 1.6M       | 219K      | 231K         |
| easylogging++      | 26M        | 1.6M       | 581K      | 394K         |

---

## Why null benchmark matters

Even disabled logs still do work:

- level checks
- argument handling

---

## Null benchmark

![Null benchmark](images/null.png)

![Null log scale](images/null_log_scale.png)

---

## File benchmark

![File benchmark](images/file.png)

---

## Console benchmark

![Console benchmark](images/console.png)

---

## File + Console

![File + console](images/file_console.png)

---

## Formatting impact

- std::format ~3x slower
- iostream ~4–5x slower

---

## Conclusions

- Logging cost may differ by orders of magnitude
- Even disabled logs have overhead
- Formatting matters
- Architecture matters

logme and spdlog show strongest results.
