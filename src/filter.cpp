#include "filter.h"

#include "bench_util.h"

namespace bench
{
std::vector<std::string> SplitFilter(const std::string& filter)
{
  std::vector<std::string> tokens;
  std::string token;

  for (char ch : filter)
  {
    if (ch == ',')
    {
      if (!token.empty())
      {
        tokens.push_back(token);
        token.clear();
      }
      continue;
    }

    token.push_back(ch);
  }

  if (!token.empty())
    tokens.push_back(token);

  return tokens;
}

static bool IsLibTokenMatch(const std::string& token, const std::string& lib)
{
  if (token == "easylogging" && lib == "easylogging++")
    return true;

  if (token == "boost" && lib == "boost.log")
    return true;

  return token == lib;
}

static bool IsModeToken(const std::string& token)
{
  return token == "null" ||
         token == "file" ||
         token == "console" ||
         token == "file+console";
}

static bool IsFormatToken(const std::string& token)
{
  return token == "c" ||
         token == "cpp" ||
         token == "cpp-stream" ||
         token == "fmt";
}

static bool IsMeasureToken(const std::string& token)
{
  return token == "throughput" || token == "latency";
}

static bool IsLibToken(const std::string& token)
{
  return token == "logme" ||
         token == "spdlog" ||
         token == "quill" ||
         token == "easylogging" ||
         token == "easylogging++" ||
         token == "boost" ||
         token == "boost.log" ||
         token == "g3log" ||
         token == "plog";
}

bool MatchFilter(const std::string& filter, const BenchCase& benchCase)
{
  if (filter.empty())
    return true;

  auto tokens = SplitFilter(filter);

  for (const auto& token : tokens)
  {
    if (IsLibToken(token))
    {
      if (!IsLibTokenMatch(token, benchCase.Lib))
        return false;
      continue;
    }

    if (IsModeToken(token))
    {
      if (token != ModeName(benchCase.Mode))
        return false;
      continue;
    }

    if (IsFormatToken(token))
    {
      if (token == "cpp-stream")
      {
        if (benchCase.Format != FormatType::Cpp)
          return false;
      }
      else if (token != FormatName(benchCase.Format))
      {
        return false;
      }
      continue;
    }

    if (IsMeasureToken(token))
    {
      if (token != MeasureName(benchCase.Measure))
        return false;
      continue;
    }

    return false;
  }

  return true;
}
} // namespace bench
