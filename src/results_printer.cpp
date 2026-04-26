#include "results_printer.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "bench_util.h"

namespace bench
{
static std::string RowKey(const BenchResult& result)
{
  return result.Lib + " (" + result.Format + ")";
}

static int ModeIndex(const std::string& mode)
{
  if (mode == "null") return 0;
  if (mode == "file") return 1;
  if (mode == "console") return 2;
  if (mode == "file+console") return 3;
  return -1;
}

static void PrintThroughputTable(
  const std::vector<BenchResult>& results,
  const std::string& title,
  uint64_t BenchResult::* metric)
{
  struct Row
  {
    std::string Name;
    uint64_t Cells[4]{};
    bool Has[4]{};
    bool Failed[4]{};
  };

  std::vector<Row> rows;

  auto getRow = [&](const std::string& name) -> Row*
  {
    for (auto& row : rows)
    {
      if (row.Name == name)
        return &row;
    }

    rows.push_back(Row{});
    rows.back().Name = name;
    return &rows.back();
  };

  for (const auto& result : results)
  {
    int mi = ModeIndex(result.Mode);
    if (mi < 0)
      continue;

    auto* row = getRow(RowKey(result));
    row->Cells[mi] = result.*metric;
    row->Has[mi] = true;
    row->Failed[mi] = result.Failed;
  }

  size_t maxName = 7;
  for (const auto& row : rows)
  {
    maxName = (std::max)(maxName, row.Name.size());
  }

  std::cout << title << "\n";
  std::cout
    << std::left << std::setw(static_cast<int>(maxName)) << "Library"
    << " | " << std::right << std::setw(12) << "null"
    << " | " << std::right << std::setw(12) << "file"
    << " | " << std::right << std::setw(12) << "console"
    << " | " << std::right << std::setw(12) << "file+console"
    << "\n";

  std::cout << std::string(maxName, '-')
            << "-+-" << std::string(12, '-')
            << "-+-" << std::string(12, '-')
            << "-+-" << std::string(12, '-')
            << "-+-" << std::string(12, '-')
            << "\n";

  for (const auto& row : rows)
  {
    std::cout << std::left << std::setw(static_cast<int>(maxName)) << row.Name << " | ";

    for (int i = 0; i < 4; ++i)
    {
      std::ostringstream oss;
      if (!row.Has[i])
        oss << "-";
      else if (row.Failed[i])
        oss << "EXC";
      else
        oss << row.Cells[i];

      std::cout << std::right << std::setw(12) << oss.str();
      if (i != 3)
        std::cout << " | ";
    }

    std::cout << "\n";
  }

  std::cout << "\n";
}

static void PrintLatencyProducerTable(const Cli& cli, const std::vector<BenchResult>& results)
{
  struct Row
  {
    std::string Name;
    std::string Cells[4];
    bool Has[4]{};
  };

  std::vector<Row> rows;

  auto getRow = [&](const std::string& name) -> Row*
  {
    for (auto& row : rows)
    {
      if (row.Name == name)
        return &row;
    }

    rows.push_back(Row{});
    rows.back().Name = name;
    return &rows.back();
  };

  for (const auto& result : results)
  {
    int mi = ModeIndex(result.Mode);
    if (mi < 0)
      continue;

    auto* row = getRow(RowKey(result));
    row->Has[mi] = true;

    std::ostringstream oss;
    if (result.Failed)
      oss << "EXC";
    else
      oss << result.NsPerCall << " (" << result.TotalNs << ")";

    row->Cells[mi] = oss.str();
  }

  size_t maxName = 7;
  size_t cellWidth = 12;
  for (const auto& row : rows)
  {
    maxName = (std::max)(maxName, row.Name.size());
    for (int i = 0; i < 4; ++i)
    {
      if (row.Has[i])
        cellWidth = (std::max)(cellWidth, row.Cells[i].size());
    }
  }

  std::cout << "Median producer latency, ns/call (total for N="
            << cli.Cycles << " calls in parentheses)\n";
  std::cout
    << std::left << std::setw(static_cast<int>(maxName)) << "Library"
    << " | " << std::right << std::setw(static_cast<int>(cellWidth)) << "null"
    << " | " << std::right << std::setw(static_cast<int>(cellWidth)) << "file"
    << " | " << std::right << std::setw(static_cast<int>(cellWidth)) << "console"
    << " | " << std::right << std::setw(static_cast<int>(cellWidth)) << "file+console"
    << "\n";

  std::cout << std::string(maxName, '-')
            << "-+-" << std::string(cellWidth, '-')
            << "-+-" << std::string(cellWidth, '-')
            << "-+-" << std::string(cellWidth, '-')
            << "-+-" << std::string(cellWidth, '-')
            << "\n";

  for (const auto& row : rows)
  {
    std::cout << std::left << std::setw(static_cast<int>(maxName)) << row.Name << " | ";

    for (int i = 0; i < 4; ++i)
    {
      std::string cell = row.Has[i] ? row.Cells[i] : "-";
      std::cout << std::right << std::setw(static_cast<int>(cellWidth)) << cell;
      if (i != 3)
        std::cout << " | ";
    }

    std::cout << "\n";
  }

  std::cout << "\n";
}

static void PrintLatencyDrainTable(const std::vector<BenchResult>& results)
{
  struct Row
  {
    std::string Name;
    uint64_t Cells[4]{};
    bool Has[4]{};
    bool Failed[4]{};
  };

  std::vector<Row> rows;

  auto getRow = [&](const std::string& name) -> Row*
  {
    for (auto& row : rows)
    {
      if (row.Name == name)
        return &row;
    }

    rows.push_back(Row{});
    rows.back().Name = name;
    return &rows.back();
  };

  for (const auto& result : results)
  {
    int mi = ModeIndex(result.Mode);
    if (mi < 0)
      continue;

    auto* row = getRow(RowKey(result));
    row->Cells[mi] = result.DrainNs;
    row->Has[mi] = true;
    row->Failed[mi] = result.Failed;
  }

  size_t maxName = 7;
  for (const auto& row : rows)
  {
    maxName = (std::max)(maxName, row.Name.size());
  }

  bool hasDash = false;

  std::cout << "Median drain time after producer stop, ns\n";
  std::cout
    << std::left << std::setw(static_cast<int>(maxName)) << "Library"
    << " | " << std::right << std::setw(12) << "null"
    << " | " << std::right << std::setw(12) << "file"
    << " | " << std::right << std::setw(12) << "console"
    << " | " << std::right << std::setw(12) << "file+console"
    << "\n";

  std::cout << std::string(maxName, '-')
            << "-+-" << std::string(12, '-')
            << "-+-" << std::string(12, '-')
            << "-+-" << std::string(12, '-')
            << "-+-" << std::string(12, '-')
            << "\n";

  for (const auto& row : rows)
  {
    std::cout << std::left << std::setw(static_cast<int>(maxName)) << row.Name << " | ";

    for (int i = 0; i < 4; ++i)
    {
      std::ostringstream oss;
      if (!row.Has[i])
      {
        oss << "-";
      }
      else if (row.Failed[i])
      {
        oss << "EXC";
      }
      else if (row.Cells[i] == 0)
      {
        oss << "-";
        hasDash = true;
      }
      else
      {
        oss << row.Cells[i];
      }

      std::cout << std::right << std::setw(12) << oss.str();
      if (i != 3)
        std::cout << " | ";
    }

    std::cout << "\n";
  }

  if (hasDash)
  {
    std::cout << "\n";
    std::cout << "Note: '-' indicates that this driver does not yet provide a separate measured drain/teardown time.\n";
  }

  std::cout << "\n";
}

void PrintResults(const Cli& cli, const std::vector<BenchResult>& results)
{
#if defined(USE_FMT)
  const char* buildName = "fmt-build";
#else
  const char* buildName = "std-build";
#endif

  std::cout << "build: " << buildName << "\n";
  std::cout << "measure: " << MeasureName(cli.Measure) << "\n\n";

  if (cli.Measure == MeasureMode::Throughput)
  {
    PrintThroughputTable(results, "Median cycles per run", &BenchResult::Cycles);
    return;
  }

  PrintLatencyProducerTable(cli, results);
  PrintLatencyDrainTable(results);
}
} // namespace bench
