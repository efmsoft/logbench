#pragma once

#include <functional>
#include <memory>
#include <string>

#include "bench_types.h"

namespace bench
{
class IBenchDriver
{
public:
  virtual ~IBenchDriver() = default;

  virtual const char* GetLibName() const = 0;
  virtual DriverCaps GetCaps() const = 0;
  virtual bool Setup(BenchMode mode, const std::string& filePath, MeasureMode measure) = 0;
  virtual std::function<void(void)> MakeLogOnce(FormatType format) = 0;
  virtual uint64_t TeardownAndDrainNs() = 0;
};
} // namespace bench
