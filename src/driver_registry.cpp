#include "driver_registry.h"

#include "drivers/boost_log_driver.h"
#include "drivers/easylogging_driver.h"
#include "drivers/g3log_driver.h"
#include "drivers/logme_driver.h"
#include "drivers/plog_driver.h"
#include "drivers/quill_driver.h"
#include "drivers/spdlog_driver.h"

namespace bench
{
std::vector<std::unique_ptr<IBenchDriver>> CreateDrivers()
{
  std::vector<std::unique_ptr<IBenchDriver>> drivers;

  drivers.push_back(CreateLogmeDriver());
  drivers.push_back(CreateSpdlogDriver());

#if BENCH_WITH_QUILL
  drivers.push_back(CreateQuillDriver());
#endif

#if BENCH_WITH_EASYLOGGING
  drivers.push_back(CreateEasyloggingDriver());
#endif

#if BENCH_WITH_BOOST_LOG
  drivers.push_back(CreateBoostLogDriver());
#endif

#if BENCH_WITH_G3LOG
  drivers.push_back(CreateG3logDriver());
#endif

#if BENCH_WITH_PLOG
  drivers.push_back(CreatePlogDriver());
#endif

  return drivers;
}
} // namespace bench
