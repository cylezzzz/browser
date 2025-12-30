\
/* src/ui/TabWidget.cpp */
#include "ui/TabWidget.h"

namespace ui {

TabWidget::TabWidget(NovaApp* app, QWidget* parent)
  : QTabWidget(parent), app_(app) {
  setDocumentMode(true);
  setTabsClosable(true);
  setMovable(true);

  QObject::connect(this, &QTabWidget::tabCloseRequested, this, &TabWidget::closeTab);
  QObject::connect(this, &QTabWidget::currentChanged, this, [this](int) {
    auto* t = currentBrowserTab();
    if (!t) return;
    emit activeUrlChanged(t->view()->url());
    emit activeTitleChanged(t->view()->title());
  });

  addNewTab(QUrl("nova://newtab"), true);
}

BrowserTab* TabWidget::currentBrowserTab() const {
  return qobject_cast<BrowserTab*>(currentWidget());
}

void TabWidget::hookTabSignals(BrowserTab* tab) {
  QObject::connect(tab, &BrowserTab::titleChanged, this, [this, tab](const QString& title) {
    int idx = indexOf(tab);
    if (idx >= 0) setTabText(idx, title.isEmpty() ? "New Tab" : title.left(28));
    if (tab == currentBrowserTab()) emit activeTitleChanged(title);
  });
  QObject::connect(tab, &BrowserTab::urlChanged, this, [this, tab](const QUrl& url) {
    if (tab == currentBrowserTab()) emit activeUrlChanged(url);
  });
  QObject::connect(tab, &BrowserTab::loadStateChanged, this, [this, tab](bool loading) {
    if (tab == currentBrowserTab()) emit activeLoadStateChanged(loading);
  });
}

BrowserTab* TabWidget::addNewTab(const QUrl& url, bool switchTo) {
  auto* tab = new BrowserTab(this);
  hookTabSignals(tab);
  int idx = addTab(tab, "New Tab");
  if (switchTo) setCurrentIndex(idx);
  tab->view()->setUrl(url);
  return tab;
}

void TabWidget::closeTab(int index) {
  QWidget* w = widget(index);
  auto* tab = qobject_cast<BrowserTab*>(w);
  if (tab) {
    ClosedTabInfo ci;
    ci.url = tab->view()->url();
    ci.title = tab->view()->title();
    closed_.push_back(ci);
    if (closed_.size() > 20) closed_.removeFirst();
  }
  removeTab(index);
  if (w) w->deleteLater();
  if (count() == 0) addNewTab(QUrl("nova://newtab"), true);
}

void TabWidget::reopenClosedTab() {
  if (closed_.isEmpty()) return;
  auto ci = closed_.takeLast();
  auto* tab = addNewTab(ci.url, true);
  int idx = indexOf(tab);
  if (idx >= 0 && !ci.title.isEmpty()) setTabText(idx, ci.title.left(28));
}

} // namespace ui
