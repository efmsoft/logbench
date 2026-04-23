#pragma once

#include <string>
#include <vector>

#include "bench_driver.h"
#include "bench_types.h"

namespace bench
{
BenchResult RunBenchCase(const Cli& cli, const std::string& lib, BenchMode mode, FormatType format);
} // namespace bench
