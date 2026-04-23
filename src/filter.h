#pragma once

#include <string>
#include <vector>

#include "bench_types.h"

namespace bench
{
std::vector<std::string> SplitFilter(const std::string& filter);
bool MatchFilter(const std::string& filter, const BenchCase& benchCase);
} // namespace bench
