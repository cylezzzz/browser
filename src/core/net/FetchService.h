\
/* src/core/net/FetchService.h */
#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <unordered_map>
#include <mutex>

#include "core/net/RateLimiter.h"

namespace core::net {

struct FetchResult {
  int status = 0;
  QByteArray body;
  QByteArray contentType;
  QUrl finalUrl;
  QString error;
  QByteArray etag;
  QByteArray lastModified;
};

class FetchService : public QObject {
  Q_OBJECT
public:
  explicit FetchService(QObject* parent = nullptr);

  void setTimeoutMs(int ms);
  void setRetries(int n);
  void setStripTracking(bool on);
  void setRate(double permitsPerSec);

  void fetch(const QUrl& url, const std::function<void(const FetchResult&)>& cb);

private:
  void fetchOnce(const QUrl& url, int attempt, const std::function<void(const FetchResult&)>& cb);

  QNetworkAccessManager nam_;
  int timeoutMs_;
  int retries_;
  bool stripTracking_;
  RateLimiter limiter_;

  struct CacheEntry {
    QByteArray body;
    QByteArray contentType;
    QByteArray etag;
    QByteArray lastModified;
    qint64 tsMs = 0;
  };

  std::mutex cacheMu_;
  std::unordered_map<std::string, CacheEntry> cache_;

  QString cacheKey(const QUrl& url) const;
};

} // namespace core::net
