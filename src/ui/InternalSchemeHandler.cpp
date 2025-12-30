\
/* src/ui/InternalSchemeHandler.cpp */
#include "ui/InternalSchemeHandler.h"
#include "util/Time.h"
#include "util/Log.h"
#include <QtWebEngineCore/QWebEngineUrlScheme>
#include <QtWebEngineCore/QWebEngineUrlSchemeHandler>
#include <QBuffer>
#include <QUrlQuery>

namespace ui {

static void registerNovaScheme() {
  static bool done = false;
  if (done) return;
  done = true;

  QWebEngineUrlScheme scheme("nova");
  scheme.setFlags(QWebEngineUrlScheme::SecureScheme
                  | QWebEngineUrlScheme::LocalScheme
                  | QWebEngineUrlScheme::LocalAccessAllowed);
  scheme.setSyntax(QWebEngineUrlScheme::Syntax::HostAndPort);
  QWebEngineUrlScheme::registerScheme(scheme);
}

InternalSchemeHandler::InternalSchemeHandler(NovaApp* app, QObject* parent)
  : QWebEngineUrlSchemeHandler(parent), app_(app) {
  registerNovaScheme();
}

QByteArray InternalSchemeHandler::cssBase() {
  QByteArray css;
  css += "body{margin:0;font-family:Segoe UI,Arial,sans-serif;background:#0f1115;color:#e9eef6;}\n";
  css += "a{color:#b9c7ff;text-decoration:none;}a:hover{text-decoration:underline;}\n";
  css += ".wrap{max-width:980px;margin:0 auto;padding:22px;}\n";
  css += ".card{background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.08);border-radius:14px;padding:16px;margin:12px 0;}\n";
  css += ".muted{opacity:0.75;font-size:13px;}\n";
  css += "input,button,select{font-size:14px;border-radius:10px;border:1px solid rgba(255,255,255,0.12);background:#151824;color:#e9eef6;padding:10px;}\n";
  css += "button{cursor:pointer;}\n";
  css += ".grid{display:grid;grid-template-columns:repeat(4,1fr);gap:12px;}\n";
  css += ".tile{padding:14px;border-radius:14px;background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.08);}\n";
  return css;
}

QByteArray InternalSchemeHandler::wrapHtml(const QString& title, const QString& bodyHtml) {
  QString html;
  html += "<!doctype html><html><head><meta charset='utf-8'/>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'/>";
  html += "<title>" + title.toHtmlEscaped() + "</title>";
  html += "<style>" + QString::fromUtf8(cssBase()) + "</style>";
  html += "</head><body>";
  html += bodyHtml;
  html += "</body></html>";
  return html.toUtf8();
}

QByteArray InternalSchemeHandler::pageNewTab(const QUrl&) const {
  QString body;
  body += "<div class='wrap'>";
  body += "<h1>NovaBrowse</h1>";
  body += "<div class='muted'>New Tab • Local-first browser shell with search + AI.</div>";
  body += "<div class='card'>";
  body += "<p>Use the omnibox (Ctrl+L) to enter a URL or a query.</p>";
  body += "<p class='muted'>Internal pages: nova://history • nova://bookmarks • nova://downloads • nova://settings • nova://about</p>";
  body += "</div>";
  body += "<h2>Top Sites (demo)</h2>";
  body += "<div class='grid'>";
  const QStringList sites = {"https://duckduckgo.com", "https://news.ycombinator.com", "https://www.wikipedia.org", "https://github.com"};
  for (const auto& s : sites) {
    body += "<div class='tile'><a href='" + s + "'>" + s.toHtmlEscaped() + "</a></div>";
  }
  body += "</div>";
  body += "</div>";
  return wrapHtml("New Tab", body);
}

QByteArray InternalSchemeHandler::pageAbout() const {
  QString body;
  body += "<div class='wrap'>";
  body += "<h1>About</h1>";
  body += "<div class='card'>";
  body += "<p><b>NovaBrowse</b> is a native C++ browser shell (Qt 6 + QtWebEngine). Rendering is Chromium (embedded). Everything else is local C++ services.</p>";
  body += "<p class='muted'>No cloud AI. No paywall/captcha bypass. No background crawling without user action.</p>";
  body += "</div>";
  body += "<div class='card'><div class='muted'>Build: 0.1.0 • " + QString::fromStdString(util::iso_utc(util::now_ms())) + "</div></div>";
  body += "</div>";
  return wrapHtml("About", body);
}

QByteArray InternalSchemeHandler::pageSettings() const {
  QString body;
  body += "<div class='wrap'><h1>Settings</h1>";
  body += "<div class='card'>";
  body += "<p class='muted'>Primary settings are in data/config.json (next to exe) or AppData config.json.</p>";
  body += "<p>AI Host: <code>" + QString::fromStdString(app_->config().ollamaHost()).toHtmlEscaped() + "</code></p>";
  body += "<p>AI Model: <code>" + QString::fromStdString(app_->config().ollamaModel()).toHtmlEscaped() + "</code></p>";
  body += "<p>Search Provider: <code>" + QString::fromStdString(app_->config().searchProvider()).toHtmlEscaped() + "</code></p>";
  body += "</div>";
  body += "<div class='card'><p class='muted'>UI settings page is minimal in MVP. Real settings UI would edit config + DB-backed flags.</p></div>";
  body += "</div>";
  return wrapHtml("Settings", body);
}

QByteArray InternalSchemeHandler::pageHistory() const {
  QString body;
  body += "<div class='wrap'><h1>History</h1>";
  body += "<div class='card'><div class='muted'>Latest 50</div>";
  body += "<ul>";
  app_->db().query("SELECT url,title,ts FROM history ORDER BY ts DESC LIMIT 50;", {}, [&](int, char** vals, char**) {
    QString url = vals[0] ? vals[0] : "";
    QString title = vals[1] ? vals[1] : "";
    body += "<li><a href='" + url.toHtmlEscaped() + "'>" + (title.isEmpty() ? url : title).toHtmlEscaped() + "</a>"
            + " <span class='muted'>(" + url.toHtmlEscaped() + ")</span></li>";
  });
  body += "</ul></div></div>";
  return wrapHtml("History", body);
}

QByteArray InternalSchemeHandler::pageBookmarks() const {
  QString body;
  body += "<div class='wrap'><h1>Bookmarks</h1>";
  body += "<div class='card'><div class='muted'>All</div><ul>";
  app_->db().query("SELECT url,title,folder,ts FROM bookmarks ORDER BY ts DESC LIMIT 200;", {}, [&](int, char** vals, char**) {
    QString url = vals[0] ? vals[0] : "";
    QString title = vals[1] ? vals[1] : "";
    QString folder = vals[2] ? vals[2] : "";
    body += "<li><a href='" + url.toHtmlEscaped() + "'>" + (title.isEmpty() ? url : title).toHtmlEscaped() + "</a>"
            + " <span class='muted'>" + folder.toHtmlEscaped() + "</span></li>";
  });
  body += "</ul></div></div>";
  return wrapHtml("Bookmarks", body);
}

QByteArray InternalSchemeHandler::pageDownloads() const {
  QString body;
  body += "<div class='wrap'><h1>Downloads</h1>";
  body += "<div class='card'><div class='muted'>Latest 50</div><ul>";
  app_->db().query("SELECT path,url,ts,status FROM downloads ORDER BY ts DESC LIMIT 50;", {}, [&](int, char** vals, char**) {
    QString path = vals[0] ? vals[0] : "";
    QString url = vals[1] ? vals[1] : "";
    QString status = vals[3] ? vals[3] : "";
    body += "<li><span class='muted'>" + status.toHtmlEscaped() + "</span> "
            + "<a href='" + url.toHtmlEscaped() + "'>" + url.toHtmlEscaped() + "</a> "
            + "<span class='muted'>" + path.toHtmlEscaped() + "</span></li>";
  });
  body += "</ul></div></div>";
  return wrapHtml("Downloads", body);
}

QByteArray InternalSchemeHandler::pageSearch(const QUrl& url) {
  QUrlQuery q(url);
  QString query = q.queryItemValue("q");
  QString type = q.queryItemValue("type");
  if (type.isEmpty()) type = "web";

  // This page is rendered synchronously from cached results in DB if present.
  // UI triggers async search and then navigates to this page after caching.
  std::string keyQuery = query.toStdString();
  std::string provider = app_->config().searchProvider();
  std::string json;
  app_->db().query("SELECT json FROM search_cache WHERE query=? AND provider=?;", {keyQuery, provider}, [&](int, char** vals, char**) {
    if (vals && vals[0]) json = vals[0];
  });

  services::search::SearchResponse sr;
  sr.query = keyQuery;
  sr.provider = provider;
  if (!json.empty()) {
    try {
      auto j = nlohmann::json::parse(json);
      if (j.contains("items") && j["items"].is_array()) {
        for (const auto& it : j["items"]) {
          services::search::SearchResultItem si;
          si.title = it.value("title", "");
          si.url = it.value("url", "");
          si.snippet = it.value("snippet", "");
          sr.items.push_back(si);
        }
      }
    } catch (...) {}
  }

  std::string html = services::search::DdgHtmlSearch::toHtmlResultsPage(sr, type.toStdString());
  return QByteArray::fromStdString(html);
}

QByteArray InternalSchemeHandler::pageDeepSearch(const QUrl& url) {
  QUrlQuery q(url);
  QString name = q.queryItemValue("name");
  QString body = "<div class='wrap'><h1>Deep Search</h1>";

  if (name.isEmpty()) {
    body += "<div class='card'><p>Use the side panel to analyze the current page and create a profile.</p></div>";
  } else {
    auto results = app_->deepsearch().searchProfiles(name);
    body += "<div class='card'><div class='muted'>Profiles matching: " + name.toHtmlEscaped() + "</div>";
    body += "<ul>";
    for (const auto& p : results) {
      body += "<li><b>" + QString::fromStdString(p.name).toHtmlEscaped() + "</b> "
              + "<span class='muted'>(" + QString::fromStdString(p.type).toHtmlEscaped() + ")</span> "
              + "<span class='muted'>id=" + QString::fromStdString(p.entityId).left(10).toHtmlEscaped() + "…</span></li>";
    }
    body += "</ul></div>";
    body += "<div class='card'><p class='muted'>Graph view is in the Side Panel (Entities tab).</p></div>";
  }

  body += "</div>";
  return wrapHtml("Deep Search", body);
}

void InternalSchemeHandler::requestStarted(QWebEngineUrlRequestJob* job) {
  const QUrl url = job->requestUrl();
  const QString host = url.host();
  QByteArray data;

  if (host == "newtab") data = pageNewTab(url);
  else if (host == "about") data = pageAbout();
  else if (host == "settings") data = pageSettings();
  else if (host == "history") data = pageHistory();
  else if (host == "bookmarks") data = pageBookmarks();
  else if (host == "downloads") data = pageDownloads();
  else if (host == "search") data = pageSearch(url);
  else if (host == "deepsearch") data = pageDeepSearch(url);
  else data = wrapHtml("Nova", "<div class='wrap'><h1>Unknown internal page</h1></div>");

  QBuffer* buf = new QBuffer(job);
  buf->setData(data);
  buf->open(QIODevice::ReadOnly);
  job->reply("text/html", buf);
}

} // namespace ui
