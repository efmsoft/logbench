#include "measure_runner.h"

#include <chrono>
#include <thread>

#include "../fast_timer.h"

namespace bench
{
using Clock = std::chrono::steady_clock;

static const uint64_t TIME_CHECK_BATCH = 1024;

void PauseBetweenRuns(int pauseMs)
{
  if (pauseMs > 0)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(pauseMs));
  }
}

RunStats RunThroughput(int seconds, int warmupMs, const std::function<void(void)>& logOnce)
{
  uint64_t warmStart = NowMs();
  while (ElapsedMs(warmStart) < static_cast<uint64_t>(warmupMs))
  {
    for (uint64_t i = 0; i < TIME_CHECK_BATCH; ++i)
    {
      logOnce();
    }
  }

  RunStats stats;
  uint64_t start = NowMs();

  while (ElapsedMs(start) < static_cast<uint64_t>(seconds) * 1000ULL)
  {
    for (uint64_t i = 0; i < TIME_CHECK_BATCH; ++i)
    {
      logOnce();
      ++stats.Cycles;
    }
  }

  return stats;
}

RunStats RunLatency(int cycles, int warmupMs, const std::function<void(void)>& logOnce)
{
  (void) warmupMs;

  auto start = Clock::now();
  for (int i = 0; i < cycles; ++i)
  {
    logOnce();
  }
  auto end = Clock::now();

  RunStats stats;
  stats.Cycles = static_cast<uint64_t>(cycles);
  stats.TotalNs = static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
  stats.NsPerCall = (cycles > 0) ? (stats.TotalNs / static_cast<uint64_t>(cycles)) : 0;
  return stats;
}
} // namespace bench