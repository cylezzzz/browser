\
/* src/core/entities/EntityDetector.h */
#pragma once
#include <string>
#include <vector>

namespace core::entities {

enum class EntityType { Person, Org, Place, Unknown };

struct EntityMention {
  std::string name;
  EntityType type;
  double confidence;
};

class EntityDetector {
public:
  std::vector<EntityMention> detect(const std::string& text);

private:
  static bool looksLikePerson(const std::string& name);
  static bool looksLikeOrg(const std::string& name);
  static bool looksLikePlace(const std::string& name);
};

std::string toString(EntityType t);

} // namespace core::entities
