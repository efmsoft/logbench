#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "plog_driver.h"

#if BENCH_WITH_PLOG
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#include "../bench_util.h"

#include <plog/Init.h>
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Formatters/MessageOnlyFormatter.h>

namespace bench
{
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;

class PLogNullAppender : public plog::IAppender
{
public:
    void write(const plog::Record&) override {}
};

static std::unique_ptr<PLogNullAppender> g_NullAppender;
static std::unique_ptr<plog::ConsoleAppender<plog::MessageOnlyFormatter>> g_ConsoleAppender;
static std::unique_ptr<plog::RollingFileAppender<plog::MessageOnlyFormatter>> g_FileAppender;
static std::unique_ptr<plog::ConsoleAppender<plog::MessageOnlyFormatter>> g_MultiConsoleAppender;
static std::unique_ptr<plog::RollingFileAppender<plog::MessageOnlyFormatter>> g_MultiFileAppender;

class PlogDriver : public IBenchDriver
{
public:
    const char* GetLibName() const override { return "plog"; }

    DriverCaps GetCaps() const override { return DriverCaps{ true, true, false }; }

    bool Setup(BenchMode mode, const std::string& filePath, MeasureMode) override
    {
        Value = 0;
        Mode = mode;

        std::error_code ec;
        fs::remove(filePath, ec);
        fs::create_directories(fs::path(filePath).parent_path(), ec);
        
        if (mode == BenchMode::Null)
        {
            if (!g_NullAppender) 
            {
                g_NullAppender = std::make_unique<PLogNullAppender>();
                plog::init<0>(plog::info, g_NullAppender.get());
            }
            return true;
        }

        if (mode == BenchMode::Console)
        {
            if (!g_ConsoleAppender) {
                g_ConsoleAppender = std::make_unique<plog::ConsoleAppender<plog::MessageOnlyFormatter>>();
                plog::init<1>(plog::info, g_ConsoleAppender.get());
            }
            return true;
        }

        if (mode == BenchMode::File)
        {
            if (!g_FileAppender) {
                g_FileAppender = std::make_unique<plog::RollingFileAppender<plog::MessageOnlyFormatter>>(filePath.c_str(), 0, 0);
                plog::init<2>(plog::info, g_FileAppender.get());
            }
            return true;
        }

        if (mode == BenchMode::FileConsole)
        {
            if (!g_MultiConsoleAppender) {
                g_MultiConsoleAppender = std::make_unique<plog::ConsoleAppender<plog::MessageOnlyFormatter>>();
                g_MultiFileAppender = std::make_unique<plog::RollingFileAppender<plog::MessageOnlyFormatter>>(filePath.c_str(), 0, 0);
                plog::init<3>(plog::info, g_MultiConsoleAppender.get()).addAppender(g_MultiFileAppender.get());
            }
            return true;
        }

        return false;
    }

    std::function<void(void)> MakeLogOnce(FormatType format, PayloadType payload) override
    {
        switch (Mode)
        {
        case BenchMode::Null:
            return MakeModeLogOnce<0>(format, payload);

        case BenchMode::Console:
            return MakeModeLogOnce<1>(format, payload);

        case BenchMode::File:
            return MakeModeLogOnce<2>(format, payload);

        case BenchMode::FileConsole:
            return MakeModeLogOnce<3>(format, payload);
        }
        return {};
    }

    uint64_t TeardownAndDrainNs() override
    {
        return 0;
    }

private:
    template<int Instance>
    std::function<void(void)> MakeModeLogOnce(FormatType format, PayloadType payload)
    {
        if (format == FormatType::C)
        {
            if (payload == PayloadType::DynamicString)
            {
                return [this]()
                {
                    std::string message = MakeDynamicString(++Value);
                    PLOG_(Instance, plog::info).printf("%s", message.c_str());
                };
            }

            return [this]()
            {
                PLOG_(Instance, plog::info).printf("value is %d", ++Value);
            };
        }

        if (payload == PayloadType::DynamicString)
        {
            return [this]()
            {
                std::string message = MakeDynamicString(++Value);
                PLOG_(Instance, plog::info) << message;
            };
        }

        return [this]()
        {
            PLOG_(Instance, plog::info) << "value is " << ++Value;
        };
    }

private:
    BenchMode Mode = BenchMode::Null;
    int Value = 0;
};

std::unique_ptr<IBenchDriver> CreatePlogDriver()
{
    return std::make_unique<PlogDriver>();
}
} // namespace bench
#else
namespace bench
{
std::unique_ptr<IBenchDriver> CreatePlogDriver() { return nullptr; }
} // namespace bench
#endif