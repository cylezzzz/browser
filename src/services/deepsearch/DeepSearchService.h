\
/* src/services/deepsearch/DeepSearchService.h */
#pragma once
#include <QObject>
#include <QString>
#include <vector>
#include <nlohmann/json.hpp>

#include "core/storage/SqliteDb.h"
#include "core/entities/EntityDetector.h"

namespace services::deepsearch {

struct EntityProfile {
  std::string entityId;
  std::string name;
  std::string type;
  double confidence = 0.0;
  std::vector<std::string> sources;
  std::vector<std::pair<std::string, std::string>> related; // (name, relation)
};

class DeepSearchService : public QObject {
  Q_OBJECT
public:
  explicit DeepSearchService(core::storage::SqliteDb* db, QObject* parent = nullptr);

  EntityProfile buildProfileFromText(const QString& seedName, const QString& url, const QString& extractedText);
  std::vector<EntityProfile> searchProfiles(const QString& queryLike);

  bool upsertEntity(const EntityProfile& p);
  nlohmann::json profileToJson(const EntityProfile& p) const;

private:
  core::storage::SqliteDb* db_;
  core::entities::EntityDetector detector_;

  static std::string makeId(const std::string& name, const std::string& type);
};

} // namespace services::deepsearch
