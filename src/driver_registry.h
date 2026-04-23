#pragma once

#include <memory>
#include <vector>

#include "bench_driver.h"

namespace bench
{
std::vector<std::unique_ptr<IBenchDriver>> CreateDrivers();
} // namespace bench
