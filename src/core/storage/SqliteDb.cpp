\
/* src/core/storage/SqliteDb.cpp */
#include "core/storage/SqliteDb.h"
#include "util/Log.h"
#include <sstream>

namespace core::storage {

SqliteDb::SqliteDb() : db_(nullptr) {}
SqliteDb::~SqliteDb() { close(); }

bool SqliteDb::open(const std::string& path) {
  std::lock_guard<std::mutex> lk(mu_);
  if (db_) return true;

  int rc = sqlite3_open_v2(path.c_str(), &db_,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                           nullptr);
  if (rc != SQLITE_OK) {
    util::Log::error("SQLite open failed: " + std::string(sqlite3_errstr(rc)));
    return false;
  }

  // Pragmas: reasonable defaults for desktop app
  exec("PRAGMA journal_mode=WAL;");
  exec("PRAGMA synchronous=NORMAL;");
  exec("PRAGMA foreign_keys=ON;");
  exec("PRAGMA temp_store=MEMORY;");
  exec("PRAGMA cache_size=-20000;"); // ~20MB pages

  return true;
}

void SqliteDb::close() {
  std::lock_guard<std::mutex> lk(mu_);
  if (!db_) return;
  sqlite3_close(db_);
  db_ = nullptr;
}

bool SqliteDb::exec(const std::string& sql) {
  std::lock_guard<std::mutex> lk(mu_);
  if (!db_) return false;
  char* err = nullptr;
  int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
  if (rc != SQLITE_OK) {
    std::string msg = err ? err : "unknown sqlite error";
    sqlite3_free(err);
    util::Log::error("SQLite exec failed: " + msg + " SQL=" + sql);
    return false;
  }
  return true;
}

bool SqliteDb::execParams(const std::string& sql, const std::vector<std::string>& params) {
  std::lock_guard<std::mutex> lk(mu_);
  if (!db_) return false;

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    util::Log::error("SQLite prepare failed: " + lastError());
    return false;
  }

  for (int i = 0; i < (int)params.size(); ++i) {
    sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
  }

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  if (rc != SQLITE_DONE) {
    util::Log::error("SQLite step failed: " + lastError());
    return false;
  }
  return true;
}

bool SqliteDb::query(const std::string& sql, const std::vector<std::string>& params,
                     const std::function<void(int, char**, char**)>& rowCb) {
  std::lock_guard<std::mutex> lk(mu_);
  if (!db_) return false;

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    util::Log::error("SQLite prepare failed: " + lastError());
    return false;
  }

  for (int i = 0; i < (int)params.size(); ++i) {
    sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
  }

  int colCount = sqlite3_column_count(stmt);
  std::vector<char*> values(colCount, nullptr);
  std::vector<char*> names(colCount, nullptr);

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    for (int c = 0; c < colCount; ++c) {
      names[c] = const_cast<char*>(sqlite3_column_name(stmt, c));
      const unsigned char* txt = sqlite3_column_text(stmt, c);
      values[c] = const_cast<char*>(reinterpret_cast<const char*>(txt ? txt : reinterpret_cast<const unsigned char*>("")));
    }
    rowCb(colCount, values.data(), names.data());
  }

  sqlite3_finalize(stmt);
  if (rc != SQLITE_DONE) {
    util::Log::error("SQLite query failed: " + lastError());
    return false;
  }
  return true;
}

std::string SqliteDb::lastError() const {
  if (!db_) return "db not open";
  return sqlite3_errmsg(db_);
}

} // namespace core::storage
