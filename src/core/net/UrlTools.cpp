\
/* src/core/net/UrlTools.cpp */
#include "core/net/UrlTools.h"
#include <QUrlQuery>

namespace core::net {

static bool looksLikeUrl(const QString& t) {
  if (t.contains("://")) return true;
  if (t.startsWith("www.", Qt::CaseInsensitive)) return true;
  if (t.contains('.') && !t.contains(' ')) return true;
  return false;
}

QUrl normalizeUserInput(const QString& text) {
  QString t = text.trimmed();
  if (t.isEmpty()) return QUrl("nova://newtab");

  if (t.startsWith("nova://")) return QUrl(t);

  if (looksLikeUrl(t)) {
    if (!t.contains("://")) t = "https://" + t;
    QUrl url(t);
    if (!url.isValid()) return QUrl("nova://newtab");
    return url;
  }

  // Treat as search query
  QUrl url("nova://search");
  QUrlQuery q;
  q.addQueryItem("q", t);
  q.addQueryItem("type", "web");
  url.setQuery(q);
  return url;
}

QUrl stripTracking(const QUrl& url) {
  if (!url.isValid()) return url;
  QUrl out(url);
  QUrlQuery q(out);
  static const QStringList badKeys = {
    "utm_source","utm_medium","utm_campaign","utm_term","utm_content",
    "gclid","fbclid","igshid","mc_cid","mc_eid","ref","ref_src","spm","yclid"
  };
  bool changed = false;
  for (const auto& k : badKeys) {
    if (q.hasQueryItem(k)) {
      q.removeAllQueryItems(k);
      changed = true;
    }
  }
  if (changed) out.setQuery(q);
  return out;
}

QString hostKey(const QUrl& url) {
  return url.host().toLower();
}

} // namespace core::net
