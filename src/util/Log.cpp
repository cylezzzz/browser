\
/* src/util/Log.cpp */
#include "util/Log.h"
#include "util/Time.h"
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>

namespace fs = std::filesystem;

namespace util {

static std::mutex g_mu;
static fs::path g_dir;
static fs::path g_file;
static std::size_t g_maxFiles = 5;
static std::size_t g_maxBytes = 1024 * 1024;

static const char* levelStr(LogLevel l) {
  switch (l) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO";
    case LogLevel::Warn:  return "WARN";
    case LogLevel::Error: return "ERROR";
  }
  return "INFO";
}

void Log::init(const std::string& directory, std::size_t maxFiles, std::size_t maxBytes) {
  std::lock_guard<std::mutex> lk(g_mu);
  g_dir = fs::path(directory);
  g_maxFiles = maxFiles;
  g_maxBytes = maxBytes;
  fs::create_directories(g_dir);
  g_file = g_dir / "novabrowse.log";
}

void Log::rotateIfNeeded() {
  if (g_file.empty()) return;
  std::error_code ec;
  auto size = fs::file_size(g_file, ec);
  if (ec) return;
  if (size < g_maxBytes) return;

  for (std::size_t i = g_maxFiles; i > 0; --i) {
    fs::path src = (i == 1) ? g_file : (g_dir / ("novabrowse.log." + std::to_string(i - 1)));
    fs::path dst = g_dir / ("novabrowse.log." + std::to_string(i));
    if (fs::exists(src)) {
      std::error_code ec2;
      fs::rename(src, dst, ec2);
      if (ec2) {
        fs::remove(dst, ec2);
        fs::rename(src, dst, ec2);
      }
    }
  }
}

void Log::write(LogLevel level, const std::string& msg) {
  std::lock_guard<std::mutex> lk(g_mu);
  if (g_file.empty()) return;
  rotateIfNeeded();
  std::ofstream out(g_file, std::ios::app);
  if (!out) return;
  out << iso_utc(now_ms()) << " [" << levelStr(level) << "] " << msg << "\n";
}

void Log::debug(const std::string& msg) { write(LogLevel::Debug, msg); }
void Log::info(const std::string& msg)  { write(LogLevel::Info,  msg); }
void Log::warn(const std::string& msg)  { write(LogLevel::Warn,  msg); }
void Log::error(const std::string& msg) { write(LogLevel::Error, msg); }

} // namespace util
