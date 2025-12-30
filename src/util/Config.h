\
/* src/util/Config.h */
#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace util {

class Config {
public:
  bool loadFromFile(const std::string& path);
  bool saveToFile(const std::string& path) const;

  std::string ollamaHost() const;
  std::string ollamaModel() const;

  int fetchTimeoutMs() const;
  int fetchRetries() const;
  double rateLimitPerSec() const;
  int cacheMb() const;

  std::string searchProvider() const;
  bool searchSafe() const;

  bool stripTrackingParams() const;
  bool respectRobotsTxt() const;

  const nlohmann::json& raw() const { return j_; }
  nlohmann::json& raw() { return j_; }

private:
  nlohmann::json j_;
};

} // namespace util
