#pragma once

#include <functional>

#include "../bench_types.h"

namespace bench
{
void PauseBetweenRuns(int pauseMs);
RunStats RunThroughput(int seconds, int warmupMs, const std::function<void(void)>& logOnce);
RunStats RunLatency(int cycles, int warmupMs, const std::function<void(void)>& logOnce);
} // namespace bench
