\
/* src/core/storage/SqliteDb.h */
#pragma once
#include <sqlite3.h>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace core::storage {

class SqliteDb {
public:
  SqliteDb();
  ~SqliteDb();

  SqliteDb(const SqliteDb&) = delete;
  SqliteDb& operator=(const SqliteDb&) = delete;

  bool open(const std::string& path);
  void close();

  bool exec(const std::string& sql);
  bool execParams(const std::string& sql, const std::vector<std::string>& params);

  bool query(const std::string& sql, const std::vector<std::string>& params,
             const std::function<void(int, char** , char**)>& rowCb);

  sqlite3* handle() const { return db_; }

  std::string lastError() const;

private:
  sqlite3* db_;
  mutable std::mutex mu_;
};

} // namespace core::storage
