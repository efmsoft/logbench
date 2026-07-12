#include "bench_runner.h"

#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "bench_util.h"
#include "driver_registry.h"
#include "modes/measure_runner.h"

namespace bench
{
static void ReportProgress(
  std::ostream* progressOutput,
  const std::string& text)
{
  std::cerr << text << std::endl;

  if (progressOutput != nullptr)
  {
    *progressOutput << text << std::endl;
    progressOutput->flush();
  }
}

static std::string MakeProgressText(
  const char* action,
  const Cli& cli,
  const std::string& lib,
  BenchMode mode,
  FormatType format,
  int pass,
  double elapsedSeconds = -1.0)
{
  std::ostringstream output;
  output
    << action
    << ": library=" << lib
    << " mode=" << ModeName(mode)
    << " format=" << FormatName(format)
    << " pass=" << pass << "/" << cli.Repeat;

  if (elapsedSeconds >= 0.0)
  {
    output
      << " elapsed="
      << std::fixed
      << std::setprecision(1)
      << elapsedSeconds
      << "s";
  }

  return output.str();
}

BenchResult RunBenchCase(
  const Cli& cli,
  const std::string& lib,
  BenchMode mode,
  FormatType format,
  std::ostream* progressOutput)
{
  BenchResult result;
  result.Lib = lib;
  result.Mode = ModeName(mode);
  result.Format = FormatName(format);
  result.Measure = MeasureName(cli.Measure);

  std::vector<uint64_t> cyclesRuns;
  std::vector<uint64_t> totalNsRuns;
  std::vector<uint64_t> nsPerCallRuns;
  std::vector<uint64_t> drainRuns;

  cyclesRuns.reserve(static_cast<size_t>(cli.Repeat));
  totalNsRuns.reserve(static_cast<size_t>(cli.Repeat));
  nsPerCallRuns.reserve(static_cast<size_t>(cli.Repeat));
  drainRuns.reserve(static_cast<size_t>(cli.Repeat));

  for (int i = 0; i < cli.Repeat; ++i)
  {
    int pass = i + 1;
    ReportProgress(
      progressOutput,
      MakeProgressText("Testing", cli, lib, mode, format, pass));

    auto passStarted = std::chrono::steady_clock::now();
    auto drivers = CreateDrivers();
    IBenchDriver* driver = nullptr;

    for (auto& d : drivers)
    {
      if (lib == d->GetLibName())
      {
        driver = d.get();
        break;
      }
    }

    if (!driver)
      break;

    try
    {
      auto filePath = JoinPath(cli.OutDir, lib + "_bench.log");
      if (!driver->Setup(mode, filePath, cli.Measure))
      {
        std::cerr << lib << " setup failed for mode " << ModeName(mode) << "\n";
        break;
      }

      auto logOnce = driver->MakeLogOnce(format, cli.Payload);

      if (cli.Measure == MeasureMode::Throughput)
      {
        auto stats = RunThroughput(cli.Seconds, cli.WarmupMs, logOnce);
        cyclesRuns.push_back(stats.Cycles);
        result.Cycles = Median(cyclesRuns);
        driver->TeardownAndDrainNs();
      }
      else
      {
        auto stats = RunLatency(cli.Cycles, cli.WarmupMs, logOnce);
        auto drainNs = driver->TeardownAndDrainNs();
        cyclesRuns.push_back(stats.Cycles);
        totalNsRuns.push_back(stats.TotalNs);
        nsPerCallRuns.push_back(stats.NsPerCall);
        drainRuns.push_back(drainNs);

        result.Cycles = Median(cyclesRuns);
        result.TotalNs = Median(totalNsRuns);
        result.NsPerCall = Median(nsPerCallRuns);
        result.DrainNs = Median(drainRuns);
      }
    }
    catch (const std::exception& ex)
    {
      result.Failed = true;
      result.FailedStage = "run";
      result.ErrorMessage = ex.what();
      std::cerr << "exception in " << lib << " (" << result.Mode << ", " << result.Format
                << "): " << result.ErrorMessage << "\n";
      break;
    }
    catch (...)
    {
      result.Failed = true;
      result.FailedStage = "run";
      result.ErrorMessage = "unknown exception";
      std::cerr << "exception in " << lib << " (" << result.Mode << ", " << result.Format
                << "): " << result.ErrorMessage << "\n";
      break;
    }

    auto passFinished = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(passFinished - passStarted).count();
    ReportProgress(
      progressOutput,
      MakeProgressText("Completed", cli, lib, mode, format, pass, elapsed));

    if (i + 1 < cli.Repeat)
    {
      PauseBetweenRuns(cli.PauseMs);
    }
  }

  return result;
}
} // namespace bench
