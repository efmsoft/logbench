#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "spdlog_driver.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <vector>

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace bench
{
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;

class SpdlogDriver : public IBenchDriver
{
public:
  const char* GetLibName() const override
  {
    return "spdlog";
  }

  DriverCaps GetCaps() const override
  {
    return DriverCaps{false, false, true};
  }

  bool Setup(BenchMode mode, const std::string& filePath, MeasureMode measure) override
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
        fs::remove(filePath, ec);
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath, false));
      }
    }

    Async = (measure == MeasureMode::Latency);

    if (Async)
    {
      ThreadPool = std::make_shared<spdlog::details::thread_pool>(1u << 16u, 1u);
      Logger = std::make_shared<spdlog::async_logger>(
        "bench_spdlog",
        sinks.begin(),
        sinks.end(),
        ThreadPool,
        spdlog::async_overflow_policy::block);
    }
    else
    {
      Logger = std::make_shared<spdlog::logger>("bench_spdlog", sinks.begin(), sinks.end());
    }

    Logger->set_level(spdlog::level::info);
    Logger->flush_on(spdlog::level::off);
    Logger->set_pattern("%v");
    return true;
  }

  std::function<void(void)> MakeLogOnce(FormatType) override
  {
    return [this, value = 0]() mutable
    {
      ++value;
      Logger->info("value is {}", value);
    };
  }

  uint64_t TeardownAndDrainNs() override
  {
    auto start = Clock::now();

    if (Logger)
    {
      Logger->flush();
    }

    Logger.reset();
    ThreadPool.reset();

    auto end = Clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
  }

private:
  std::shared_ptr<spdlog::logger> Logger;
  std::shared_ptr<spdlog::details::thread_pool> ThreadPool;
  bool Async = false;
};

std::unique_ptr<IBenchDriver> CreateSpdlogDriver()
{
  return std::make_unique<SpdlogDriver>();
}
} // namespace bench
