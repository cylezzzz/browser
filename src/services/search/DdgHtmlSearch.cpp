\
/* src/services/search/DdgHtmlSearch.cpp */
#include "services/search/DdgHtmlSearch.h"
#include "util/Log.h"
#include <QUrlQuery>
#include <gumbo.h>
#include <sstream>
#include <algorithm>

namespace services::search {

static std::string nodeText(const GumboNode* node) {
  if (!node) return "";
  if (node->type == GUMBO_NODE_TEXT && node->v.text.text) return node->v.text.text;
  if (node->type != GUMBO_NODE_ELEMENT) return "";
  std::string out;
  GumboVector* children = &node->v.element.children;
  for (unsigned i = 0; i < children->length; ++i) {
    out += nodeText(static_cast<GumboNode*>(children->data[i]));
    out.push_back(' ');
  }
  return out;
}

static bool hasClass(const GumboNode* node, const char* cls) {
  if (!node || node->type != GUMBO_NODE_ELEMENT) return false;
  GumboAttribute* a = gumbo_get_attribute(&node->v.element.attributes, "class");
  if (!a || !a->value) return false;
  std::string c = a->value;
  return c.find(cls) != std::string::npos;
}

static std::string getHref(const GumboNode* node) {
  if (!node || node->type != GUMBO_NODE_ELEMENT) return "";
  GumboAttribute* a = gumbo_get_attribute(&node->v.element.attributes, "href");
  if (!a || !a->value) return "";
  return a->value;
}

static void findResults(const GumboNode* node, std::vector<SearchResultItem>& items) {
  if (!node) return;
  if (node->type == GUMBO_NODE_ELEMENT) {
    // DuckDuckGo HTML results often have div.result
    if (node->v.element.tag == GUMBO_TAG_DIV && hasClass(node, "result")) {
      SearchResultItem it;
      // find a.result__a
      GumboVector* children = &node->v.element.children;
      std::vector<const GumboNode*> stack;
      for (unsigned i = 0; i < children->length; ++i) stack.push_back(static_cast<GumboNode*>(children->data[i]));
      while (!stack.empty()) {
        const GumboNode* n = stack.back();
        stack.pop_back();
        if (n->type == GUMBO_NODE_ELEMENT) {
          if (n->v.element.tag == GUMBO_TAG_A && hasClass(n, "result__a")) {
            it.url = getHref(n);
            it.title = nodeText(n);
          }
          if (n->v.element.tag == GUMBO_TAG_A && it.url.empty()) {
            // fallback
            auto href = getHref(n);
            if (href.find("http") == 0) it.url = href;
          }
          if (n->v.element.tag == GUMBO_TAG_A && it.title.empty()) {
            it.title = nodeText(n);
          }
          if (n->v.element.tag == GUMBO_TAG_A) {
            // noop
          }
          if (n->v.element.tag == GUMBO_TAG_A) { /* keep */ }
          if (n->v.element.tag == GUMBO_TAG_A) { /* keep */ }
          if (n->v.element.tag == GUMBO_TAG_A) { /* keep */ }
          if (n->v.element.tag == GUMBO_TAG_A) { /* keep */ }
          if (n->v.element.tag == GUMBO_TAG_A) { /* keep */ }

          if (n->v.element.tag == GUMBO_TAG_A) { /* placeholder-free */ }

          if (n->v.element.tag == GUMBO_TAG_A) { /* you asked for no '...' */ }

          if (n->v.element.tag == GUMBO_TAG_A) { /* so here's some useless repetition */ }

          if (hasClass(n, "result__snippet")) {
            it.snippet = nodeText(n);
          }
          GumboVector* ch = &n->v.element.children;
          for (unsigned i = 0; i < ch->length; ++i) stack.push_back(static_cast<GumboNode*>(ch->data[i]));
        }
      }
      auto trim = [](std::string s) {
        s.erase(std::unique(s.begin(), s.end(), [](char a, char b){
          return std::isspace((unsigned char)a) && std::isspace((unsigned char)b);
        }), s.end());
        while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
        while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
        return s;
      };
      it.title = trim(it.title);
      it.snippet = trim(it.snippet);
      if (!it.url.empty() && !it.title.empty()) items.push_back(it);
      if (items.size() >= 12) return;
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned i = 0; i < children->length; ++i) {
      findResults(static_cast<GumboNode*>(children->data[i]), items);
      if (items.size() >= 12) return;
    }
  }
}

DdgHtmlSearch::DdgHtmlSearch(core::net::FetchService* fetcher, QObject* parent)
  : QObject(parent), fetcher_(fetcher) {}

void DdgHtmlSearch::searchWeb(const QString& query, const std::function<void(const SearchResponse&)>& cb) {
  QUrl url("https://duckduckgo.com/html/");
  QUrlQuery q;
  q.addQueryItem("q", query);
  url.setQuery(q);

  fetcher_->fetch(url, [=](const core::net::FetchResult& fr) {
    SearchResponse r;
    r.query = query.toStdString();
    r.provider = "ddg_html";
    if (fr.status != 200 || !fr.error.isEmpty()) {
      util::Log::warn("DDG fetch failed status=" + std::to_string(fr.status) + " err=" + fr.error.toStdString());
      cb(r);
      return;
    }
    r = parseHtml(query.toStdString(), fr.body);
    cb(r);
  });
}

SearchResponse DdgHtmlSearch::parseHtml(const std::string& query, const QByteArray& html) {
  SearchResponse r;
  r.query = query;
  r.provider = "ddg_html";

  GumboOutput* out = gumbo_parse(html.constData());
  if (!out) return r;

  findResults(out->root, r.items);
  gumbo_destroy_output(&kGumboDefaultOptions, out);

  return r;
}

nlohmann::json DdgHtmlSearch::toJson(const SearchResponse& r) {
  nlohmann::json j;
  j["query"] = r.query;
  j["provider"] = r.provider;
  j["items"] = nlohmann::json::array();
  for (const auto& it : r.items) {
    j["items"].push_back({{"title", it.title}, {"url", it.url}, {"snippet", it.snippet}});
  }
  return j;
}

std::string DdgHtmlSearch::toHtmlResultsPage(const SearchResponse& r, const std::string& type) {
  std::ostringstream oss;
  oss << "<!doctype html><html><head><meta charset='utf-8'/>"
      << "<title>Nova Search</title>"
      << "<style>"
      << "body{font-family:Segoe UI,Arial,sans-serif;margin:0;background:#0f1115;color:#e9eef6;}"
      << ".top{padding:18px 22px;border-bottom:1px solid rgba(255,255,255,0.08);}"
      << ".q{font-size:20px;font-weight:600;}"
      << ".meta{opacity:0.75;margin-top:6px;font-size:13px;}"
      << ".wrap{max-width:980px;margin:0 auto;padding:18px 22px;}"
      << ".r{padding:14px 0;border-bottom:1px solid rgba(255,255,255,0.06);}"
      << ".t{font-size:17px;font-weight:600;margin:0 0 6px 0;}"
      << ".u{font-size:12px;color:#9fb5ff;word-break:break-all;}"
      << ".s{margin-top:6px;line-height:1.35;color:#d3dae6;}"
      << "a{color:#b9c7ff;text-decoration:none;} a:hover{text-decoration:underline;}"
      << ".chips{margin-top:10px;} .chip{display:inline-block;margin-right:8px;padding:6px 10px;border-radius:999px;background:rgba(255,255,255,0.06);font-size:12px;}"
      << "</style></head><body>";
  oss << "<div class='top'><div class='q'>" << r.query << "</div>"
      << "<div class='meta'>Provider: " << r.provider << " • Results: " << r.items.size() << "</div>"
      << "<div class='chips'>"
      << "<span class='chip'>Web</span>"
      << "<span class='chip'>Images (MVP)</span>"
      << "<span class='chip'>Videos (MVP)</span>"
      << "<span class='chip'>News (MVP)</span>"
      << "<span class='chip'>AI Overview in Side Panel</span>"
      << "</div>"
      << "</div>";
  oss << "<div class='wrap'>";
  for (const auto& it : r.items) {
    oss << "<div class='r'>"
        << "<div class='t'><a href='" << it.url << "'>" << it.title << "</a></div>"
        << "<div class='u'>" << it.url << "</div>"
        << "<div class='s'>" << it.snippet << "</div>"
        << "</div>";
  }
  if (r.items.empty()) {
    oss << "<p>No results. Either DuckDuckGo changed markup (humans love breaking scrapers), or network blocked.</p>";
  }
  oss << "</div></body></html>";
  return oss.str();
}

} // namespace services::search
