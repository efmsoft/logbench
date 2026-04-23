#include "easylogging_driver.h"

#if BENCH_WITH_EASYLOGGING
#include <chrono>

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

namespace bench
{
using Clock = std::chrono::steady_clock;

class EasyloggingDriver : public IBenchDriver
{
public:
  const char* GetLibName() const override
  {
    return "easylogging++";
  }

  DriverCaps GetCaps() const override
  {
    return DriverCaps{false, true, false};
  }

  bool Setup(BenchMode mode, const std::string& filePath, MeasureMode) override
  {
    el::Configurations conf;
    conf.setToDefault();

    conf.set(el::Level::Info, el::ConfigurationType::Enabled, "true");
    conf.set(el::Level::Info, el::ConfigurationType::Format, "%msg");
    conf.set(
      el::Level::Info,
      el::ConfigurationType::ToStandardOutput,
      (mode == BenchMode::Console || mode == BenchMode::FileConsole) ? "true" : "false");
    conf.set(
      el::Level::Info,
      el::ConfigurationType::ToFile,
      (mode == BenchMode::File || mode == BenchMode::FileConsole) ? "true" : "false");

    if (mode == BenchMode::File || mode == BenchMode::FileConsole)
    {
      conf.set(el::Level::Global, el::ConfigurationType::Filename, filePath);
    }

    el::Loggers::reconfigureAllLoggers(conf);
    return true;
  }

  std::function<void(void)> MakeLogOnce(FormatType) override
  {
    return [this, value = 0]() mutable
    {
      ++value;
      LOG(INFO) << "value is " << value;
    };
  }

  uint64_t TeardownAndDrainNs() override
  {
    auto start = Clock::now();
    el::Loggers::flushAll();
    auto end = Clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
  }
};

std::unique_ptr<IBenchDriver> CreateEasyloggingDriver()
{
  return std::make_unique<EasyloggingDriver>();
}
} // namespace bench
#else
namespace bench
{
std::unique_ptr<IBenchDriver> CreateEasyloggingDriver()
{
  return nullptr;
}
} // namespace bench
#endif
