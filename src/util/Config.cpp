\
/* src/util/Config.cpp */
#include "util/Config.h"
#include <fstream>

namespace util {

bool Config::loadFromFile(const std::string& path) {
  std::ifstream in(path);
  if (!in) return false;
  in >> j_;
  return true;
}

bool Config::saveToFile(const std::string& path) const {
  std::ofstream out(path);
  if (!out) return false;
  out << j_.dump(2);
  return true;
}

static std::string getStr(const nlohmann::json& j, const std::vector<std::string>& keys, const std::string& def) {
  const nlohmann::json* cur = &j;
  for (const auto& k : keys) {
    if (!cur->contains(k)) return def;
    cur = &((*cur)[k]);
  }
  if (cur->is_string()) return cur->get<std::string>();
  return def;
}

static int getInt(const nlohmann::json& j, const std::vector<std::string>& keys, int def) {
  const nlohmann::json* cur = &j;
  for (const auto& k : keys) {
    if (!cur->contains(k)) return def;
    cur = &((*cur)[k]);
  }
  if (cur->is_number_integer()) return cur->get<int>();
  return def;
}

static double getDouble(const nlohmann::json& j, const std::vector<std::string>& keys, double def) {
  const nlohmann::json* cur = &j;
  for (const auto& k : keys) {
    if (!cur->contains(k)) return def;
    cur = &((*cur)[k]);
  }
  if (cur->is_number()) return cur->get<double>();
  return def;
}

static bool getBool(const nlohmann::json& j, const std::vector<std::string>& keys, bool def) {
  const nlohmann::json* cur = &j;
  for (const auto& k : keys) {
    if (!cur->contains(k)) return def;
    cur = &((*cur)[k]);
  }
  if (cur->is_boolean()) return cur->get<bool>();
  return def;
}

std::string Config::ollamaHost() const { return getStr(j_, {"ollama","host"}, "http://127.0.0.1:11434"); }
std::string Config::ollamaModel() const { return getStr(j_, {"ollama","model"}, "llama3.2"); }

int Config::fetchTimeoutMs() const { return getInt(j_, {"fetch","timeout_ms"}, 12000); }
int Config::fetchRetries() const { return getInt(j_, {"fetch","retries"}, 2); }
double Config::rateLimitPerSec() const { return getDouble(j_, {"fetch","rate_limit_per_sec"}, 1.5); }
int Config::cacheMb() const { return getInt(j_, {"fetch","cache_mb"}, 64); }

std::string Config::searchProvider() const { return getStr(j_, {"search","provider"}, "ddg_html"); }
bool Config::searchSafe() const { return getBool(j_, {"search","safe"}, true); }

bool Config::stripTrackingParams() const { return getBool(j_, {"privacy","strip_tracking_params"}, true); }
bool Config::respectRobotsTxt() const { return getBool(j_, {"privacy","respect_robots_txt"}, false); }

} // namespace util
