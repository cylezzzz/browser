\
/* src/ui/BrowserTab.h */
#pragma once
#include <QWidget>
#include <QtWebEngineWidgets/QWebEngineView>
#include <QtWebEngineCore/QWebEnginePage>
#include <nlohmann/json.hpp>

namespace ui {

class BrowserTab : public QWidget {
  Q_OBJECT
public:
  explicit BrowserTab(QWidget* parent = nullptr);

  QWebEngineView* view() const { return view_; }

  void appendChat(const QString& role, const QString& content);
  const nlohmann::json& chatHistory() const { return chat_; }
  void clearChat();

signals:
  void titleChanged(const QString& title);
  void urlChanged(const QUrl& url);
  void loadStateChanged(bool isLoading);

private:
  QWebEngineView* view_;
  nlohmann::json chat_; // [{role,content}]
};

} // namespace ui
