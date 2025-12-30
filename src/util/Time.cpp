\
/* src/util/Time.cpp */
#include "util/Time.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace util {

std::int64_t now_ms() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::string iso_utc(std::int64_t ms) {
  std::time_t t = static_cast<std::time_t>(ms / 1000);
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &t);
#else
  gmtime_r(&t, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

} // namespace util
