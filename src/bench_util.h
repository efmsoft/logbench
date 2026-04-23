#pragma once

#include <string>
#include <vector>

#include "bench_types.h"

namespace bench
{
const char* ModeName(BenchMode mode);
const char* FormatName(FormatType format);
const char* MeasureName(MeasureMode mode);
uint64_t Median(std::vector<uint64_t> v);
std::string JoinPath(const std::string& dir, const std::string& file);
bool StartsWith(const std::string& s, const char* pfx);
std::vector<FormatType> SupportedFormats(const DriverCaps& caps);
} // namespace bench
