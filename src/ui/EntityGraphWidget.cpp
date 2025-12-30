\
/* src/ui/EntityGraphWidget.cpp */
#include "ui/EntityGraphWidget.h"
#include "core/entities/EntityDetector.h"
#include <QVBoxLayout>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QtMath>

namespace ui {

EntityGraphWidget::EntityGraphWidget(QWidget* parent)
  : QWidget(parent),
    view_(new QGraphicsView(this)),
    scene_(new QGraphicsScene(this)) {

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(view_);

  view_->setScene(scene_);
  view_->setRenderHint(QPainter::Antialiasing, true);
  view_->setBackgroundBrush(QColor(15,17,21));
  view_->setFrameShape(QFrame::NoFrame);
}

void EntityGraphWidget::setEntities(const std::vector<core::entities::EntityMention>& entities, const QString& centerName) {
  scene_->clear();

  const QPointF center(0, 0);
  auto* centerItem = scene_->addEllipse(-26, -26, 52, 52, QPen(QColor(185,199,255)), QBrush(QColor(21,24,36)));
  Q_UNUSED(centerItem);

  auto* centerText = scene_->addText(centerName.isEmpty() ? "Page" : centerName);
  centerText->setDefaultTextColor(QColor(233,238,246));
  centerText->setPos(-centerText->boundingRect().width()/2.0, 34);

  int n = (int)entities.size();
  if (n == 0) {
    auto* t = scene_->addText("No entities detected (yet).");
    t->setDefaultTextColor(QColor(233,238,246));
    t->setPos(-t->boundingRect().width()/2.0, -t->boundingRect().height()/2.0);
    view_->fitInView(scene_->itemsBoundingRect().adjusted(-40,-40,40,40), Qt::KeepAspectRatio);
    return;
  }

  const double radius = 160.0;
  for (int i = 0; i < n && i < 14; ++i) {
    double ang = (2.0 * M_PI * i) / std::min(n, 14);
    QPointF p(center.x() + radius * std::cos(ang), center.y() + radius * std::sin(ang));

    QColor nodeColor(155, 170, 255);
    if (entities[i].type == core::entities::EntityType::Org) nodeColor = QColor(170, 255, 200);
    if (entities[i].type == core::entities::EntityType::Place) nodeColor = QColor(255, 210, 170);

    scene_->addLine(QLineF(center, p), QPen(QColor(255,255,255,70)));

    scene_->addEllipse(p.x()-18, p.y()-18, 36, 36, QPen(nodeColor), QBrush(QColor(21,24,36)));
    QString label = QString::fromStdString(entities[i].name);
    if (label.size() > 22) label = label.left(22) + "…";
    auto* txt = scene_->addText(label);
    txt->setDefaultTextColor(QColor(233,238,246));
    txt->setPos(p.x() - txt->boundingRect().width()/2.0, p.y() + 22);
  }

  view_->fitInView(scene_->itemsBoundingRect().adjusted(-40,-40,40,40), Qt::KeepAspectRatio);
}

} // namespace ui
