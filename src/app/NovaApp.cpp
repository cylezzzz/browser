\
/* src/app/NovaApp.cpp */
#include "app/NovaApp.h"
#include "ui/MainWindow.h"
#include "core/storage/Migrations.h"
#include "util/Log.h"
#include <QStandardPaths>
#include <QDir>
#include <QtWebEngineCore/QtWebEngineCore>

NovaApp::NovaApp(int& argc, char** argv)
  : app_(argc, argv) {
  app_.setApplicationName("NovaBrowse");
  app_.setOrganizationName("NovaBrowse");
  app_.setApplicationVersion("0.1.0");
}

QString NovaApp::dataDir() const {
  QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (dir.isEmpty()) dir = QDir::homePath() + "/.novabrowse";
  QDir().mkpath(dir);
  return dir;
}

bool NovaApp::initConfig() {
  QString dir = dataDir();
  QString cfg = dir + "/config.json";

  // Prefer local "data/config.json" next to exe, else AppData.
  QString exeCfg = QCoreApplication::applicationDirPath() + "/data/config.json";
  if (QFileInfo::exists(exeCfg)) cfg = exeCfg;

  if (!config_.loadFromFile(cfg.toStdString())) {
    // fallback: try bundled config in working dir
    QString bundled = QDir::currentPath() + "/config/config.json";
    if (!config_.loadFromFile(bundled.toStdString())) return false;
    config_.saveToFile(cfg.toStdString());
  }

  util::Log::init((dir + "/logs").toStdString());
  util::Log::info("Config loaded from: " + cfg.toStdString());
  return true;
}

bool NovaApp::initStorage() {
  QString dir = dataDir();
  QString dbPath = dir + "/novabrowse.sqlite3";
  if (!db_.open(dbPath.toStdString())) return false;
  if (!core::storage::runMigrations(db_)) return false;
  util::Log::info("DB opened: " + dbPath.toStdString());
  return true;
}

void NovaApp::initServices() {
  fetcher_.setTimeoutMs(config_.fetchTimeoutMs());
  fetcher_.setRetries(config_.fetchRetries());
  fetcher_.setStripTracking(config_.stripTrackingParams());
  fetcher_.setRate(config_.rateLimitPerSec());

  search_ = std::make_unique<services::search::DdgHtmlSearch>(&fetcher_);
  ollama_ = std::make_unique<services::ai::OllamaClient>(&fetcher_);
  ollama_->setHost(QString::fromStdString(config_.ollamaHost()));
  deepsearch_ = std::make_unique<services::deepsearch::DeepSearchService>(&db_);
}

int NovaApp::run() {
  QtWebEngineCore::initialize();

  if (!initConfig()) return 1;
  if (!initStorage()) return 2;
  initServices();

  mainWindow_ = std::make_unique<ui::MainWindow>(this);
  mainWindow_->show();

  return app_.exec();
}
