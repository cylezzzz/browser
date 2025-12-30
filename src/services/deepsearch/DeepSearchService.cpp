\
/* src/services/deepsearch/DeepSearchService.cpp */
#include "services/deepsearch/DeepSearchService.h"
#include "util/Time.h"
#include <algorithm>
#include <QCryptographicHash>

namespace services::deepsearch {

std::string DeepSearchService::makeId(const std::string& name, const std::string& type) {
  const QByteArray s = QByteArray::fromStdString(type + ":" + name);
  const QByteArray h = QCryptographicHash::hash(s, QCryptographicHash::Sha1).toHex();
  return h.toStdString();
}

DeepSearchService::DeepSearchService(core::storage::SqliteDb* db, QObject* parent)
  : QObject(parent), db_(db) {}

EntityProfile DeepSearchService::buildProfileFromText(const QString& seedName, const QString& url, const QString& extractedText) {
  EntityProfile p;
  p.name = seedName.toStdString();
  p.type = "person";
  p.entityId = makeId(p.name, p.type);

  auto mentions = detector_.detect(extractedText.toStdString());
  p.confidence = 0.65;
  p.sources.push_back(url.toStdString());

  int added = 0;
  for (const auto& m : mentions) {
    if (m.name == p.name) continue;
    if (m.confidence < 0.55) continue;
    std::string rel = (m.type == core::entities::EntityType::Org) ? "affiliated" :
                      (m.type == core::entities::EntityType::Person) ? "related_person" :
                      (m.type == core::entities::EntityType::Place) ? "location" : "related";
    p.related.push_back({m.name, rel});
    if (++added >= 10) break;
  }
  return p;
}

bool DeepSearchService::upsertEntity(const EntityProfile& p) {
  if (!db_) return false;
  const std::string aliases = "[]";
  const std::int64_t ts = util::now_ms();
  bool ok = db_->execParams(
    "INSERT INTO entity_index(entity_id,name,type,aliases,ts) VALUES(?,?,?,?,?) "
    "ON CONFLICT(entity_id) DO UPDATE SET name=excluded.name, type=excluded.type, aliases=excluded.aliases, ts=excluded.ts;",
    {p.entityId, p.name, p.type, aliases, std::to_string(ts)}
  );
  return ok;
}

std::vector<EntityProfile> DeepSearchService::searchProfiles(const QString& queryLike) {
  std::vector<EntityProfile> out;
  if (!db_) return out;

  std::string like = "%" + queryLike.toStdString() + "%";
  db_->query("SELECT entity_id,name,type,ts FROM entity_index WHERE name LIKE ? ORDER BY ts DESC LIMIT 25;",
             {like},
             [&](int, char** vals, char**) {
               EntityProfile p;
               p.entityId = vals[0] ? vals[0] : "";
               p.name = vals[1] ? vals[1] : "";
               p.type = vals[2] ? vals[2] : "unknown";
               p.confidence = 0.60;
               out.push_back(p);
             });
  return out;
}

nlohmann::json DeepSearchService::profileToJson(const EntityProfile& p) const {
  nlohmann::json j;
  j["entity_id"] = p.entityId;
  j["name"] = p.name;
  j["type"] = p.type;
  j["confidence"] = p.confidence;
  j["sources"] = p.sources;
  j["related"] = nlohmann::json::array();
  for (const auto& r : p.related) j["related"].push_back({{"name", r.first}, {"relation", r.second}});
  return j;
}

} // namespace services::deepsearch
