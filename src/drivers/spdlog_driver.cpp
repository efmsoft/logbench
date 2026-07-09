#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "spdlog_driver.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <vector>

#include "../bench_util.h"

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
    Value = 0;
    Loggers.clear();
    ThreadPool.reset();

    if (measure == MeasureMode::Latency)
    {
      std::vector<spdlog::sink_ptr> sinks;
      AddSinks(sinks, mode, filePath);
      ThreadPool = std::make_shared<spdlog::details::thread_pool>(ASYNC_QUEUE_RECORD_CAPACITY, 1u);
      Loggers.push_back(
        std::make_shared<spdlog::async_logger>(
          "bench_spdlog_latency",
          sinks.begin(),
          sinks.end(),
          ThreadPool,
          spdlog::async_overflow_policy::block));
      ConfigureLogger(Loggers.back());
      return true;
    }

    if (mode == BenchMode::Null)
    {
      std::vector<spdlog::sink_ptr> sinks;
      sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
      Loggers.push_back(std::make_shared<spdlog::logger>("bench_spdlog_null", sinks.begin(), sinks.end()));
      ConfigureLogger(Loggers.back());
      return true;
    }

    if (mode == BenchMode::Console || mode == BenchMode::FileConsole)
    {
      std::vector<spdlog::sink_ptr> sinks;
      sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
      Loggers.push_back(std::make_shared<spdlog::logger>("bench_spdlog_console", sinks.begin(), sinks.end()));
      ConfigureLogger(Loggers.back());
    }

    if (mode == BenchMode::File || mode == BenchMode::FileConsole)
    {
      std::error_code ec;
      fs::create_directories(fs::path(filePath).parent_path(), ec);
      fs::remove(filePath, ec);

      std::vector<spdlog::sink_ptr> sinks;
      sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath, false));
      ThreadPool = std::make_shared<spdlog::details::thread_pool>(ASYNC_QUEUE_RECORD_CAPACITY, 1u);
      Loggers.push_back(
        std::make_shared<spdlog::async_logger>(
          "bench_spdlog_file",
          sinks.begin(),
          sinks.end(),
          ThreadPool,
          spdlog::async_overflow_policy::block));
      ConfigureLogger(Loggers.back());
    }

    return !Loggers.empty();
  }

  std::function<void(void)> MakeLogOnce(FormatType, PayloadType payload) override
  {
    if (payload == PayloadType::DynamicString)
    {
      return [this]()
      {
        std::string message = MakeDynamicString(++Value);
        for (auto& logger : Loggers)
        {
          logger->info("{}", message);
        }
      };
    }

    return [this]()
    {
      ++Value;
      for (auto& logger : Loggers)
      {
        logger->info("value is {}", Value);
      }
    };
  }

  uint64_t TeardownAndDrainNs() override
  {
    auto start = Clock::now();

    for (auto& logger : Loggers)
    {
      if (logger)
      {
        logger->flush();
      }
    }

    Loggers.clear();
    ThreadPool.reset();

    auto end = Clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
  }

private:
  void AddSinks(
    std::vector<spdlog::sink_ptr>& sinks
    , BenchMode mode
    , const std::string& filePath)
  {
    if (mode == BenchMode::Null)
    {
      sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
      return;
    }

    if (mode == BenchMode::Console || mode == BenchMode::FileConsole)
    {
      sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }

    if (mode == BenchMode::File || mode == BenchMode::FileConsole)
    {
      std::error_code ec;
      fs::create_directories(fs::path(filePath).parent_path(), ec);
      fs::remove(filePath, ec);
      sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath, false));
    }
  }

  void ConfigureLogger(const std::shared_ptr<spdlog::logger>& logger)
  {
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::off);
    logger->set_pattern("%v");
  }

private:
  std::vector<std::shared_ptr<spdlog::logger>> Loggers;
  std::shared_ptr<spdlog::details::thread_pool> ThreadPool;
  int Value = 0;
};

std::unique_ptr<IBenchDriver> CreateSpdlogDriver()
{
  return std::make_unique<SpdlogDriver>();
}
} // namespace bench
