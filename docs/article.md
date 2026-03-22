# What Does LOG_INFO() Really Cost?
## Benchmarking C++ Logging Libraries

Logging is present in almost every C++ project.

## Goal

Compare:
- spdlog
- quill
- easylogging++
- logme

## Results

![Result 1](images/1.png)

![Result 2](images/2.png)

![Result 3](images/3.png)

## Notes

This benchmark isolates logging overhead and does not represent real-world configs.
