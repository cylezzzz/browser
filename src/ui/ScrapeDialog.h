\
/* src/ui/ScrapeDialog.h */
#pragma once
#include <QDialog>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QUrl>

#include "services/scraper/ScraperService.h"

namespace ui {

class ScrapeDialog : public QDialog {
  Q_OBJECT
public:
  explicit ScrapeDialog(QWidget* parent = nullptr);

  void setPage(const QUrl& url, const QString& html);

private slots:
  void runScrape();
  void copyToClipboard();
  void saveToFile();

private:
  QUrl url_;
  QString html_;

  QComboBox* mode_;
  QPlainTextEdit* out_;
  QPushButton* run_;
  QPushButton* copy_;
  QPushButton* save_;

  services::scraper::ScraperService scraper_;

  services::scraper::ScrapeMode modeFromIndex(int idx) const;
};

} // namespace ui
