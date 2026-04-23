#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "logme_driver.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Backend/FileBackend.h>
#include <Logme/Logme.h>

#include "../bench_util.h"

namespace bench
{
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;

static Logme::OutputFlags MinimalLogmeFlags()
{
  Logme::OutputFlags flags;
  flags.Value = 0;
  flags.Eol = 1;
  return flags;
}

class LogmeDriver : public IBenchDriver
{
public:
  const char* GetLibName() const override
  {
    return "logme";
  }

  DriverCaps GetCaps() const override
  {
    return DriverCaps{true, true, true};
  }

  bool Setup(BenchMode mode, const std::string& filePath, MeasureMode) override
  {
    static int unique = 0;
    ++unique;

    std::string id = "bench_" + std::string(ModeName(mode)) + "_" + std::to_string(unique);
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
      File->SetMaxSize(0);
      File->SetAppend(false);

      std::error_code ec;
      //fs::remove(filePath, ec);
      if (fs::exists(filePath))
      {
        if (!fs::remove(filePath, ec))
        {
          if (ec)
          {
            std::cerr << "Failed to remove file: " << ec.message() << std::endl;
          }
        }
      }

      if (File->CreateLog(filePath.c_str()) == false)
      {
        printf("Logme failed to create log file %s\n", filePath.c_str());
        return false;
      }
      Ch->AddBackend(File);
    }

    return true;
  }

  std::function<void(void)> MakeLogOnce(FormatType format) override
  {
    if (format == FormatType::C)
    {
      return [this, value = 0]() mutable
      {
        ++value;
        LogmeI(Ch, "value is %i", value);
      };
    }

    if (format == FormatType::Cpp)
    {
      return [this, value = 0]() mutable
      {
        ++value;
        LogmeI(Ch) << "value is " << value;
      };
    }

    return [this, value = 0]() mutable
    {
      ++value;
      fLogmeI(Ch, "value is {}", value);
    };
  }

  uint64_t TeardownAndDrainNs() override
  {
    auto start = Clock::now();

    File.reset();

    if (Ch)
    {
      Ch->Flush();
      Ch->RemoveBackends();
    }

    Ch.reset();
    Logme::Instance->DeleteChannel(ChId);

    auto end = Clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
  }

private:
  Logme::ID ChId{"bench"};
  Logme::ChannelPtr Ch;
  std::shared_ptr<Logme::FileBackend> File;
};

std::unique_ptr<IBenchDriver> CreateLogmeDriver()
{
  return std::make_unique<LogmeDriver>();
}
} // namespace bench
