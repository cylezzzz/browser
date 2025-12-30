\
/* src/services/ai/OllamaClient.h */
#pragma once
#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <nlohmann/json.hpp>

#include "core/net/FetchService.h"

namespace services::ai {

struct OllamaHealth {
  bool ok = false;
  QString version;
  QString error;
};

class OllamaClient : public QObject {
  Q_OBJECT
public:
  explicit OllamaClient(core::net::FetchService* fetcher, QObject* parent = nullptr);

  void setHost(const QString& host);
  QString host() const { return host_; }

  void health(const std::function<void(const OllamaHealth&)>& cb);
  void listModels(const std::function<void(const nlohmann::json&)>& cb);
  void hasModel(const QString& model, const std::function<void(bool)>& cb);

  void generate(const QString& model, const QString& prompt,
                const std::function<void(const QString& text, const nlohmann::json& raw)>& cb);

  void chat(const QString& model, const nlohmann::json& messages,
            const std::function<void(const QString& text, const nlohmann::json& raw)>& cb);

private:
  void postJson(const QUrl& url, const nlohmann::json& body,
                const std::function<void(int status, const QByteArray& data, const QString& err)>& cb);

  core::net::FetchService* fetcher_;
  QString host_;
};

} // namespace services::ai
