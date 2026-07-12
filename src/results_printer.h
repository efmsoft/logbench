#pragma once

#include <ostream>
#include <vector>

#include "bench_types.h"

namespace bench
{
void PrintResults(
  std::ostream& output,
  const Cli& cli,
  const std::vector<BenchResult>& results);
} // namespace bench
