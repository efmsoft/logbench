#include "bench_runner.h"

#include <exception>
#include <iostream>
#include <vector>

#include "bench_util.h"
#include "driver_registry.h"
#include "modes/measure_runner.h"

namespace bench
{
BenchResult RunBenchCase(const Cli& cli, const std::string& lib, BenchMode mode, FormatType format)
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

      auto logOnce = driver->MakeLogOnce(format);

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
        totalNsRuns.push_back(stats.TotalNs);
        nsPerCallRuns.push_back(stats.NsPerCall);
        drainRuns.push_back(drainNs);

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

    if (i + 1 < cli.Repeat)
    {
      PauseBetweenRuns(cli.PauseMs);
    }
  }

  return result;
}
} // namespace bench
