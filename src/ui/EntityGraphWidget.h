\
/* src/ui/EntityGraphWidget.h */
#pragma once
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <vector>

#include "core/entities/EntityDetector.h"

namespace ui {

class EntityGraphWidget : public QWidget {
  Q_OBJECT
public:
  explicit EntityGraphWidget(QWidget* parent = nullptr);

  void setEntities(const std::vector<core::entities::EntityMention>& entities, const QString& centerName = QString());

private:
  QGraphicsView* view_;
  QGraphicsScene* scene_;
};

} // namespace ui
