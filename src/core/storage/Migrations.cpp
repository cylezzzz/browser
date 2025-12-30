\
/* src/core/storage/Migrations.cpp */
#include "core/storage/Migrations.h"
#include "util/Log.h"

namespace core::storage {

static bool ensureMeta(SqliteDb& db) {
  return db.exec(R"SQL(
    CREATE TABLE IF NOT EXISTS meta(
      key TEXT PRIMARY KEY,
      value TEXT NOT NULL
    );
  )SQL");
}

static int currentVersion(SqliteDb& db) {
  int v = 0;
  db.query("SELECT value FROM meta WHERE key='schema_version';", {}, [&](int, char** vals, char**) {
    if (vals && vals[0]) v = std::atoi(vals[0]);
  });
  return v;
}

static bool setVersion(SqliteDb& db, int v) {
  return db.execParams("INSERT INTO meta(key,value) VALUES('schema_version', ?) "
                       "ON CONFLICT(key) DO UPDATE SET value=excluded.value;",
                       {std::to_string(v)});
}

bool runMigrations(SqliteDb& db) {
  if (!ensureMeta(db)) return false;

  int v = currentVersion(db);
  util::Log::info("DB schema_version=" + std::to_string(v));

  if (v < 1) {
    // Core tables
    bool ok = true;

    ok &= db.exec(R"SQL(
      CREATE TABLE IF NOT EXISTS history(
        url TEXT NOT NULL,
        title TEXT,
        ts INTEGER NOT NULL
      );
      CREATE INDEX IF NOT EXISTS idx_history_ts ON history(ts DESC);

      CREATE TABLE IF NOT EXISTS bookmarks(
        url TEXT PRIMARY KEY,
        title TEXT,
        folder TEXT,
        ts INTEGER NOT NULL
      );

      CREATE TABLE IF NOT EXISTS downloads(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        path TEXT,
        url TEXT,
        ts INTEGER NOT NULL,
        status TEXT
      );

      CREATE TABLE IF NOT EXISTS settings(
        key TEXT PRIMARY KEY,
        value TEXT
      );

      CREATE TABLE IF NOT EXISTS search_cache(
        query TEXT NOT NULL,
        provider TEXT NOT NULL,
        ts INTEGER NOT NULL,
        json TEXT NOT NULL,
        PRIMARY KEY(query, provider)
      );

      CREATE TABLE IF NOT EXISTS local_docs(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        url_or_path TEXT,
        title TEXT,
        ts INTEGER NOT NULL,
        content TEXT,
        meta TEXT,
        jsonld TEXT,
        links TEXT,
        tables TEXT
      );
    )SQL");

    // FTS5 virtual table (content index)
    ok &= db.exec(R"SQL(
      CREATE VIRTUAL TABLE IF NOT EXISTS local_docs_fts USING fts5(
        title, content, url_or_path, tokenize = 'porter'
      );
    )SQL");

    // Entities
    ok &= db.exec(R"SQL(
      CREATE TABLE IF NOT EXISTS entity_index(
        entity_id TEXT PRIMARY KEY,
        name TEXT NOT NULL,
        type TEXT NOT NULL,
        aliases TEXT,
        ts INTEGER NOT NULL
      );

      CREATE TABLE IF NOT EXISTS entity_links(
        entity_id TEXT NOT NULL,
        doc_id INTEGER NOT NULL,
        confidence REAL NOT NULL,
        mentions_json TEXT,
        PRIMARY KEY(entity_id, doc_id),
        FOREIGN KEY(doc_id) REFERENCES local_docs(id) ON DELETE CASCADE
      );
    )SQL");

    if (!ok) return false;
    if (!setVersion(db, 1)) return false;
  }

  return true;
}

} // namespace core::storage
