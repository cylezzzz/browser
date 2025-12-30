\
/* src/ui/InternalSchemeHandler.h */
#pragma once
#include <QObject>
#include <QtWebEngineCore/QWebEngineUrlSchemeHandler>
#include <QtWebEngineCore/QWebEngineUrlRequestJob>

#include "app/NovaApp.h"

namespace ui {

class InternalSchemeHandler : public QWebEngineUrlSchemeHandler {
  Q_OBJECT
public:
  explicit InternalSchemeHandler(NovaApp* app, QObject* parent = nullptr);

  void requestStarted(QWebEngineUrlRequestJob* job) override;

private:
  NovaApp* app_;

  QByteArray pageNewTab(const QUrl& url) const;
  QByteArray pageAbout() const;
  QByteArray pageSettings() const;
  QByteArray pageHistory() const;
  QByteArray pageBookmarks() const;
  QByteArray pageDownloads() const;
  QByteArray pageSearch(const QUrl& url);
  QByteArray pageDeepSearch(const QUrl& url);

  static QByteArray wrapHtml(const QString& title, const QString& bodyHtml);
  static QByteArray cssBase();
};

} // namespace ui
