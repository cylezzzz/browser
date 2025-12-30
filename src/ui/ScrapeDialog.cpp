\
/* src/ui/ScrapeDialog.cpp */
#include "ui/ScrapeDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QFile>
#include <QTextStream>

namespace ui {

ScrapeDialog::ScrapeDialog(QWidget* parent)
  : QDialog(parent),
    mode_(new QComboBox(this)),
    out_(new QPlainTextEdit(this)),
    run_(new QPushButton("Run", this)),
    copy_(new QPushButton("Copy", this)),
    save_(new QPushButton("Save…", this)) {

  setWindowTitle("Scraper Toolbelt");
  resize(860, 620);

  mode_->addItem("Reader (Markdown)");
  mode_->addItem("Full Text");
  mode_->addItem("Metadata (JSON)");
  mode_->addItem("Links");
  mode_->addItem("Headings (MVP)");
  mode_->addItem("Blocks (JSON)");
  mode_->addItem("Tables -> JSON (stub)");
  mode_->addItem("JSON-LD (stub)");

  out_->setReadOnly(true);
  out_->setPlaceholderText("Output…");

  auto* top = new QHBoxLayout();
  top->addWidget(mode_, 1);
  top->addWidget(run_);
  top->addWidget(copy_);
  top->addWidget(save_);

  auto* layout = new QVBoxLayout(this);
  layout->addLayout(top);
  layout->addWidget(out_, 1);

  connect(run_, &QPushButton::clicked, this, &ScrapeDialog::runScrape);
  connect(copy_, &QPushButton::clicked, this, &ScrapeDialog::copyToClipboard);
  connect(save_, &QPushButton::clicked, this, &ScrapeDialog::saveToFile);
}

void ScrapeDialog::setPage(const QUrl& url, const QString& html) {
  url_ = url;
  html_ = html;
  runScrape();
}

services::scraper::ScrapeMode ScrapeDialog::modeFromIndex(int idx) const {
  using services::scraper::ScrapeMode;
  switch (idx) {
    case 0: return ScrapeMode::Reader;
    case 1: return ScrapeMode::FullText;
    case 2: return ScrapeMode::Metadata;
    case 3: return ScrapeMode::Links;
    case 4: return ScrapeMode::Headings;
    case 5: return ScrapeMode::BlocksJson;
    case 6: return ScrapeMode::TablesStub;
    case 7: return ScrapeMode::JsonLdStub;
    default: return ScrapeMode::Reader;
  }
}

void ScrapeDialog::runScrape() {
  auto mode = modeFromIndex(mode_->currentIndex());
  auto result = scraper_.run(mode, url_, html_);
  out_->setPlainText(result.text);
}

void ScrapeDialog::copyToClipboard() {
  QApplication::clipboard()->setText(out_->toPlainText());
}

void ScrapeDialog::saveToFile() {
  QString suggested = "export.txt";
  if (mode_->currentIndex() == 0) suggested = "reader.md";
  if (mode_->currentIndex() == 2 || mode_->currentIndex() >= 5) suggested = "export.json";

  QString path = QFileDialog::getSaveFileName(this, "Save export", suggested);
  if (path.isEmpty()) return;

  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return;
  QTextStream ts(&f);
  ts.setEncoding(QStringConverter::Utf8);
  ts << out_->toPlainText();
  f.close();
}

} // namespace ui
