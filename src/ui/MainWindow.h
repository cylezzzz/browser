\
/* src/ui/MainWindow.h */
#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include <QToolBar>
#include <QAction>
#include <QDockWidget>
#include <QLabel>

#include "app/NovaApp.h"
#include "ui/TabWidget.h"
#include "ui/SidePanel.h"
#include "ui/InternalSchemeHandler.h"

namespace ui {

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(NovaApp* app, QWidget* parent = nullptr);

protected:
  void closeEvent(QCloseEvent* e) override;

private slots:
  void onNavigate();
  void onBack();
  void onForward();
  void onReload();
  void onStop();
  void onNewTab();
  void onCloseTab();
  void onReopenTab();
  void onFindInPage();
  void onScrapeDialog();
  void onAnalyzeShortcut();
  void onAskShortcut();

private:
  NovaApp* app_;
  TabWidget* tabs_;
  SidePanel* side_;
  QDockWidget* sideDock_;
  QLineEdit* omnibox_;
  QLabel* status_;
  QAction* back_;
  QAction* forward_;
  QAction* reload_;
  QAction* stop_;
  QAction* newTab_;
  QAction* closeTab_;
  QAction* reopenTab_;
  QAction* toggleSide_;

  InternalSchemeHandler* schemeHandler_;

  void setupUi();
  void setupShortcuts();
  void hookSignals();
  void recordHistory(const QUrl& url, const QString& title);

  void performSearchIfNeeded(const QString& inputText);
};

} // namespace ui
