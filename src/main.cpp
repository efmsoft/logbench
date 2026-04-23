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

        results.push_back(bench::RunBenchCase(cli, driver->GetLibName(), mode, format));
      }
    }
  }

  bench::PrintResults(cli, results);
  return 0;
}
