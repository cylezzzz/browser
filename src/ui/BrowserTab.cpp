\
/* src/ui/BrowserTab.cpp */
#include "ui/BrowserTab.h"
#include <QVBoxLayout>

namespace ui {

BrowserTab::BrowserTab(QWidget* parent)
  : QWidget(parent),
    view_(new QWebEngineView(this)),
    chat_(nlohmann::json::array()) {

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(view_);

  QObject::connect(view_, &QWebEngineView::titleChanged, this, &BrowserTab::titleChanged);
  QObject::connect(view_, &QWebEngineView::urlChanged, this, &BrowserTab::urlChanged);

  QObject::connect(view_, &QWebEngineView::loadStarted, this, [this]() { emit loadStateChanged(true); });
  QObject::connect(view_, &QWebEngineView::loadFinished, this, [this](bool) { emit loadStateChanged(false); });
}

void BrowserTab::appendChat(const QString& role, const QString& content) {
  nlohmann::json m;
  m["role"] = role.toStdString();
  m["content"] = content.toStdString();
  chat_.push_back(m);
}

void BrowserTab::clearChat() {
  chat_ = nlohmann::json::array();
}

} // namespace ui
