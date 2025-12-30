\
/* src/core/net/RateLimiter.h */
#pragma once
#include <chrono>
#include <mutex>

namespace core::net {

class RateLimiter {
public:
  explicit RateLimiter(double permitsPerSecond = 1.0);

  void setRate(double permitsPerSecond);
  void acquire();

private:
  std::mutex mu_;
  double permitsPerSecond_;
  std::chrono::steady_clock::time_point next_;
};

} // namespace core::net
