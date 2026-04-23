#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "bench_util.h"

#include <algorithm>

namespace bench
{
const char* ModeName(BenchMode mode)
{
  switch (mode)
  {
    case BenchMode::Null: return "null";
    case BenchMode::File: return "file";
    case BenchMode::Console: return "console";
    case BenchMode::FileConsole: return "file+console";
  }
  return "unknown";
}

const char* FormatName(FormatType format)
{
  switch (format)
  {
    case FormatType::C: return "c";
    case FormatType::Cpp: return "cpp";
    case FormatType::Fmt: return "fmt";
  }
  return "unknown";
}

const char* MeasureName(MeasureMode mode)
{
  switch (mode)
  {
    case MeasureMode::Throughput: return "throughput";
    case MeasureMode::Latency: return "latency";
  }
  return "unknown";
}

uint64_t Median(std::vector<uint64_t> v)
{
  if (v.empty())
    return 0;

  std::sort(v.begin(), v.end());
  return v[v.size() / 2];
}

std::string JoinPath(const std::string& dir, const std::string& file)
{
  if (dir.empty() || dir == ".")
    return file;

#if defined(_WIN32)
  constexpr char sep = '\\';
#else
  constexpr char sep = '/';
#endif

  if (dir.back() == sep)
    return dir + file;

  return dir + sep + file;
}

bool StartsWith(const std::string& s, const char* pfx)
{
  return s.rfind(pfx, 0) == 0;
}

std::vector<FormatType> SupportedFormats(const DriverCaps& caps)
{
  std::vector<FormatType> formats;

  if (caps.HasC)
    formats.push_back(FormatType::C);

  if (caps.HasCpp)
    formats.push_back(FormatType::Cpp);

  if (caps.HasFmt)
    formats.push_back(FormatType::Fmt);

  return formats;
}
} // namespace bench
