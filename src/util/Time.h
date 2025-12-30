\
/* src/util/Time.h */
#pragma once
#include <cstdint>
#include <string>

namespace util {
  std::int64_t now_ms();
  std::string iso_utc(std::int64_t ms);
}
