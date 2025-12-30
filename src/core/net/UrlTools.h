\
/* src/core/net/UrlTools.h */
#pragma once
#include <QString>
#include <QUrl>

namespace core::net {

QUrl normalizeUserInput(const QString& text);
QUrl stripTracking(const QUrl& url);
QString hostKey(const QUrl& url);

} // namespace core::net
