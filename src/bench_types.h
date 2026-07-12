#pragma once

#include <cstdint>
#include <string>

namespace bench
{
enum class BenchMode
{
  Null,
  File,
  Console,
  FileConsole
};

enum class FormatType
{
  C,
  Cpp,
  Fmt
};

enum class MeasureMode
{
  Throughput,
  Latency
};

enum class PayloadType
{
  Integer,
  DynamicString
};

enum class LatencyAsyncProfile
{
  Unsupported,
  BoundedLosslessRecords,
  BoundedLosslessBytes,
  BoundedLosslessRecordsAndBytes
};

struct BenchCase
{
  std::string Lib;
  BenchMode Mode;
  FormatType Format;
  MeasureMode Measure;
};

struct BenchResult
{
  std::string Lib;
  std::string Mode;
  std::string Format;
  std::string Measure;
  bool Failed = false;
  std::string FailedStage;
  std::string ErrorMessage;
  uint64_t Cycles = 0;
  uint64_t TotalNs = 0;
  uint64_t NsPerCall = 0;
  uint64_t DrainNs = 0;
};

struct Cli
{
  int Seconds = 10;
  int Cycles = 50000;
  int Repeat = 3;
  int WarmupMs = 100;
  int PauseMs = 150;
  std::string OutDir = ".";
  std::string ResultsFile;
  std::string Filter;
  MeasureMode Measure = MeasureMode::Throughput;
  PayloadType Payload = PayloadType::Integer;
};

struct DriverCaps
{
  bool HasC = false;
  bool HasCpp = false;
  bool HasFmt = false;
  LatencyAsyncProfile LatencyProfile = LatencyAsyncProfile::Unsupported;
};

struct RunStats
{
  uint64_t Cycles = 0;
  uint64_t TotalNs = 0;
  uint64_t NsPerCall = 0;
};
} // namespace bench
