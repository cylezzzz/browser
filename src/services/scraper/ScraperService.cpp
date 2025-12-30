\
/* src/services/scraper/ScraperService.cpp */
#include "services/scraper/ScraperService.h"
#include <sstream>

namespace services::scraper {

static std::string escJson(const std::string& s) {
  std::string o;
  o.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
      case '\\': o += "\\\\"; break;
      case '"': o += "\\\""; break;
      case '\n': o += "\\n"; break;
      case '\r': o += "\\r"; break;
      case '\t': o += "\\t"; break;
      default: o.push_back(c); break;
    }
  }
  return o;
}

ScrapeOutput ScraperService::run(ScrapeMode mode, const QUrl& url, const QString& html) {
  ScrapeOutput out;
  core::extract::ExtractedPage ep = extractor_.extract(html.toStdString(), url.toString().toStdString());

  switch (mode) {
    case ScrapeMode::Reader:
      out.mime = "text/markdown";
      out.text = toMarkdownReader(ep);
      return out;
    case ScrapeMode::FullText:
      out.mime = "text/plain";
      out.text = QString::fromStdString(ep.fullText);
      return out;
    case ScrapeMode::Metadata: {
      out.mime = "application/json";
      nlohmann::json j;
      j["title"] = ep.title;
      j["description"] = ep.description;
      j["canonical_url"] = ep.canonicalUrl;
      out.text = QString::fromStdString(j.dump(2));
      return out;
    }
    case ScrapeMode::Links: {
      out.mime = "text/plain";
      QString t;
      for (const auto& l : ep.links) t += QString::fromStdString(l) + "\n";
      out.text = t;
      return out;
    }
    case ScrapeMode::Headings: {
      out.mime = "text/plain";
      QString t;
      for (const auto& h : ep.headings) t += QString::fromStdString(h) + "\n";
      out.text = t.isEmpty() ? "(Headings collection is minimal in MVP)\n" : t;
      return out;
    }
    case ScrapeMode::BlocksJson:
      out.mime = "application/json";
      out.text = toJsonBlocks(ep);
      return out;
    case ScrapeMode::TablesStub:
      out.mime = "application/json";
      out.text = R"({"tables":[],"note":"Tables extraction stub in MVP. Use DOM selector mode or extend extractor."})";
      return out;
    case ScrapeMode::JsonLdStub:
      out.mime = "application/json";
      out.text = R"({"jsonld":[],"note":"JSON-LD extraction stub in MVP. Extend extractor for <script type='application/ld+json'>."})";
      return out;
  }

  out.mime = "text/plain";
  out.text = "Unknown mode";
  return out;
}

QString ScraperService::toMarkdownReader(const core::extract::ExtractedPage& ep) {
  QString md;
  md += "# " + QString::fromStdString(ep.title.empty() ? "Untitled" : ep.title) + "\n\n";
  if (!ep.description.empty()) md += "> " + QString::fromStdString(ep.description) + "\n\n";
  for (const auto& b : ep.blocks) {
    md += "## " + QString::fromStdString(b.id) + "\n\n";
    md += QString::fromStdString(b.text) + "\n\n";
  }
  return md;
}

QString ScraperService::toJsonBlocks(const core::extract::ExtractedPage& ep) {
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"title\": \"" << escJson(ep.title) << "\",\n";
  oss << "  \"canonical_url\": \"" << escJson(ep.canonicalUrl) << "\",\n";
  oss << "  \"blocks\": [\n";
  for (size_t i = 0; i < ep.blocks.size(); ++i) {
    const auto& b = ep.blocks[i];
    oss << "    {\"id\": \"" << escJson(b.id) << "\", \"text\": \"" << escJson(b.text) << "\"}";
    if (i + 1 != ep.blocks.size()) oss << ",";
    oss << "\n";
  }
  oss << "  ]\n";
  oss << "}\n";
  return QString::fromStdString(oss.str());
}

} // namespace services::scraper
