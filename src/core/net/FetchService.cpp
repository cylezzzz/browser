\
/* src/core/net/FetchService.cpp */
#include "core/net/FetchService.h"
#include "core/net/UrlTools.h"
#include "util/Log.h"
#include "util/Time.h"
#include <QTimer>
#include <QNetworkRequest>

namespace core::net {

FetchService::FetchService(QObject* parent)
  : QObject(parent),
    timeoutMs_(12000),
    retries_(2),
    stripTracking_(true),
    limiter_(1.5) {
}

void FetchService::setTimeoutMs(int ms) { timeoutMs_ = ms > 1000 ? ms : 1000; }
void FetchService::setRetries(int n) { retries_ = (n < 0) ? 0 : n; }
void FetchService::setStripTracking(bool on) { stripTracking_ = on; }
void FetchService::setRate(double permitsPerSec) { limiter_.setRate(permitsPerSec); }

QString FetchService::cacheKey(const QUrl& url) const {
  return (url.scheme() + "://" + url.host() + url.path() + "?" + url.query()).toUtf8().toHex();
}

void FetchService::fetch(const QUrl& url, const std::function<void(const FetchResult&)>& cb) {
  fetchOnce(url, 0, cb);
}

void FetchService::fetchOnce(const QUrl& inUrl, int attempt, const std::function<void(const FetchResult&)>& cb) {
  QUrl url = inUrl;
  if (stripTracking_) url = core::net::stripTracking(url); // FIX: avoid name shadowing

  limiter_.acquire();

  // Simple cache with validators
  const QString key = cacheKey(url);
  CacheEntry cached;
  bool haveCache = false;
  {
    std::lock_guard<std::mutex> lk(cacheMu_);
    auto it = cache_.find(key.toStdString());
    if (it != cache_.end()) {
      cached = it->second;
      haveCache = true;
    }
  }

  QNetworkRequest req(url);
  req.setHeader(QNetworkRequest::UserAgentHeader, "NovaBrowse/0.1 (+QtWebEngine)");
  req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

  if (haveCache) {
    if (!cached.etag.isEmpty()) req.setRawHeader("If-None-Match", cached.etag);
    if (!cached.lastModified.isEmpty()) req.setRawHeader("If-Modified-Since", cached.lastModified);
  }

  // Hygiene: don't send cookies for fetch pipeline
  req.setRawHeader("Cookie", "");

  QNetworkReply* reply = nam_.get(req);

  QTimer* timer = new QTimer(reply);
  timer->setSingleShot(true);
  QObject::connect(timer, &QTimer::timeout, reply, [reply]() {
    if (reply->isRunning()) reply->abort();
  });
  timer->start(timeoutMs_);

  QObject::connect(reply, &QNetworkReply::finished, reply, [=]() {
    FetchResult fr;
    fr.finalUrl = reply->url();
    fr.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    fr.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
    fr.etag = reply->rawHeader("ETag");
    fr.lastModified = reply->rawHeader("Last-Modified");

    if (reply->error() == QNetworkReply::NoError) {
      if (fr.status == 304 && haveCache) {
        fr.body = cached.body;
        fr.contentType = cached.contentType;
        fr.status = 200;
        cb(fr);
      } else {
        fr.body = reply->readAll();
        // Cache only small-ish responses (basic safety)
        if (fr.body.size() <= 1024 * 1024) {
          CacheEntry ce;
          ce.body = fr.body;
          ce.contentType = fr.contentType;
          ce.etag = fr.etag;
          ce.lastModified = fr.lastModified;
          ce.tsMs = util::now_ms();
          std::lock_guard<std::mutex> lk(cacheMu_);
          cache_[key.toStdString()] = ce;
        }
        cb(fr);
      }
    } else {
      fr.error = reply->errorString();
      util::Log::warn("Fetch error " + fr.error.toStdString() + " url=" + url.toString().toStdString());
      if (attempt < retries_) {
        fetchOnce(inUrl, attempt + 1, cb);
      } else {
        cb(fr);
      }
    }
    reply->deleteLater();
  });
}

} // namespace core::net
