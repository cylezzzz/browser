\
/* src/app/NovaApp.h */
#pragma once
#include <QApplication>
#include <memory>

#include "util/Config.h"
#include "core/storage/SqliteDb.h"
#include "core/net/FetchService.h"
#include "services/search/DdgHtmlSearch.h"
#include "services/ai/OllamaClient.h"
#include "services/deepsearch/DeepSearchService.h"

namespace ui { class MainWindow; }

class NovaApp {
public:
  NovaApp(int& argc, char** argv);
  int run();

  util::Config& config() { return config_; }
  core::storage::SqliteDb& db() { return db_; }
  core::net::FetchService& fetcher() { return fetcher_; }
  services::search::DdgHtmlSearch& search() { return *search_; }
  services::ai::OllamaClient& ollama() { return *ollama_; }
  services::deepsearch::DeepSearchService& deepsearch() { return *deepsearch_; }

private:
  QApplication app_;
  util::Config config_;
  core::storage::SqliteDb db_;
  core::net::FetchService fetcher_;

  std::unique_ptr<services::search::DdgHtmlSearch> search_;
  std::unique_ptr<services::ai::OllamaClient> ollama_;
  std::unique_ptr<services::deepsearch::DeepSearchService> deepsearch_;
  std::unique_ptr<ui::MainWindow> mainWindow_;

  QString dataDir() const;
  bool initConfig();
  bool initStorage();
  void initServices();
};
