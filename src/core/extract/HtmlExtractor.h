\
/* src/core/extract/HtmlExtractor.h */
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace core::extract {

struct Block {
  std::string id;
  std::string text;
};

struct ExtractedPage {
  std::string title;
  std::string description;
  std::string canonicalUrl;
  std::vector<std::string> headings;
  std::vector<std::string> links;
  std::vector<Block> blocks;
  std::string fullText;
};

class HtmlExtractor {
public:
  ExtractedPage extract(const std::string& html, const std::string& baseUrl);

private:
  static std::string stripWhitespace(const std::string& s);
};

} // namespace core::extract
