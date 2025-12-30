\
/* src/ui/MainWindow.cpp */
#include "ui/MainWindow.h"
#include "core/net/UrlTools.h"
#include "util/Log.h"
#include "util/Time.h"
#include "ui/ScrapeDialog.h"

#include <QVBoxLayout>
#include <QKeySequence>
#include <QShortcut>
#include <QMessageBox>
#include <QInputDialog>
#include <QtWebEngineCore/QWebEngineProfile>
#include <QtWebEngineCore/QWebEngineCookieStore>
#include <QtWebEngineCore/QWebEngineUrlScheme>
#include <QtWebEngineCore/QWebEngineUrlSchemeHandler>
#include <QtWebEngineWidgets/QWebEngineFindTextResult>
#include <QUrlQuery>

namespace ui {

MainWindow::MainWindow(NovaApp* app, QWidget* parent)
  : QMainWindow(parent),
    app_(app),
    tabs_(new TabWidget(app, this)),
    side_(new SidePanel(app, this)),
    sideDock_(new QDockWidget("AI", this)),
    omnibox_(new QLineEdit(this)),
    status_(new QLabel(this)),
    back_(nullptr),
    forward_(nullptr),
    reload_(nullptr),
    stop_(nullptr),
    newTab_(nullptr),
    closeTab_(nullptr),
    reopenTab_(nullptr),
    toggleSide_(nullptr),
    schemeHandler_(new InternalSchemeHandler(app, this)) {

  setWindowTitle("NovaBrowse");
  resize(1280, 820);

  setupUi();
  setupShortcuts();
  hookSignals();

  // Register scheme handler for nova://
  QWebEngineProfile::defaultProfile()->installUrlSchemeHandler("nova", schemeHandler_);

  // Default cookie policy for fetch pipeline is handled in FetchService (no cookies).
  // QtWebEngine itself uses its own cookie jar as a normal browser.
}

void MainWindow::setupUi() {
  // Toolbar
  QToolBar* tb = new QToolBar(this);
  tb->setMovable(false);
  tb->setIconSize(QSize(16,16));
  addToolBar(Qt::TopToolBarArea, tb);

  back_ = tb->addAction("◀");
  forward_ = tb->addAction("▶");
  reload_ = tb->addAction("⟳");
  stop_ = tb->addAction("⨯");
  newTab_ = tb->addAction("+");
  toggleSide_ = tb->addAction("AI");

  omnibox_->setClearButtonEnabled(true);
  omnibox_->setPlaceholderText("Search or enter address");
  omnibox_->setMinimumWidth(520);
  tb->addWidget(omnibox_);

  // Status bar
  statusBar()->addWidget(status_, 1);
  status_->setText("Ready");

  // Central
  setCentralWidget(tabs_);

  // Side panel dock
  sideDock_->setWidget(side_);
  sideDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  addDockWidget(Qt::RightDockWidgetArea, sideDock_);
  sideDock_->setVisible(true);
}

void MainWindow::setupShortcuts() {
  // Chrome-like shortcuts
  new QShortcut(QKeySequence("Ctrl+L"), this, [this]() { omnibox_->setFocus(); omnibox_->selectAll(); });
  new QShortcut(QKeySequence("Ctrl+T"), this, [this]() { onNewTab(); });
  new QShortcut(QKeySequence("Ctrl+W"), this, [this]() { onCloseTab(); });
  new QShortcut(QKeySequence("Ctrl+Shift+T"), this, [this]() { onReopenTab(); });
  new QShortcut(QKeySequence("Ctrl+Tab"), this, [this]() {
    int idx = (tabs_->currentIndex() + 1) % tabs_->count();
    tabs_->setCurrentIndex(idx);
  });
  new QShortcut(QKeySequence("Ctrl+Shift+Tab"), this, [this]() {
    int idx = tabs_->currentIndex() - 1;
    if (idx < 0) idx = tabs_->count() - 1;
    tabs_->setCurrentIndex(idx);
  });
  new QShortcut(QKeySequence("Ctrl+F"), this, [this]() { onFindInPage(); });
  new QShortcut(QKeySequence("Ctrl+Shift+S"), this, [this]() { onScrapeDialog(); });
  new QShortcut(QKeySequence("Ctrl+Shift+A"), this, [this]() { onAnalyzeShortcut(); });
  new QShortcut(QKeySequence("Ctrl+Return"), this, [this]() { onAskShortcut(); });
}

void MainWindow::hookSignals() {
  connect(omnibox_, &QLineEdit::returnPressed, this, &MainWindow::onNavigate);

  connect(back_, &QAction::triggered, this, &MainWindow::onBack);
  connect(forward_, &QAction::triggered, this, &MainWindow::onForward);
  connect(reload_, &QAction::triggered, this, &MainWindow::onReload);
  connect(stop_, &QAction::triggered, this, &MainWindow::onStop);
  connect(newTab_, &QAction::triggered, this, &MainWindow::onNewTab);
  connect(toggleSide_, &QAction::triggered, this, [this]() {
    sideDock_->setVisible(!sideDock_->isVisible());
  });

  connect(tabs_, &TabWidget::activeUrlChanged, this, [this](const QUrl& url) {
    omnibox_->setText(url.toString());
    side_->setActiveTab(tabs_->currentBrowserTab());
  });

  connect(tabs_, &TabWidget::activeTitleChanged, this, [this](const QString& title) {
    setWindowTitle(title.isEmpty() ? "NovaBrowse" : (title + " - NovaBrowse"));
    recordHistory(tabs_->currentBrowserTab()->view()->url(), title);
  });

  connect(tabs_, &TabWidget::activeLoadStateChanged, this, [this](bool loading) {
    status_->setText(loading ? "Loading…" : "Ready");
  });

  // WebEngine navigation events
  for (int i = 0; i < tabs_->count(); ++i) {
    auto* t = qobject_cast<BrowserTab*>(tabs_->widget(i));
    if (!t) continue;
    connect(t->view(), &QWebEngineView::urlChanged, this, [this](const QUrl& url) {
      omnibox_->setText(url.toString());
    });
  }
}

void MainWindow::recordHistory(const QUrl& url, const QString& title) {
  if (!url.isValid()) return;
  if (url.scheme() == "nova") return;
  const std::string u = url.toString().toStdString();
  const std::string t = title.toStdString();
  app_->db().execParams("INSERT INTO history(url,title,ts) VALUES(?,?,?);",
                        {u, t, std::to_string(util::now_ms())});
}

void MainWindow::performSearchIfNeeded(const QString& inputText) {
  // If input is a query (not a URL), normalizeUserInput returns nova://search.
  QUrl target = core::net::normalizeUserInput(inputText);
  if (target.scheme() == "nova" && target.host() == "search") {
    QUrlQuery q(target);
    QString query = q.queryItemValue("q");
    if (query.isEmpty()) return;

    status_->setText("Searching…");
    app_->search().searchWeb(query, [=](const services::search::SearchResponse& r) {
      auto j = services::search::DdgHtmlSearch::toJson(r);
      const std::string jsonStr = j.dump(2);

      app_->db().execParams(
        "INSERT INTO search_cache(query,provider,ts,json) VALUES(?,?,?,?) "
        "ON CONFLICT(query,provider) DO UPDATE SET ts=excluded.ts, json=excluded.json;",
        {r.query, r.provider, std::to_string(util::now_ms()), jsonStr}
      );

      side_->setSearchContext(query, QString::fromStdString(r.provider), QString::fromStdString(jsonStr));

      // Navigate to internal search results page
      QUrl u("nova://search");
      QUrlQuery qq;
      qq.addQueryItem("q", query);
      qq.addQueryItem("type", "web");
      u.setQuery(qq);

      QMetaObject::invokeMethod(this, [=]() {
        auto* tab = tabs_->currentBrowserTab();
        if (!tab) tab = tabs_->addNewTab(u, true);
        tab->view()->setUrl(u);
        status_->setText("Ready");
      });
    });
  } else {
    auto* tab = tabs_->currentBrowserTab();
    if (!tab) tab = tabs_->addNewTab(target, true);
    tab->view()->setUrl(target);
  }
}

void MainWindow::onNavigate() {
  QString input = omnibox_->text().trimmed();
  if (input.isEmpty()) return;
  performSearchIfNeeded(input);
}

void MainWindow::onBack() {
  if (auto* t = tabs_->currentBrowserTab()) t->view()->back();
}

void MainWindow::onForward() {
  if (auto* t = tabs_->currentBrowserTab()) t->view()->forward();
}

void MainWindow::onReload() {
  if (auto* t = tabs_->currentBrowserTab()) t->view()->reload();
}

void MainWindow::onStop() {
  if (auto* t = tabs_->currentBrowserTab()) t->view()->stop();
}

void MainWindow::onNewTab() {
  tabs_->addNewTab(QUrl("nova://newtab"), true);
}

void MainWindow::onCloseTab() {
  int idx = tabs_->currentIndex();
  if (idx >= 0) tabs_->closeTab(idx);
}

void MainWindow::onReopenTab() {
  tabs_->reopenClosedTab();
}

void MainWindow::onFindInPage() {
  auto* t = tabs_->currentBrowserTab();
  if (!t) return;

  bool ok = false;
  QString needle = QInputDialog::getText(this, "Find in page", "Text:", QLineEdit::Normal, "", &ok);
  if (!ok || needle.trimmed().isEmpty()) return;

  t->view()->findText("", QWebEnginePage::FindFlag::FindBackward); // reset
  t->view()->findText(needle, QWebEnginePage::FindFlags(), [this](const QWebEngineFindTextResult& res) {
    status_->setText(QString("Matches: %1").arg(res.numberOfMatches()));
  });
}

void MainWindow::onScrapeDialog() {
  auto* t = tabs_->currentBrowserTab();
  if (!t) return;

  QUrl url = t->view()->url();
  t->view()->page()->toHtml([=](const QString& html) {
    ScrapeDialog dlg(this);
    dlg.setPage(url, html);
    dlg.exec();
  });
}

void MainWindow::onAnalyzeShortcut() {
  side_->setActiveTab(tabs_->currentBrowserTab());
  side_->onAnalyzePage();
  if (!sideDock_->isVisible()) sideDock_->setVisible(true);
}

void MainWindow::onAskShortcut() {
  if (!sideDock_->isVisible()) sideDock_->setVisible(true);
  side_->setActiveTab(tabs_->currentBrowserTab());
  side_->setFocus();
}

void MainWindow::closeEvent(QCloseEvent* e) {
  QMainWindow::closeEvent(e);
}

} // namespace ui
