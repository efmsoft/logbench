#pragma once

#include "../bench_driver.h"

namespace bench
{
std::unique_ptr<IBenchDriver> CreatePlogDriver();
} // namespace bench
