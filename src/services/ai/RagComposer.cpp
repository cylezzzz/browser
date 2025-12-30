\
/* src/services/ai/RagComposer.cpp */
#include "services/ai/RagComposer.h"
#include <sstream>

namespace services::ai {

RagContext RagComposer::fromBlocks(const std::string& url,
                                  const std::vector<std::pair<std::string, std::string>>& blocks,
                                  std::size_t budgetChars) {
  RagContext rc;
  std::ostringstream oss;
  std::size_t used = 0;
  for (const auto& b : blocks) {
    const std::string& id = b.first;
    const std::string& txt = b.second;
    if (txt.empty()) continue;
    std::string line = "[" + id + "] " + txt + "\n";
    if (used + line.size() > budgetChars) break;
    oss << line;
    used += line.size();
    rc.citations.push_back({url, id});
  }
  rc.contextText = oss.str();
  return rc;
}

} // namespace services::ai
