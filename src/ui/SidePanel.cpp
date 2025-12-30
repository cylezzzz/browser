\
/* src/ui/SidePanel.cpp */
#include "ui/SidePanel.h"
#include "ui/BrowserTab.h"
#include "services/ai/RagComposer.h"
#include "util/Log.h"
#include "util/Time.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

namespace ui {

SidePanel::SidePanel(NovaApp* app, QWidget* parent)
  : QWidget(parent),
    app_(app),
    activeTab_(nullptr),
    tabs_(new QTabWidget(this)),
    chatView_(new QPlainTextEdit(this)),
    chatInput_(new QLineEdit(this)),
    askBtn_(new QPushButton("Ask (Ctrl+Enter)", this)),
    analyzeBtn_(new QPushButton("Analyze Page (Ctrl+Shift+A)", this)),
    overviewBtn_(new QPushButton("AI Overview (Search)", this)),
    usePageCtx_(new QCheckBox("Use Page Context", this)),
    useSearchCtx_(new QCheckBox("Use Search Context", this)),
    graph_(new EntityGraphWidget(this)),
    entityList_(new QPlainTextEdit(this)),
    deepSearchBtn_(new QPushButton("Open DeepSearch", this)) {

  setMinimumWidth(360);

  // Chat tab layout
  QWidget* chatTab = new QWidget(this);
  chatView_->setReadOnly(true);
  chatView_->setPlaceholderText("AI conversation…");
  chatInput_->setPlaceholderText("Ask about the current page/search…");
  usePageCtx_->setChecked(true);
  useSearchCtx_->setChecked(true);

  auto* chatButtons = new QHBoxLayout();
  chatButtons->addWidget(askBtn_);
  chatButtons->addWidget(analyzeBtn_);
  chatButtons->addWidget(overviewBtn_);

  auto* toggles = new QHBoxLayout();
  toggles->addWidget(usePageCtx_);
  toggles->addWidget(useSearchCtx_);
  toggles->addStretch(1);

  auto* chatLayout = new QVBoxLayout(chatTab);
  chatLayout->addLayout(toggles);
  chatLayout->addWidget(chatView_, 1);
  auto* inputRow = new QHBoxLayout();
  inputRow->addWidget(chatInput_, 1);
  inputRow->addWidget(askBtn_);
  chatLayout->addLayout(inputRow);
  chatLayout->addLayout(chatButtons);

  // Entities tab layout
  QWidget* entTab = new QWidget(this);
  entityList_->setReadOnly(true);
  entityList_->setPlaceholderText("Entities…");
  auto* entLayout = new QVBoxLayout(entTab);
  entLayout->addWidget(graph_, 2);
  entLayout->addWidget(entityList_, 1);
  entLayout->addWidget(deepSearchBtn_);

  tabs_->addTab(chatTab, "AI");
  tabs_->addTab(entTab, "Entities");

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(tabs_);

  connect(askBtn_, &QPushButton::clicked, this, &SidePanel::onAsk);
  connect(analyzeBtn_, &QPushButton::clicked, this, &SidePanel::onAnalyzePage);
  connect(overviewBtn_, &QPushButton::clicked, this, &SidePanel::onOverviewFromSearch);
  connect(chatInput_, &QLineEdit::returnPressed, this, &SidePanel::onAsk);
  connect(deepSearchBtn_, &QPushButton::clicked, this, [this]() {
    if (!activeTab_) return;
    activeTab_->view()->setUrl(QUrl("nova://deepsearch"));
  });

  appendChatLine("system", "Local AI: Ollama. If it isn't running, you'll get an error instead of magic.");
}

void SidePanel::setActiveTab(BrowserTab* tab) {
  activeTab_ = tab;
}

void SidePanel::setSearchContext(const QString& query, const QString& provider, const QString& resultsJson) {
  searchQuery_ = query;
  searchProvider_ = provider;
  searchJson_ = resultsJson;
}

void SidePanel::appendChatLine(const QString& who, const QString& text) {
  chatView_->appendPlainText("[" + who + "] " + text);
}

QString SidePanel::buildChatPromptWithContext(const QString& userMsg, const core::extract::ExtractedPage* page, const QString* searchJson) {
  QString prompt;
  prompt += "You are NovaBrowse local assistant. Rules:\n";
  prompt += "- Use only provided sources/context. If insufficient evidence, say so.\n";
  prompt += "- When citing, use format: (source: URL#block_id) when page context is provided.\n";
  prompt += "- Be concise.\n\n";

  if (page) {
    std::vector<std::pair<std::string,std::string>> blocks;
    blocks.reserve(page->blocks.size());
    for (const auto& b : page->blocks) blocks.push_back({b.id, b.text});
    auto rc = services::ai::RagComposer::fromBlocks(page->canonicalUrl, blocks, 7000);
    prompt += "PAGE_CONTEXT_URL: " + QString::fromStdString(page->canonicalUrl) + "\n";
    prompt += "PAGE_CONTEXT_BLOCKS:\n" + QString::fromStdString(rc.contextText) + "\n";
  }

  if (searchJson) {
    prompt += "SEARCH_RESULTS_JSON:\n" + *searchJson + "\n";
  }

  prompt += "\nUSER:\n" + userMsg + "\n";
  return prompt;
}

void SidePanel::refreshEntitiesFromPage(const core::extract::ExtractedPage& ep, const QString& url) {
  auto ents = detector_.detect(ep.fullText);
  QString list;
  for (const auto& e : ents) {
    list += QString::fromStdString(core::entities::toString(e.type)) + " • "
         + QString::fromStdString(e.name) + " • conf=" + QString::number(e.confidence, 'f', 2) + "\n";
  }
  entityList_->setPlainText(list.isEmpty() ? "No entities detected." : list);
  graph_->setEntities(ents, "Page");

  // Create a minimal DeepSearch profile for the top person-like entity, if any
  for (const auto& e : ents) {
    if (e.type == core::entities::EntityType::Person && e.confidence > 0.70) {
      auto profile = app_->deepsearch().buildProfileFromText(QString::fromStdString(e.name), url, QString::fromStdString(ep.fullText));
      app_->deepsearch().upsertEntity(profile);
      break;
    }
  }
}

void SidePanel::onAnalyzePage() {
  if (!activeTab_) return;

  QUrl url = activeTab_->view()->url();
  activeTab_->view()->page()->toHtml([=](const QString& html) {
    core::extract::ExtractedPage ep = extractor_.extract(html.toStdString(), url.toString().toStdString());

    appendChatLine("system", "Extracted " + QString::number((int)ep.blocks.size()) + " blocks. Building analysis prompt…");
    refreshEntitiesFromPage(ep, url.toString());

    QString userMsg = "Analyze the page. Provide: summary, key claims with block citations, uncertainty/bias notes, and an entity list.";
    QString prompt = buildChatPromptWithContext(userMsg, &ep, useSearchCtx_->isChecked() ? &searchJson_ : nullptr);

    app_->ollama().generate(QString::fromStdString(app_->config().ollamaModel()), prompt, [=](const QString& text, const nlohmann::json&) {
      appendChatLine("assistant", text);
    });
  });
}

void SidePanel::onOverviewFromSearch() {
  if (searchJson_.isEmpty()) {
    appendChatLine("system", "No search context available. Perform a search first.");
    return;
  }
  QString userMsg = "Create an overview of the topic from the search results. Cite sources by URL. If evidence is weak, say so.";
  QString prompt = buildChatPromptWithContext(userMsg, nullptr, &searchJson_);
  app_->ollama().generate(QString::fromStdString(app_->config().ollamaModel()), prompt, [=](const QString& text, const nlohmann::json&) {
    appendChatLine("assistant", text);
  });
}

void SidePanel::onAsk() {
  if (!activeTab_) return;
  QString msg = chatInput_->text().trimmed();
  if (msg.isEmpty()) return;
  chatInput_->clear();

  appendChatLine("user", msg);

  if (!usePageCtx_->isChecked() && !useSearchCtx_->isChecked()) {
    QString prompt = buildChatPromptWithContext(msg, nullptr, nullptr);
    app_->ollama().generate(QString::fromStdString(app_->config().ollamaModel()), prompt, [=](const QString& text, const nlohmann::json&) {
      appendChatLine("assistant", text);
    });
    return;
  }

  QUrl url = activeTab_->view()->url();
  if (usePageCtx_->isChecked()) {
    activeTab_->view()->page()->toHtml([=](const QString& html) {
      core::extract::ExtractedPage ep = extractor_.extract(html.toStdString(), url.toString().toStdString());
      QString prompt = buildChatPromptWithContext(msg, &ep, useSearchCtx_->isChecked() ? &searchJson_ : nullptr);
      app_->ollama().generate(QString::fromStdString(app_->config().ollamaModel()), prompt, [=](const QString& text, const nlohmann::json&) {
        appendChatLine("assistant", text);
      });
    });
  } else {
    QString prompt = buildChatPromptWithContext(msg, nullptr, &searchJson_);
    app_->ollama().generate(QString::fromStdString(app_->config().ollamaModel()), prompt, [=](const QString& text, const nlohmann::json&) {
      appendChatLine("assistant", text);
    });
  }
}

} // namespace ui
