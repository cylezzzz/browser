\
/* src/services/ai/RagComposer.h */
#pragma once
#include <string>
#include <vector>

namespace services::ai {

struct Citation {
  std::string url;
  std::string blockId;
};

struct RagContext {
  std::string contextText;
  std::vector<Citation> citations;
};

class RagComposer {
public:
  // Build a bounded context from extracted blocks. "budgetChars" is a practical stand-in for tokens.
  static RagContext fromBlocks(const std::string& url,
                               const std::vector<std::pair<std::string, std::string>>& blocks,
                               std::size_t budgetChars = 6000);
};

} // namespace services::ai
