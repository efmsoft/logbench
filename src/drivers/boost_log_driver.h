#pragma once

#include "../bench_driver.h"

namespace bench
{
std::unique_ptr<IBenchDriver> CreateBoostLogDriver();
} // namespace bench
