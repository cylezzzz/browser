\
/* src/core/storage/Migrations.h */
#pragma once
#include "core/storage/SqliteDb.h"
#include <string>

namespace core::storage {
  bool runMigrations(SqliteDb& db);
}
