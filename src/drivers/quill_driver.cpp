#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "quill_driver.h"

#if BENCH_WITH_QUILL
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/NullSink.h>
#include <quill/sinks/FileSink.h>

#include "../bench_util.h"

namespace bench
{
using Clock = std::chrono::steady_clock;

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

class QuillDriver : public IBenchDriver
{
public:
  const char* GetLibName() const override
  {
    return "quill";
  }

  DriverCaps GetCaps() const override
  {
    return DriverCaps{false, false, true};
  }

  bool Setup(BenchMode mode, const std::string& filePath, MeasureMode) override
  {
    static std::once_flag once;
    std::call_once(
      once,
      []()
      {
        quill::BackendOptions backendOptions;
        backendOptions.error_notifier =
          [](std::string const&) noexcept
          {
          };

        quill::SignalHandlerOptions signalHandlerOptions;
        quill::Backend::start<CustomQuillFrontendOptions>(backendOptions, signalHandlerOptions);
      });

    static int unique = 0;
    ++unique;

    std::string loggerName = "bench_quill_" + std::string(ModeName(mode)) + "_" + std::to_string(unique);

    if (mode == BenchMode::Null)
    {
      auto sink = QuillFrontend::create_or_get_sink<quill::NullSink>("quill_null_" + std::to_string(unique));
      Logger = QuillFrontend::create_or_get_logger(loggerName, std::move(sink));
      return Logger != nullptr;
    }

    std::vector<std::shared_ptr<quill::Sink>> sinks;

    if (mode == BenchMode::Console || mode == BenchMode::FileConsole)
    {
      sinks.push_back(
        QuillFrontend::create_or_get_sink<quill::ConsoleSink>(
          "quill_console_" + std::to_string(unique)));
    }

    if (mode == BenchMode::File || mode == BenchMode::FileConsole)
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      auto fileSink = QuillFrontend::create_or_get_sink<quill::FileSink>(filePath, cfg);
      sinks.push_back(std::move(fileSink));
    }

    quill::PatternFormatterOptions pattern;
    pattern.format_pattern = "%(message)";

    Logger = QuillFrontend::create_or_get_logger(loggerName, std::move(sinks), pattern);
    return Logger != nullptr;
  }

  std::function<void(void)> MakeLogOnce(FormatType) override
  {
    return [this, value = 0]() mutable
    {
      ++value;
      LOG_INFO(Logger, "value is {}", value);
    };
  }

  uint64_t TeardownAndDrainNs() override
  {
    auto start = Clock::now();

    if (Logger)
    {
      QuillFrontend::remove_logger_blocking(Logger);
      Logger = nullptr;
    }

    auto end = Clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
  }

private:
  QuillLogger* Logger = nullptr;
};

std::unique_ptr<IBenchDriver> CreateQuillDriver()
{
  return std::make_unique<QuillDriver>();
}
} // namespace bench
#else
namespace bench
{
std::unique_ptr<IBenchDriver> CreateQuillDriver()
{
  return nullptr;
}
} // namespace bench
#endif
