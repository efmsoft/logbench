#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "boost_log_driver.h"

#if BENCH_WITH_BOOST_LOG
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <streambuf>
#include <vector>

#include <boost/core/null_deleter.hpp>
#include <boost/make_shared.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>

#include "../bench_types.h"

namespace bench
{
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;
namespace bl = boost::log;
namespace bls = boost::log::sources;
namespace blsink = boost::log::sinks;
namespace blexpr = boost::log::expressions;

class BoostNullStream : public std::ostream
{
public:
  BoostNullStream()
    : std::ostream(&Buf)
  {
  }

private:
  class NullBuf : public std::streambuf
  {
  public:
    int overflow(int c) override
    {
      return c;
    }
  } Buf;
};

class BoostLogDriver : public IBenchDriver
{
public:
  using OstreamBackend = blsink::text_ostream_backend;
  using FileBackend = blsink::text_file_backend;
  using SyncOstreamSink = blsink::synchronous_sink<OstreamBackend>;
  using AsyncOstreamSink = blsink::asynchronous_sink<OstreamBackend>;
  using SyncFileSink = blsink::synchronous_sink<FileBackend>;
  using AsyncFileSink = blsink::asynchronous_sink<FileBackend>;

  const char* GetLibName() const override
  {
    return "boost.log";
  }

  DriverCaps GetCaps() const override
  {
    return DriverCaps{false, true, false};
  }

  bool Setup(BenchMode mode, const std::string& filePath, MeasureMode measure) override
  {
    Measure = measure;
    Sinks.clear();
    NullStream.reset();

    if (mode == BenchMode::Null)
    {
      NullStream = std::make_shared<BoostNullStream>();
      AddOstreamSink(std::shared_ptr<std::ostream>(NullStream, NullStream.get()));
      return true;
    }

    if (mode == BenchMode::Console || mode == BenchMode::FileConsole)
    {
      std::shared_ptr<std::ostream> stream(&std::clog, [](std::ostream*) {});
      AddOstreamSink(stream);
    }

    if (mode == BenchMode::File || mode == BenchMode::FileConsole)
    {
      std::error_code ec;
      fs::remove(filePath, ec);
      AddFileSink(filePath);
    }

    return !Sinks.empty();
  }

  std::function<void(void)> MakeLogOnce(FormatType) override
  {
    return [this, value = 0]() mutable
    {
      ++value;
      BOOST_LOG_SEV(Logger, bl::trivial::info) << "value is " << value;
    };
  }

  uint64_t TeardownAndDrainNs() override
  {
    auto start = Clock::now();

    for (auto& sink : Sinks)
    {
      boost::shared_ptr<blsink::sink> sharedSink = sink;
      bl::core::get()->remove_sink(sharedSink);
      sink->flush();
    }

    NullStream.reset();
    Sinks.clear();

    auto end = Clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
  }

private:
  void AddOstreamSink(const std::shared_ptr<std::ostream>& stream)
  {
    if (Measure == MeasureMode::Latency)
    {
      auto sink = boost::make_shared<AsyncOstreamSink>();
      sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(stream.get(), boost::null_deleter()));
      sink->set_formatter(blexpr::stream << blexpr::smessage);
      bl::core::get()->add_sink(sink);
      Sinks.push_back(sink);
      return;
    }

    auto sink = boost::make_shared<SyncOstreamSink>();
    sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(stream.get(), boost::null_deleter()));
    sink->set_formatter(blexpr::stream << blexpr::smessage);
    bl::core::get()->add_sink(sink);
    Sinks.push_back(sink);
  }

  void AddFileSink(const std::string& filePath)
  {
    if (Measure == MeasureMode::Latency)
    {
      auto backend = boost::make_shared<FileBackend>(
        bl::keywords::file_name = filePath,
        bl::keywords::open_mode = std::ios_base::out | std::ios_base::trunc,
        bl::keywords::auto_flush = false);
      auto sink = boost::make_shared<AsyncFileSink>(backend);
      sink->set_formatter(blexpr::stream << blexpr::smessage);
      bl::core::get()->add_sink(sink);
      Sinks.push_back(sink);
      return;
    }

    auto backend = boost::make_shared<FileBackend>(
      bl::keywords::file_name = filePath,
      bl::keywords::open_mode = std::ios_base::out | std::ios_base::trunc,
      bl::keywords::auto_flush = false);
    auto sink = boost::make_shared<SyncFileSink>(backend);
    sink->set_formatter(blexpr::stream << blexpr::smessage);
    bl::core::get()->add_sink(sink);
    Sinks.push_back(sink);
  }

private:
  bls::severity_logger_mt<bl::trivial::severity_level> Logger;
  std::vector<boost::shared_ptr<blsink::sink>> Sinks;
  std::shared_ptr<BoostNullStream> NullStream;
  MeasureMode Measure = MeasureMode::Throughput;
};

std::unique_ptr<IBenchDriver> CreateBoostLogDriver()
{
  return std::make_unique<BoostLogDriver>();
}
} // namespace bench
#else
namespace bench
{
std::unique_ptr<IBenchDriver> CreateBoostLogDriver()
{
  return nullptr;
}
} // namespace bench
#endif
