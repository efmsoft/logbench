#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "bench_types.h"

namespace bench
{
inline constexpr size_t ASYNC_QUEUE_RECORD_CAPACITY = 8u * 1024u;
inline constexpr size_t ASYNC_QUEUE_BYTE_CAPACITY = 4u * 1024u * 1024u;
inline constexpr size_t ASYNC_QUEUE_CAPACITY = ASYNC_QUEUE_RECORD_CAPACITY;

const char* ModeName(BenchMode mode);
const char* FormatName(FormatType format);
const char* MeasureName(MeasureMode mode);
const char* PayloadName(PayloadType payload);
std::string MakeDynamicString(uint64_t value);
uint64_t Median(std::vector<uint64_t> v);
std::string JoinPath(const std::string& dir, const std::string& file);
bool StartsWith(const std::string& s, const char* pfx);
std::vector<FormatType> SupportedFormats(const DriverCaps& caps);
} // namespace bench
