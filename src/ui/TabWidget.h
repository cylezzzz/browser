\
/* src/ui/TabWidget.h */
#pragma once
#include <QTabWidget>
#include <QUrl>

#include "ui/BrowserTab.h"
#include "app/NovaApp.h"

namespace ui {

class TabWidget : public QTabWidget {
  Q_OBJECT
public:
  explicit TabWidget(NovaApp* app, QWidget* parent = nullptr);

  BrowserTab* currentBrowserTab() const;
  BrowserTab* addNewTab(const QUrl& url, bool switchTo = true);
  void closeTab(int index);
  void reopenClosedTab();

signals:
  void activeUrlChanged(const QUrl& url);
  void activeTitleChanged(const QString& title);
  void activeLoadStateChanged(bool loading);

private:
  NovaApp* app_;
  struct ClosedTabInfo { QUrl url; QString title; };
  QList<ClosedTabInfo> closed_;

  void hookTabSignals(BrowserTab* tab);
};

} // namespace ui
