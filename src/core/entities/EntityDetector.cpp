\
/* src/core/entities/EntityDetector.cpp */
#include "core/entities/EntityDetector.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <unordered_map>

namespace core::entities {

static std::string trim(const std::string& s) {
  auto b = s.begin();
  while (b != s.end() && std::isspace((unsigned char)*b)) ++b;
  auto e = s.end();
  while (e != b && std::isspace((unsigned char)*(e - 1))) --e;
  return std::string(b, e);
}

static bool isStopword(const std::string& w) {
  static const std::unordered_map<std::string, bool> stop = {
    {"The",true},{"A",true},{"An",true},{"And",true},{"Or",true},{"But",true},
    {"This",true},{"That",true},{"These",true},{"Those",true},{"In",true},{"On",true},
    {"At",true},{"For",true},{"With",true},{"From",true},{"By",true},{"As",true},
    {"It",true},{"Its",true},{"He",true},{"She",true},{"They",true},{"We",true},{"I",true}
  };
  return stop.find(w) != stop.end();
}

bool EntityDetector::looksLikePerson(const std::string& name) {
  // Two capitalized tokens
  std::regex re(R"(^[A-Z][a-zA-Z\-']+\s+[A-Z][a-zA-Z\-']+(\s+[A-Z][a-zA-Z\-']+)?$)");
  return std::regex_match(name, re);
}

bool EntityDetector::looksLikeOrg(const std::string& name) {
  static const std::vector<std::string> hints = {"Inc","Corp","LLC","Ltd","GmbH","AG","University","Institute","Company","Bank","Group"};
  for (const auto& h : hints) {
    if (name.find(h) != std::string::npos) return true;
  }
  return false;
}

bool EntityDetector::looksLikePlace(const std::string& name) {
  static const std::vector<std::string> hints = {"City","State","Province","County","Berlin","London","Paris","New York","Munich","Hamburg"};
  for (const auto& h : hints) {
    if (name.find(h) != std::string::npos) return true;
  }
  return false;
}

std::vector<EntityMention> EntityDetector::detect(const std::string& text) {
  // Heuristic: sequences of capitalized words, limited length
  std::regex re(R"(\b([A-Z][a-zA-Z\-']+)(\s+[A-Z][a-zA-Z\-']+){0,3}\b)");
  std::sregex_iterator it(text.begin(), text.end(), re);
  std::sregex_iterator end;

  std::unordered_map<std::string, int> freq;
  for (; it != end; ++it) {
    std::string m = trim(it->str());
    if (m.size() < 4) continue;
    if (isStopword(m)) continue;
    // skip sentence starts like "However"
    if (m.find(' ') == std::string::npos && m.size() < 7) continue;
    freq[m]++;
  }

  std::vector<EntityMention> out;
  out.reserve(freq.size());
  for (const auto& kv : freq) {
    EntityMention em;
    em.name = kv.first;
    em.type = EntityType::Unknown;
    em.confidence = std::min(0.95, 0.35 + kv.second * 0.08);

    if (looksLikeOrg(em.name)) { em.type = EntityType::Org; em.confidence = std::min(0.98, em.confidence + 0.15); }
    else if (looksLikePlace(em.name)) { em.type = EntityType::Place; em.confidence = std::min(0.98, em.confidence + 0.10); }
    else if (looksLikePerson(em.name)) { em.type = EntityType::Person; em.confidence = std::min(0.98, em.confidence + 0.20); }

    out.push_back(em);
  }

  std::sort(out.begin(), out.end(), [](const EntityMention& a, const EntityMention& b) {
    return a.confidence > b.confidence;
  });

  if (out.size() > 30) out.resize(30);
  return out;
}

std::string toString(EntityType t) {
  switch (t) {
    case EntityType::Person: return "person";
    case EntityType::Org: return "org";
    case EntityType::Place: return "place";
    case EntityType::Unknown: return "unknown";
  }
  return "unknown";
}

} // namespace core::entities
