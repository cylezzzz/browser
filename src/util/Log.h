\
/* src/util/Log.h */
#pragma once
#include <string>

namespace util {

enum class LogLevel { Debug, Info, Warn, Error };

class Log {
public:
  static void init(const std::string& directory, std::size_t maxFiles = 5, std::size_t maxBytes = 1024 * 1024);
  static void write(LogLevel level, const std::string& msg);
  static void debug(const std::string& msg);
  static void info(const std::string& msg);
  static void warn(const std::string& msg);
  static void error(const std::string& msg);

private:
  static void rotateIfNeeded();
};

} // namespace util
