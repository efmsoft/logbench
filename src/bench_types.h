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
  int Seconds = 3;
  int Cycles = 200000;
  int Repeat = 5;
  int WarmupMs = 300;
  int PauseMs = 250;
  std::string OutDir = ".";
  std::string Filter;
  MeasureMode Measure = MeasureMode::Throughput;
};

struct DriverCaps
{
  bool HasC = false;
  bool HasCpp = false;
  bool HasFmt = false;
};

struct RunStats
{
  uint64_t Cycles = 0;
  uint64_t TotalNs = 0;
  uint64_t NsPerCall = 0;
};
} // namespace bench
