#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "g3log_driver.h"

#if BENCH_WITH_G3LOG
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <g3log/g3log.hpp>
#include <g3log/logmessage.hpp>
#include <g3log/logworker.hpp>

#include "../bench_types.h"

namespace bench
{
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;

struct G3BenchSink
{
  bool ToConsole = false;
  bool ToFile = false;
  std::ofstream File;

  void Configure(BenchMode mode, const std::string& filePath)
  {
    ToConsole = (mode == BenchMode::Console || mode == BenchMode::FileConsole);
    ToFile = (mode == BenchMode::File || mode == BenchMode::FileConsole);

    if (ToFile)
    {
      std::error_code ec;
      fs::create_directories(fs::path(filePath).parent_path(), ec);
      fs::remove(filePath, ec);
      File.open(filePath, std::ios::out | std::ios::trunc);
    }
  }

  void Save(g3::LogMessageMover message)
  {
    const std::string text = message.get().message() + "\n";

    if (ToConsole)
    {
      std::cout << text;
      std::cout.flush();
    }

    if (ToFile && File.is_open())
    {
      File << text;
    }
  }

  void Flush()
  {
    if (ToConsole)
    {
      std::cout.flush();
    }

    if (File.is_open())
    {
      File.flush();
      File.close();
    }
  }
};

class G3logDriver : public IBenchDriver
{
public:
  const char* GetLibName() const override
  {
    return "g3log";
  }

  DriverCaps GetCaps() const override
  {
    return DriverCaps{true, true, false};
  }

  bool Setup(BenchMode mode, const std::string& filePath, MeasureMode) override
  {
    Worker = g3::LogWorker::createLogWorker();
    SinkHandle = Worker->addSink(std::make_unique<G3BenchSink>(), &G3BenchSink::Save);
    SinkHandle->call(&G3BenchSink::Configure, mode, filePath).get();
    g3::initializeLogging(Worker.get());
    return true;
  }

  std::function<void(void)> MakeLogOnce(FormatType format) override
  {
    if (format == FormatType::C)
    {
      return [this, value = 0]() mutable
      {
        ++value;
        LOGF(INFO, "value is %i", value);
      };
    }

    return [this, value = 0]() mutable
    {
      ++value;
      LOG(INFO) << "value is " << value;
    };
  }

  uint64_t TeardownAndDrainNs() override
  {
    auto start = Clock::now();

    if (SinkHandle)
    {
      SinkHandle->call(&G3BenchSink::Flush).get();
    }

    SinkHandle.reset();
    Worker.reset();

    auto end = Clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
  }

private:
  std::unique_ptr<g3::LogWorker> Worker;
  std::unique_ptr<g3::SinkHandle<G3BenchSink>> SinkHandle;
};

std::unique_ptr<IBenchDriver> CreateG3logDriver()
{
  return std::make_unique<G3logDriver>();
}
} // namespace bench
#else
namespace bench
{
std::unique_ptr<IBenchDriver> CreateG3logDriver()
{
  return nullptr;
}
} // namespace bench
#endif
