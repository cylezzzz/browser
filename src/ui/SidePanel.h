\
/* src/ui/SidePanel.h */
#pragma once
#include <QWidget>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>

#include "app/NovaApp.h"
#include "ui/EntityGraphWidget.h"
#include "core/extract/HtmlExtractor.h"
#include "core/entities/EntityDetector.h"

namespace ui {

class BrowserTab;

class SidePanel : public QWidget {
  Q_OBJECT
public:
  explicit SidePanel(NovaApp* app, QWidget* parent = nullptr);

  void setActiveTab(BrowserTab* tab);
  void setSearchContext(const QString& query, const QString& provider, const QString& resultsJson);

private slots:
  void onAsk();
  void onAnalyzePage();
  void onOverviewFromSearch();

private:
  NovaApp* app_;
  BrowserTab* activeTab_;

  QTabWidget* tabs_;
  // Chat tab
  QPlainTextEdit* chatView_;
  QLineEdit* chatInput_;
  QPushButton* askBtn_;
  QPushButton* analyzeBtn_;
  QPushButton* overviewBtn_;
  QCheckBox* usePageCtx_;
  QCheckBox* useSearchCtx_;

  // Entities tab
  EntityGraphWidget* graph_;
  QPlainTextEdit* entityList_;
  QPushButton* deepSearchBtn_;

  // State
  QString searchQuery_;
  QString searchProvider_;
  QString searchJson_;

  core::extract::HtmlExtractor extractor_;
  core::entities::EntityDetector detector_;

  void appendChatLine(const QString& who, const QString& text);
  QString buildChatPromptWithContext(const QString& userMsg, const core::extract::ExtractedPage* page, const QString* searchJson);
  void refreshEntitiesFromPage(const core::extract::ExtractedPage& ep, const QString& url);
};

} // namespace ui
