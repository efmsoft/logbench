#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <Logme/Logme.h>
#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/FileBackend.h>

#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(disable: 4840)
#endif

#if BENCH_WITH_QUILL
  #include <quill/Backend.h>
  #include <quill/Frontend.h>
  #include <quill/LogMacros.h>
  #include <quill/Logger.h>
  #include <quill/sinks/ConsoleSink.h>
  #include <quill/sinks/FileSink.h>
  #include <quill/sinks/RotatingFileSink.h>
  #include <quill/sinks/NullSink.h>
#endif

#if BENCH_WITH_EASYLOGGING
  #include "easylogging++.h"
  INITIALIZE_EASYLOGGINGPP
#endif

using Clock = std::chrono::steady_clock;

enum class BenchMode
{
  Null,
  File,
  Console,
  FileConsole
};

enum class FmtMode
{
  Native,
  LogmeC,
  LogmeCppStream,
  LogmeStdFormat
};

struct BenchCase
{
  std::string Lib;
  BenchMode Mode;
  FmtMode Fmt;
};

struct BenchResult
{
  std::string Lib;
  std::string Mode;
  std::string Fmt;
  uint64_t Cycles = 0;
};

static const char* ModeName(BenchMode mode)
{
  switch (mode)
  {
    case BenchMode::Null: return "null";
    case BenchMode::File: return "file";
    case BenchMode::Console: return "console";
    case BenchMode::FileConsole: return "file+console";
  }
  return "unknown";
}

static const char* FmtName(FmtMode fmt)
{
  switch (fmt)
  {
    case FmtMode::Native: return "native";
    case FmtMode::LogmeC: return "c";
    case FmtMode::LogmeCppStream: return "cpp-stream";
    case FmtMode::LogmeStdFormat: return "std::format";
  }
  return "unknown";
}

static uint64_t Median(std::vector<uint64_t> v)
{
  if (v.empty())
    return 0;

  std::sort(v.begin(), v.end());
  return v[v.size() / 2];
}

static uint64_t RunOnce(
  int seconds
  , int warmupMs
  , std::function<void(void)>&& logOnce
)
{
  std::atomic_bool stop(false);
  uint64_t cycles = 0;

  std::thread worker(
    [&]()
    {
      auto warmEnd = Clock::now() + std::chrono::milliseconds(warmupMs);
      while (Clock::now() < warmEnd)
      {
        logOnce();
      }

      auto end = Clock::now() + std::chrono::seconds(seconds);
      while ((stop.load(std::memory_order_relaxed) == false) &&
             (Clock::now() < end))
      {
        logOnce();
        ++cycles;
      }
    }
  );

  std::this_thread::sleep_for(std::chrono::seconds(seconds));
  stop.store(true, std::memory_order_relaxed);

  worker.join();
  return cycles;
}

static void PrintTable(const std::vector<BenchResult>& results)
{
  // Pivot results:
  //   rows: libraries (+ formatting variants as part of row label)
  //   cols: modes (null/file/console/file+console)
  struct Row
  {
    std::string Name;
    uint64_t Cells[4]{};
    bool Has[4]{};
  };

  auto modeIndex = [](const std::string& m) -> int
  {
    if (m == "null") return 0;
    if (m == "file") return 1;
    if (m == "console") return 2;
    if (m == "file+console") return 3;
    return -1;
  };

  auto rowKey = [](const BenchResult& r) -> std::string
  {
    if (r.Fmt == "native")
      return r.Lib;

    return r.Lib + " (" + r.Fmt + ")";
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

  for (const auto& r : results)
  {
    int mi = modeIndex(r.Mode);
    if (mi < 0)
      continue;

    auto* row = getRow(rowKey(r));
    row->Cells[mi] = r.Cycles;
    row->Has[mi] = true;
  }

  auto maxName = size_t{7};
  for (const auto& row : rows)
    maxName = (std::max)(maxName, row.Name.size());

  auto printCell = [](bool has, uint64_t v)
  {
    if (!has)
    {
      std::cout << "-";
      return;
    }
    std::cout << v;
  };

  // Header
  std::cout
    << std::left << std::setw((int)maxName) << "Library"
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

  // Rows
  for (const auto& row : rows)
  {
    std::cout << std::left << std::setw((int)maxName) << row.Name << " | ";

    for (int i = 0; i < 4; ++i)
    {
      std::ostringstream oss;
      if (!row.Has[i])
        oss << "-";
      else
        oss << row.Cells[i];

      std::cout << std::right << std::setw(12) << oss.str();
      if (i != 3)
        std::cout << " | ";
    }
    std::cout << "\n";
  }
}

struct Cli
{
  int Seconds = 3;
  int Repeat = 5;
  int WarmupMs = 300;
  std::string OutDir = ".";
  std::string Filter;
};

static bool StartsWith(const std::string& s, const char* pfx)
{
  return s.rfind(pfx, 0) == 0;
}

static std::vector<std::string> SplitFilter(const std::string& filter)
{
  std::vector<std::string> tokens;
  std::string token;

  for (char ch : filter)
  {
    if (ch == ',')
    {
      if (!token.empty())
      {
        tokens.push_back(token);
        token.clear();
      }
      continue;
    }

    token.push_back(ch);
  }

  if (!token.empty())
    tokens.push_back(token);

  return tokens;
}

static bool IsLibTokenMatch(const std::string& token, const std::string& lib)
{
  if (token == "easylogging" && lib == "easylogging++")
    return true;

  return token == lib;
}

static bool IsModeToken(const std::string& token)
{
  return token == "null" ||
         token == "file" ||
         token == "console" ||
         token == "file+console";
}

static bool IsFmtToken(const std::string& token)
{
  return token == "native" ||
         token == "c" ||
         token == "cpp-stream" ||
         token == "std::format" ||
         token == "std-format";
}

static bool IsLibToken(const std::string& token)
{
  return token == "logme" ||
         token == "spdlog" ||
         token == "quill" ||
         token == "easylogging" ||
         token == "easylogging++";
}

static bool MatchFilter(const std::string& filter, const BenchCase& benchCase)
{
  if (filter.empty())
    return true;

  auto tokens = SplitFilter(filter);

  for (const auto& token : tokens)
  {
    if (IsLibToken(token))
    {
      if (!IsLibTokenMatch(token, benchCase.Lib))
        return false;
      continue;
    }

    if (IsModeToken(token))
    {
      if (token != ModeName(benchCase.Mode))
        return false;
      continue;
    }

    if (IsFmtToken(token))
    {
      if (token == "std-format")
      {
        if (benchCase.Fmt != FmtMode::LogmeStdFormat)
          return false;
      }
      else if (token != FmtName(benchCase.Fmt))
      {
        return false;
      }
      continue;
    }

    return false;
  }

  return true;
}

static Cli ParseCli(int argc, char** argv)
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
    else if (a == "--help" || a == "-h")
    {
      std::cout
        << "Usage: logbench [--seconds=N] [--repeat=N] [--warmup-ms=N] [--outdir=PATH] [--filter=ITEMS]\n"
        << "Default: --seconds=3 --repeat=5 --warmup-ms=300 --outdir=.\n"
        << "Filter tokens (comma-separated): library, mode, format.\n"
        << "Examples:\n"
        << "  --filter=logme\n"
        << "  --filter=file\n"
        << "  --filter=std::format\n"
        << "  --filter=logme,file\n"
        << "  --filter=logme,file,std::format\n";
      std::exit(0);
    }
  }

  return cli;
}

static std::string JoinPath(const std::string& dir, const std::string& file)
{
  if (dir.empty() || dir == ".")
    return file;

  char sep =
#if defined(_WIN32)
    '\\';
#else
    '/';
#endif

  if (dir.back() == sep)
    return dir + file;

  return dir + sep + file;
}

static Logme::OutputFlags MinimalLogmeFlags()
{
  Logme::OutputFlags f;
  f.Value = 0;
  f.Eol = 1;
  return f;
}

struct LogmeDriver
{
  Logme::ID ChId{"bench"};
  Logme::ChannelPtr Ch;
  std::shared_ptr<Logme::FileBackend> File;

  bool Setup(BenchMode mode, const std::string& filePath)
  {
    static std::atomic<int> unique(0);
    int n = unique.fetch_add(1, std::memory_order_relaxed);

    std::string id = "bench_" + std::string(ModeName(mode)) + "_" + std::to_string(n);
    ChId = Logme::ID{id.c_str()};

    Ch = Logme::Instance->CreateChannel(ChId);
    if (!Ch)
      return false;

    Ch->SetFilterLevel(Logme::LEVEL_DEBUG);
    Ch->SetFlags(MinimalLogmeFlags());

    if (mode == BenchMode::Console || mode == BenchMode::FileConsole)
    {
      Ch->AddBackend(std::make_shared<Logme::ConsoleBackend>(Ch));
    }

    if (mode == BenchMode::File || mode == BenchMode::FileConsole)
    {
      File = std::make_shared<Logme::FileBackend>(Ch);
      File->SetMaxSize(128 * 1024 * 1024); // To avoid rotation during benchmark
      File->SetAppend(false);

      std::error_code ec;
      std::filesystem::remove(filePath, ec);

      if (File->CreateLog(filePath.c_str()) == false)
        return false;

      Ch->AddBackend(File);
    }

    return true;
  }

  void Teardown()
  {
    File.reset();
    Ch->RemoveBackends();
    Ch.reset();

    Logme::Instance->DeleteChannel(ChId);
  }

  std::function<void(void)> MakeLogOnce(FmtMode fmt)
  {
    // Use a changing integer to avoid overly aggressive constant folding.
    static thread_local int v = 0;

    if (fmt == FmtMode::LogmeC)
    {
      return [this]()
      {
        ++v;
        LogmeI(Ch, "value is %i", v);
      };
    }

    if (fmt == FmtMode::LogmeCppStream)
    {
      return [this]()
      {
        ++v;
        LogmeI(Ch) << "value is " << v;
      };
    }

    // std::format family
    return [this]()
    {
      ++v;
      fLogmeI(Ch, "value is {}", v);
    };
  }
};

struct SpdlogDriver
{
  std::shared_ptr<spdlog::logger> Logger;

  bool Setup(BenchMode mode, const std::string& filePath)
  {
    std::vector<spdlog::sink_ptr> sinks;

    if (mode == BenchMode::Null)
    {
      sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
    }
    else
    {
      if (mode == BenchMode::Console || mode == BenchMode::FileConsole)
      {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
      }

      if (mode == BenchMode::File || mode == BenchMode::FileConsole)
      {
        std::error_code ec;
        std::filesystem::remove(filePath, ec);
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath, false));
      }
    }

    Logger = std::make_shared<spdlog::logger>("bench_spdlog", sinks.begin(), sinks.end());
    Logger->set_level(spdlog::level::info);
    Logger->flush_on(spdlog::level::err);
    Logger->set_pattern("%v");

    return true;
  }

  void Teardown()
  {
    Logger.reset();
  }

  std::function<void(void)> MakeLogOnce()
  {
    static thread_local int v = 0;

    return [this]()
    {
      ++v;
      Logger->info("value is {}", v);
    };
  }
};

#if BENCH_WITH_QUILL
struct CustomQuillFrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedBlocking;
  static constexpr size_t initial_queue_capacity = 64u * 1024u;
  static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
  static constexpr size_t unbounded_queue_max_capacity = 2ull * 1024u * 1024u * 1024u;
  static constexpr quill::HugePagesPolicy huge_pages_policy = quill::HugePagesPolicy::Never;
};

using QuillFrontend = quill::FrontendImpl<CustomQuillFrontendOptions>;
using QuillLogger = quill::LoggerImpl<CustomQuillFrontendOptions>;

struct QuillDriver
{
  QuillLogger* Logger = nullptr;

  bool Setup(BenchMode mode, const std::string& filePath)
  {
    static std::once_flag once;
    std::call_once(
      once,
      []()
      {
        quill::BackendOptions backend_options;
        backend_options.error_notifier =
          [](std::string const&) noexcept
          {
          };

        quill::SignalHandlerOptions signal_handler_options;
        quill::Backend::start<CustomQuillFrontendOptions>(
          backend_options,
          signal_handler_options);
      }
    );

    static std::atomic<int> unique(0);
    int n = unique.fetch_add(1, std::memory_order_relaxed);

    std::string loggerName = "bench_quill_" + std::string(ModeName(mode)) + "_" + std::to_string(n);

    if (mode == BenchMode::Null)
    {
      auto sink = QuillFrontend::create_or_get_sink<quill::NullSink>(
        "quill_null_" + std::to_string(n));
      Logger = QuillFrontend::create_or_get_logger(loggerName, std::move(sink));
      return Logger != nullptr;
    }

    std::vector<std::shared_ptr<quill::Sink>> sinks;

    if (mode == BenchMode::Console || mode == BenchMode::FileConsole)
    {
      sinks.push_back(
        QuillFrontend::create_or_get_sink<quill::ConsoleSink>(
          "quill_console_" + std::to_string(n)));
    }

    if (mode == BenchMode::File || mode == BenchMode::FileConsole)
    {
      auto file_sink = QuillFrontend::create_or_get_sink<quill::RotatingFileSink>(
        filePath,
        []()
        {
          quill::RotatingFileSinkConfig cfg;
          cfg.set_open_mode('w');
          cfg.set_rotation_on_creation(true);
          cfg.set_rotation_max_file_size(256u * 1024u * 1024u);
          cfg.set_max_backup_files(1);
          cfg.set_overwrite_rolled_files(true);
          cfg.set_remove_old_files(true);
          return cfg;
        }());
      sinks.push_back(std::move(file_sink));
    }

    Logger = QuillFrontend::create_or_get_logger(loggerName, std::move(sinks));
    return Logger != nullptr;
  }

  void Teardown()
  {
    if (Logger)
    {
      QuillFrontend::remove_logger_blocking(Logger);
      Logger = nullptr;
    }
  }

  std::function<void(void)> MakeLogOnce()
  {
    static thread_local int v = 0;

    return [this]()
    {
      ++v;
      LOG_INFO(Logger, "value is {}", v);
    };
  }
};
#endif

#if BENCH_WITH_EASYLOGGING
struct EasyloggingDriver
{
  bool Setup(BenchMode mode, const std::string& filePath)
  {
    el::Configurations conf;
    conf.setToDefault();

    conf.set(el::Level::Info, el::ConfigurationType::Enabled, "true");
    conf.set(el::Level::Info, el::ConfigurationType::Format, "%msg");
    conf.set(el::Level::Info, el::ConfigurationType::ToStandardOutput,
             (mode == BenchMode::Console || mode == BenchMode::FileConsole) ? "true" : "false");
    conf.set(el::Level::Info, el::ConfigurationType::ToFile,
             (mode == BenchMode::File || mode == BenchMode::FileConsole) ? "true" : "false");

    if (mode == BenchMode::File || mode == BenchMode::FileConsole)
    {
      conf.set(el::Level::Global, el::ConfigurationType::Filename, filePath);
    }

    el::Loggers::reconfigureAllLoggers(conf);
    return true;
  }

  void Teardown()
  {
  }

  std::function<void(void)> MakeLogOnce()
  {
    static thread_local int v = 0;

    return []()
    {
      ++v;
      LOG(INFO) << "value is " << v;
    };
  }
};
#endif

int main(int argc, char** argv)
{
  auto cli = ParseCli(argc, argv);

  std::vector<BenchResult> results;

  const BenchMode modes[] =
  {
    BenchMode::Null,
    BenchMode::File,
    BenchMode::Console,
    BenchMode::FileConsole
  };

  const FmtMode fmts[] =
  {
    FmtMode::LogmeC,
    FmtMode::LogmeCppStream,
    FmtMode::LogmeStdFormat
  };

  for (auto mode : modes)
  {
    bool runLogmeMode = false;

    for (auto fmt : fmts)
    {
      if (MatchFilter(cli.Filter, BenchCase{"logme", mode, fmt}))
      {
        runLogmeMode = true;
        break;
      }
    }

    if (!runLogmeMode)
      continue;

    LogmeDriver d;
    auto filePath = JoinPath(cli.OutDir, "logme_bench.log");

    if (!d.Setup(mode, filePath))
    {
      std::cerr << "logme setup failed for mode " << ModeName(mode) << "\n";
      continue;
    }

    for (auto fmt : fmts)
    {
      if (!MatchFilter(cli.Filter, BenchCase{"logme", mode, fmt}))
        continue;

      BenchResult r;
      r.Lib = "logme";
      r.Mode = ModeName(mode);
      r.Fmt = FmtName(fmt);

#if defined(LOGME_DISABLE_STD_FORMAT)
      if (fmt == FmtMode::LogmeStdFormat)
      {
        r.Cycles = 0;
        results.push_back(r);
        continue;
      }
#endif

      std::vector<uint64_t> runs;
      runs.reserve((size_t)cli.Repeat);

      for (int i = 0; i < cli.Repeat; ++i)
      {
        auto logOnce = d.MakeLogOnce(fmt);
        runs.push_back(RunOnce(cli.Seconds, cli.WarmupMs, std::move(logOnce)));
      }

      r.Cycles = Median(runs);
      results.push_back(r);
    }

    d.Teardown();
  }

  for (auto mode : modes)
  {
    if (!MatchFilter(cli.Filter, BenchCase{"spdlog", mode, FmtMode::Native}))
      continue;

    SpdlogDriver d;
    auto filePath = JoinPath(cli.OutDir, "spdlog_bench.log");

    if (!d.Setup(mode, filePath))
    {
      std::cerr << "spdlog setup failed for mode " << ModeName(mode) << "\n";
      continue;
    }

    std::vector<uint64_t> runs;
    runs.reserve((size_t)cli.Repeat);

    for (int i = 0; i < cli.Repeat; ++i)
    {
      auto logOnce = d.MakeLogOnce();
      runs.push_back(RunOnce(cli.Seconds, cli.WarmupMs, std::move(logOnce)));
    }

    BenchResult r;
    r.Lib = "spdlog";
    r.Mode = ModeName(mode);
    r.Fmt = "native";
    r.Cycles = Median(runs);
    results.push_back(r);

    d.Teardown();
  }

#if BENCH_WITH_QUILL
  for (auto mode : modes)
  {
    if (!MatchFilter(cli.Filter, BenchCase{"quill", mode, FmtMode::Native}))
      continue;

    QuillDriver d;
    auto filePath = JoinPath(cli.OutDir, "quill_bench.log");

    if (!d.Setup(mode, filePath))
    {
      std::cerr << "quill setup failed for mode " << ModeName(mode) << "\n";
      continue;
    }

    std::vector<uint64_t> runs;
    runs.reserve((size_t)cli.Repeat);

    for (int i = 0; i < cli.Repeat; ++i)
    {
      auto logOnce = d.MakeLogOnce();
      runs.push_back(RunOnce(cli.Seconds, cli.WarmupMs, std::move(logOnce)));
    }

    BenchResult r;
    r.Lib = "quill";
    r.Mode = ModeName(mode);
    r.Fmt = "native";
    r.Cycles = Median(runs);
    results.push_back(r);

    d.Teardown();
  }
#endif

#if BENCH_WITH_EASYLOGGING
  for (auto mode : modes)
  {
    if (!MatchFilter(cli.Filter, BenchCase{"easylogging++", mode, FmtMode::Native}))
      continue;

    EasyloggingDriver d;
    auto filePath = JoinPath(cli.OutDir, "easylogging_bench.log");

    if (!d.Setup(mode, filePath))
    {
      std::cerr << "easylogging++ setup failed for mode " << ModeName(mode) << "\n";
      continue;
    }

    std::vector<uint64_t> runs;
    runs.reserve((size_t)cli.Repeat);

    for (int i = 0; i < cli.Repeat; ++i)
    {
      auto logOnce = d.MakeLogOnce();
      runs.push_back(RunOnce(cli.Seconds, cli.WarmupMs, std::move(logOnce)));
    }

    BenchResult r;
    r.Lib = "easylogging++";
    r.Mode = ModeName(mode);
    r.Fmt = "native";
    r.Cycles = Median(runs);
    results.push_back(r);

    d.Teardown();
  }
#endif

  PrintTable(results);
  return 0;
}