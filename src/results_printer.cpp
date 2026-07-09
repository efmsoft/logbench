#include "results_printer.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "bench_util.h"

namespace bench
{
static constexpr int MODE_COUNT = 4;

static std::string RowKey(const BenchResult& result)
{
  return result.Lib + " (" + result.Format + ")";
}

static int ModeIndex(const std::string& mode)
{
  if (mode == "null")
  {
    return 0;
  }

  if (mode == "file")
  {
    return 1;
  }

  if (mode == "console")
  {
    return 2;
  }

  if (mode == "file+console")
  {
    return 3;
  }

  return -1;
}

static const char* ModeTitle(int modeIndex)
{
  switch (modeIndex)
  {
    case 0:
      return "Null";

    case 1:
      return "File";

    case 2:
      return "Console";

    case 3:
      return "File+Console";

    default:
      return "Unknown";
  }
}

static std::vector<int> GetMeasuredModes(const std::vector<BenchResult>& results)
{
  bool hasMode[MODE_COUNT]{};

  for (const auto& result : results)
  {
    int modeIndex = ModeIndex(result.Mode);
    if (modeIndex >= 0)
    {
      hasMode[modeIndex] = true;
    }
  }

  std::vector<int> modes;
  for (int i = 0; i < MODE_COUNT; ++i)
  {
    if (hasMode[i])
    {
      modes.push_back(i);
    }
  }

  return modes;
}

static std::string FormatScaledNs(uint64_t ns, double scale)
{
  double value = static_cast<double>(ns) / scale;

  if (ns != 0 && value < 0.01)
  {
    return "<0.01";
  }

  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << value;
  return oss.str();
}

struct TextBlock
{
  std::string Title;
  std::vector<std::string> Headers;
  std::vector<std::vector<std::string>> Rows;
  std::vector<size_t> Widths;
  size_t Width = 0;
};

static void CalculateBlockWidths(TextBlock& block)
{
  block.Widths.assign(block.Headers.size(), 0);

  for (size_t i = 0; i < block.Headers.size(); ++i)
  {
    block.Widths[i] = block.Headers[i].size();
  }

  if (!block.Widths.empty())
  {
    block.Widths[0] = (std::max)(block.Widths[0], static_cast<size_t>(7));
  }

  for (const auto& row : block.Rows)
  {
    for (size_t i = 0; i < row.size() && i < block.Widths.size(); ++i)
    {
      block.Widths[i] = (std::max)(block.Widths[i], row[i].size());
    }
  }

  block.Width = 0;
  for (size_t width : block.Widths)
  {
    block.Width += width;
  }

  if (block.Widths.size() > 1)
  {
    block.Width += (block.Widths.size() - 1) * 3;
  }
}

static std::string FormatBlockRow(
  const std::vector<std::string>& cells,
  const std::vector<size_t>& widths)
{
  std::ostringstream oss;

  for (size_t i = 0; i < widths.size(); ++i)
  {
    std::string cell;
    if (i < cells.size())
    {
      cell = cells[i];
    }

    if (i != 0)
    {
      oss << " | ";
    }

    if (i == 0)
    {
      oss << std::left << std::setw(static_cast<int>(widths[i])) << cell;
    }
    else
    {
      oss << std::right << std::setw(static_cast<int>(widths[i])) << cell;
    }
  }

  return oss.str();
}

static std::string FormatBlockSeparator(const std::vector<size_t>& widths)
{
  std::ostringstream oss;

  for (size_t i = 0; i < widths.size(); ++i)
  {
    if (i != 0)
    {
      oss << "-+-";
    }

    oss << std::string(widths[i], '-');
  }

  return oss.str();
}

static void PrintSideBySideBlocks(const std::vector<TextBlock>& blocks)
{
  if (blocks.empty())
  {
    return;
  }

  size_t maxRows = 0;
  for (const auto& block : blocks)
  {
    maxRows = (std::max)(maxRows, block.Rows.size());
  }

  for (size_t i = 0; i < blocks.size(); ++i)
  {
    if (i != 0)
    {
      std::cout << "    |    ";
    }

    std::cout
      << std::left
      << std::setw(static_cast<int>(blocks[i].Width))
      << blocks[i].Title;
  }

  std::cout << "\n";

  for (size_t i = 0; i < blocks.size(); ++i)
  {
    if (i != 0)
    {
      std::cout << "    |    ";
    }

    std::cout << FormatBlockRow(blocks[i].Headers, blocks[i].Widths);
  }

  std::cout << "\n";

  for (size_t i = 0; i < blocks.size(); ++i)
  {
    if (i != 0)
    {
      std::cout << "    |    ";
    }

    std::cout << FormatBlockSeparator(blocks[i].Widths);
  }

  std::cout << "\n";

  for (size_t rowIndex = 0; rowIndex < maxRows; ++rowIndex)
  {
    for (size_t blockIndex = 0; blockIndex < blocks.size(); ++blockIndex)
    {
      const auto& block = blocks[blockIndex];

      if (blockIndex != 0)
      {
        std::cout << "    |    ";
      }

      if (rowIndex < block.Rows.size())
      {
        std::cout << FormatBlockRow(block.Rows[rowIndex], block.Widths);
      }
      else
      {
        std::cout << std::string(block.Width, ' ');
      }
    }

    std::cout << "\n";
  }

  std::cout << "\n";
}

static void PrintThroughputTable(
  const std::vector<BenchResult>& results,
  const std::string& title,
  uint64_t BenchResult::* metric)
{
  struct Row
  {
    std::string Name;
    std::string Value;
    uint64_t SortValue = 0;
    bool SortValid = false;
    size_t Order = 0;
  };

  std::vector<TextBlock> blocks;
  auto modes = GetMeasuredModes(results);

  for (int modeIndex : modes)
  {
    std::vector<Row> rows;
    size_t order = 0;

    for (const auto& result : results)
    {
      if (ModeIndex(result.Mode) != modeIndex)
      {
        continue;
      }

      Row row;
      row.Name = RowKey(result);
      row.Order = order++;

      if (result.Failed)
      {
        row.Value = "EXC";
      }
      else
      {
        row.SortValue = result.*metric;
        row.SortValid = row.SortValue != 0;
        row.Value = std::to_string(row.SortValue);
      }

      rows.push_back(row);
    }

    std::stable_sort(
      rows.begin(),
      rows.end(),
      [](const Row& left, const Row& right)
      {
        if (left.SortValid != right.SortValid)
        {
          return left.SortValid;
        }

        if (!left.SortValid)
        {
          return false;
        }

        if (left.SortValue != right.SortValue)
        {
          return left.SortValue > right.SortValue;
        }

        return false;
      });

    TextBlock block;
    block.Title = ModeTitle(modeIndex);
    block.Headers = {"Library", "Cycles"};

    for (const auto& row : rows)
    {
      block.Rows.push_back({row.Name, row.Value});
    }

    CalculateBlockWidths(block);
    blocks.push_back(block);
  }

  std::cout << title << "\n";
  PrintSideBySideBlocks(blocks);
}

static void PrintLatencyTables(const std::vector<BenchResult>& results)
{
  struct Row
  {
    std::string Name;
    std::string Latency;
    std::string Drain;
    uint64_t LatencyNs = 0;
    uint64_t DrainNs = 0;
    bool HasLatency = false;
    bool Failed = false;
    size_t Order = 0;
  };

  bool hasMissingDrain = false;
  std::vector<TextBlock> blocks;
  auto modes = GetMeasuredModes(results);

  for (int modeIndex : modes)
  {
    std::vector<Row> rows;
    size_t order = 0;

    for (const auto& result : results)
    {
      if (ModeIndex(result.Mode) != modeIndex)
      {
        continue;
      }

      Row row;
      row.Name = RowKey(result);
      row.Order = order++;
      row.Failed = result.Failed;

      if (result.Failed)
      {
        row.Latency = "EXC";
        row.Drain = "EXC";
      }
      else
      {
        row.LatencyNs = result.NsPerCall;
        row.HasLatency = true;
        row.Latency = FormatScaledNs(result.NsPerCall, 1000.0);

        if (result.DrainNs == 0)
        {
          row.Drain = "-";
          hasMissingDrain = true;
        }
        else
        {
          row.DrainNs = result.DrainNs;
          row.Drain = FormatScaledNs(result.DrainNs, 1000.0);
        }
      }

      rows.push_back(row);
    }

    std::stable_sort(
      rows.begin(),
      rows.end(),
      [](const Row& left, const Row& right)
      {
        if (left.HasLatency != right.HasLatency)
        {
          return left.HasLatency;
        }

        if (!left.HasLatency)
        {
          return false;
        }

        if (left.LatencyNs != right.LatencyNs)
        {
          return left.LatencyNs < right.LatencyNs;
        }

        bool leftHasDrain = left.DrainNs != 0;
        bool rightHasDrain = right.DrainNs != 0;

        if (leftHasDrain != rightHasDrain)
        {
          return leftHasDrain;
        }

        if (leftHasDrain && left.DrainNs != right.DrainNs)
        {
          return left.DrainNs < right.DrainNs;
        }

        return false;
      });

    TextBlock block;
    block.Title = ModeTitle(modeIndex);
    block.Headers = {"Library", "Latency", "Drain"};

    for (const auto& row : rows)
    {
      block.Rows.push_back({row.Name, row.Latency, row.Drain});
    }

    CalculateBlockWidths(block);
    blocks.push_back(block);
  }

  std::cout << "Median latency (producer us/call, drain us)\n";
  PrintSideBySideBlocks(blocks);

  if (hasMissingDrain)
  {
    std::cout << "Note: '-' indicates that this driver does not yet provide a separate measured drain/teardown time.\n\n";
  }
}

void PrintResults(const Cli& cli, const std::vector<BenchResult>& results)
{
#if defined(USE_FMT)
  const char* buildName = "fmt-build";
#else
  const char* buildName = "std-build";
#endif

  std::cout << "build: " << buildName << "\n";
  std::cout << "measure: " << MeasureName(cli.Measure) << "\n";
  std::cout << "payload: " << PayloadName(cli.Payload) << "\n";

  if (cli.Measure == MeasureMode::Latency)
  {
    std::cout << "async: bounded-lossless" << "\n";
    std::cout
      << "queue: records=" << ASYNC_QUEUE_RECORD_CAPACITY
      << " where record queues are configurable"
      << ", bytes=" << ASYNC_QUEUE_BYTE_CAPACITY
      << " where byte queues are configurable"
      << "\n";
  }

  std::cout << "\n";

  if (cli.Measure == MeasureMode::Throughput)
  {
    PrintThroughputTable(results, "Median cycles per run", &BenchResult::Cycles);
    return;
  }

  PrintLatencyTables(results);
}
} // namespace bench
