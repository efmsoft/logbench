#pragma once

#include <cstdint>

#if defined(_WIN32)
#include <Windows.h>
#else
#include <time.h>
#endif

namespace bench
{
inline uint64_t NowMs()
{
#if defined(_WIN32)
  return static_cast<uint64_t>(GetTickCount64());
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000ULL +
         static_cast<uint64_t>(ts.tv_nsec) / 1000000ULL;
#endif
}

inline uint64_t ElapsedMs(uint64_t start)
{
  return NowMs() - start;
}
}  // namespace bench
