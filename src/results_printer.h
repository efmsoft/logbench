#pragma once

#include <vector>

#include "bench_types.h"

namespace bench
{
void PrintResults(const Cli& cli, const std::vector<BenchResult>& results);
} // namespace bench
