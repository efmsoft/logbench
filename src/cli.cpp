#include "cli.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

#include "bench_util.h"

namespace bench
{
Cli ParseCli(int argc, char** argv)
{
  Cli cli;

  for (int i = 1; i < argc; ++i)
  {
    std::string a = argv[i];

    if (StartsWith(a, "--filter="))
    {
      cli.Filter = a.substr(9);
    }
    else if (StartsWith(a, "--seconds="))
    {
      cli.Seconds = (std::max)(1, std::stoi(a.substr(10)));
    }
    else if (StartsWith(a, "--cycles="))
    {
      cli.Cycles = (std::max)(1, std::stoi(a.substr(9)));
    }
    else if (StartsWith(a, "--repeat="))
    {
      cli.Repeat = (std::max)(1, std::stoi(a.substr(9)));
    }
    else if (StartsWith(a, "--warmup-ms="))
    {
      cli.WarmupMs = (std::max)(0, std::stoi(a.substr(12)));
    }
    else if (StartsWith(a, "--outdir="))
    {
      cli.OutDir = a.substr(9);
      if (cli.OutDir.empty())
        cli.OutDir = ".";
    }
    else if (StartsWith(a, "--pause-ms="))
    {
      cli.PauseMs = (std::max)(0, std::stoi(a.substr(11)));
    }
    else if (StartsWith(a, "--mode="))
    {
      std::string mode = a.substr(7);
      if (mode == "latency")
        cli.Measure = MeasureMode::Latency;
      else
        cli.Measure = MeasureMode::Throughput;
    }
    else if (a == "--help" || a == "-h")
    {
      std::cout
        << "Usage: logbench [--mode=throughput|latency] [--seconds=N] [--cycles=N] [--repeat=N] [--warmup-ms=N] [--pause-ms=N] [--outdir=PATH] [--filter=ITEMS]\n"
        << "Default: --mode=throughput --seconds=3 --cycles=200000 --repeat=5 --warmup-ms=300 --pause-ms=250 --outdir=.\n"
        << "Filter tokens (comma-separated): library, sink mode, format, measurement mode.\n"
        << "Formats: c, cpp, fmt\n"
        << "Examples:\n"
        << "  --filter=logme\n"
        << "  --filter=file\n"
        << "  --filter=fmt\n"
        << "  --filter=latency,fmt\n"
        << "  --filter=boost.log,file,cpp\n";
      std::exit(0);
    }
  }

  return cli;
}
} // namespace bench
