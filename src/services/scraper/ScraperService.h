\
/* src/services/scraper/ScraperService.h */
#pragma once
#include <QString>
#include <QUrl>
#include <nlohmann/json.hpp>
#include "core/extract/HtmlExtractor.h"

namespace services::scraper {

enum class ScrapeMode {
  Reader,
  FullText,
  Metadata,
  Links,
  Headings,
  BlocksJson,
  TablesStub,
  JsonLdStub
};

struct ScrapeOutput {
  QString mime;
  QString text;
};

class ScraperService {
public:
  ScrapeOutput run(ScrapeMode mode, const QUrl& url, const QString& html);

private:
  core::extract::HtmlExtractor extractor_;
  static QString toMarkdownReader(const core::extract::ExtractedPage& ep);
  static QString toJsonBlocks(const core::extract::ExtractedPage& ep);
};

} // namespace services::scraper
