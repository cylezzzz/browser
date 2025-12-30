\
/* src/services/search/DdgHtmlSearch.h */
#pragma once
#include <QObject>
#include <QUrl>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "core/net/FetchService.h"

namespace services::search {

struct SearchResultItem {
  std::string title;
  std::string url;
  std::string snippet;
};

struct SearchResponse {
  std::string query;
  std::string provider;
  std::vector<SearchResultItem> items;
};

class DdgHtmlSearch : public QObject {
  Q_OBJECT
public:
  explicit DdgHtmlSearch(core::net::FetchService* fetcher, QObject* parent = nullptr);

  void searchWeb(const QString& query, const std::function<void(const SearchResponse&)>& cb);

  static std::string toHtmlResultsPage(const SearchResponse& r, const std::string& type = "web");
  static nlohmann::json toJson(const SearchResponse& r);

private:
  core::net::FetchService* fetcher_;
  SearchResponse parseHtml(const std::string& query, const QByteArray& html);
};

} // namespace services::search
