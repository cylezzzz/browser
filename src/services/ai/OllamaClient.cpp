\
/* src/services/ai/OllamaClient.cpp */
#include "services/ai/OllamaClient.h"
#include "util/Log.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QTimer>

namespace services::ai {

OllamaClient::OllamaClient(core::net::FetchService* fetcher, QObject* parent)
  : QObject(parent), fetcher_(fetcher), host_("http://127.0.0.1:11434") {}

void OllamaClient::setHost(const QString& host) {
  host_ = host.trimmed();
  if (host_.endsWith("/")) host_.chop(1);
}

void OllamaClient::health(const std::function<void(const OllamaHealth&)>& cb) {
  // Ollama doesn't guarantee a public "health" endpoint, but /api/tags works as availability check.
  QUrl url(host_ + "/api/tags");
  fetcher_->fetch(url, [=](const core::net::FetchResult& fr) {
    OllamaHealth h;
    if (fr.status == 200 && fr.error.isEmpty()) {
      h.ok = true;
      h.version = "ok";
    } else {
      h.ok = false;
      h.error = fr.error.isEmpty() ? QString("HTTP %1").arg(fr.status) : fr.error;
    }
    cb(h);
  });
}

void OllamaClient::listModels(const std::function<void(const nlohmann::json&)>& cb) {
  QUrl url(host_ + "/api/tags");
  fetcher_->fetch(url, [=](const core::net::FetchResult& fr) {
    if (fr.status != 200 || !fr.error.isEmpty()) {
      cb(nlohmann::json::object());
      return;
    }
    try {
      auto j = nlohmann::json::parse(fr.body.constData(), fr.body.constData() + fr.body.size());
      cb(j);
    } catch (...) {
      cb(nlohmann::json::object());
    }
  });
}

void OllamaClient::hasModel(const QString& model, const std::function<void(bool)>& cb) {
  listModels([=](const nlohmann::json& j) {
    if (!j.is_object() || !j.contains("models") || !j["models"].is_array()) { cb(false); return; }
    for (const auto& m : j["models"]) {
      if (m.contains("name") && m["name"].is_string()) {
        if (QString::fromStdString(m["name"].get<std::string>()).startsWith(model)) { cb(true); return; }
      }
    }
    cb(false);
  });
}

void OllamaClient::postJson(const QUrl& url, const nlohmann::json& body,
                            const std::function<void(int status, const QByteArray& data, const QString& err)>& cb) {
  // Use a dedicated QNetworkAccessManager owned by fetcher? We intentionally keep fetch pipeline cookie-free.
  // Here: we use QNetworkAccessManager inside FetchService, but it doesn't expose post. So we do our own NAM per call.
  QNetworkAccessManager* nam = new QNetworkAccessManager(this);
  QNetworkRequest req(url);
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setHeader(QNetworkRequest::UserAgentHeader, "NovaBrowse/0.1 (+OllamaClient)");

  QByteArray payload = QByteArray::fromStdString(body.dump());

  QNetworkReply* reply = nam->post(req, payload);

  QTimer* timer = new QTimer(reply);
  timer->setSingleShot(true);
  QObject::connect(timer, &QTimer::timeout, reply, [reply]() {
    if (reply->isRunning()) reply->abort();
  });
  timer->start(20000);

  QObject::connect(reply, &QNetworkReply::finished, reply, [=]() {
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString err;
    if (reply->error() != QNetworkReply::NoError) err = reply->errorString();
    QByteArray data = reply->readAll();
    cb(status, data, err);
    reply->deleteLater();
    nam->deleteLater();
  });
}

void OllamaClient::generate(const QString& model, const QString& prompt,
                            const std::function<void(const QString& text, const nlohmann::json& raw)>& cb) {
  QUrl url(host_ + "/api/generate");
  nlohmann::json body;
  body["model"] = model.toStdString();
  body["prompt"] = prompt.toStdString();
  body["stream"] = false;

  postJson(url, body, [=](int status, const QByteArray& data, const QString& err) {
    if (status != 200 || !err.isEmpty()) {
      cb(QString("Ollama error: %1 (HTTP %2)").arg(err).arg(status), nlohmann::json::object());
      return;
    }
    try {
      auto j = nlohmann::json::parse(data.constData(), data.constData() + data.size());
      QString text = j.contains("response") ? QString::fromStdString(j["response"].get<std::string>()) : QString();
      cb(text, j);
    } catch (...) {
      cb("Ollama response parse error.", nlohmann::json::object());
    }
  });
}

void OllamaClient::chat(const QString& model, const nlohmann::json& messages,
                        const std::function<void(const QString& text, const nlohmann::json& raw)>& cb) {
  QUrl url(host_ + "/api/chat");
  nlohmann::json body;
  body["model"] = model.toStdString();
  body["messages"] = messages;
  body["stream"] = false;

  postJson(url, body, [=](int status, const QByteArray& data, const QString& err) {
    if (status != 200 || !err.isEmpty()) {
      cb(QString("Ollama error: %1 (HTTP %2)").arg(err).arg(status), nlohmann::json::object());
      return;
    }
    try {
      auto j = nlohmann::json::parse(data.constData(), data.constData() + data.size());
      QString text;
      if (j.contains("message") && j["message"].is_object() && j["message"].contains("content")) {
        text = QString::fromStdString(j["message"]["content"].get<std::string>());
      }
      cb(text.isEmpty() ? QString("(no content)") : text, j);
    } catch (...) {
      cb("Ollama response parse error.", nlohmann::json::object());
    }
  });
}

} // namespace services::ai
