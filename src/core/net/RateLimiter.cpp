\
/* src/core/net/RateLimiter.cpp */
#include "core/net/RateLimiter.h"
#include <thread>

namespace core::net {

RateLimiter::RateLimiter(double permitsPerSecond)
  : permitsPerSecond_(permitsPerSecond),
    next_(std::chrono::steady_clock::now()) {}

void RateLimiter::setRate(double permitsPerSecond) {
  std::lock_guard<std::mutex> lk(mu_);
  permitsPerSecond_ = permitsPerSecond > 0.0 ? permitsPerSecond : 1.0;
}

void RateLimiter::acquire() {
  std::unique_lock<std::mutex> lk(mu_);
  const double pps = permitsPerSecond_ > 0.0 ? permitsPerSecond_ : 1.0;
  const auto interval = std::chrono::duration<double>(1.0 / pps);
  auto now = std::chrono::steady_clock::now();
  if (now < next_) {
    auto wait = next_ - now;
    lk.unlock();
    std::this_thread::sleep_for(wait);
    lk.lock();
  }
  next_ = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(interval);
}

} // namespace core::net
