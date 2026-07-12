#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include "bench_runner.h"
#include "bench_util.h"
#include "cli.h"
#include "driver_registry.h"
#include "filter.h"
#include "results_printer.h"

int main(int argc, char** argv)
{
  auto cli = bench::ParseCli(argc, argv);

  std::unique_ptr<std::ofstream> resultsFile;
  std::ostream* resultsOutput = nullptr;

  if (!cli.ResultsFile.empty())
  {
    resultsFile = std::make_unique<std::ofstream>(
      cli.ResultsFile,
      std::ios::out | std::ios::app);

    if (!*resultsFile)
    {
      std::cerr
        << "failed to open results file: "
        << cli.ResultsFile
        << "\n";
      return 1;
    }

    resultsOutput = resultsFile.get();
  }

  std::vector<bench::BenchResult> results;
  auto drivers = bench::CreateDrivers();

  const bench::BenchMode modes[] =
  {
    bench::BenchMode::Null,
    bench::BenchMode::File,
    bench::BenchMode::Console,
    bench::BenchMode::FileConsole
  };

  for (const auto& driver : drivers)
  {
    if (!driver)
      continue;

    auto caps = driver->GetCaps();

    if (cli.Measure == bench::MeasureMode::Latency &&
        !bench::IsLatencyAsyncProfileComparable(caps.LatencyProfile))
    {
      std::cerr
        << driver->GetLibName()
        << " skipped in latency mode: incompatible async queue profile ("
        << bench::LatencyAsyncProfileName(caps.LatencyProfile)
        << ")\n";
      continue;
    }

    auto formats = bench::SupportedFormats(caps);

    for (auto mode : modes)
    {
      for (auto format : formats)
      {
        bench::BenchCase benchCase;
        benchCase.Lib = driver->GetLibName();
        benchCase.Mode = mode;
        benchCase.Format = format;
        benchCase.Measure = cli.Measure;

        if (!bench::MatchFilter(cli.Filter, benchCase))
          continue;

        results.push_back(
          bench::RunBenchCase(
            cli,
            driver->GetLibName(),
            mode,
            format,
            resultsOutput));
      }
    }
  }

  if (resultsOutput == nullptr)
  {
    bench::PrintResults(std::cout, cli, results);
    return 0;
  }

  bench::PrintResults(*resultsOutput, cli, results);
  resultsOutput->flush();

  if (!*resultsOutput)
  {
    std::cerr
      << "failed to write results file: "
      << cli.ResultsFile
      << "\n";
    return 1;
  }

  return 0;
}
