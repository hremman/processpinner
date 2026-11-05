#include <QWheelEvent>
#include <QScrollBar>
#include "qlistwidgetscrollone.h"

QListWidgetScrollOne::QListWidgetScrollOne(QWidget *parent)
    : QListWidget(parent)
{}

void QListWidgetScrollOne::wheelEvent(QWheelEvent *event) {
    int angleDeltaY = event->angleDelta().y();

    if (angleDeltaY != 0) {
        // Если pixelDelta не пустой — это тачпад, используем стандартное поведение
        if (!event->pixelDelta().isNull()) {
            QListWidget::wheelEvent(event);
        } else {
            // Это мышь — скроллим строго по 1 шагу (элементу)
            QScrollBar *vBar = this->verticalScrollBar();
            int step = vBar->singleStep();

            if (angleDeltaY > 0) {
                vBar->setValue(vBar->value() - step); // Вверх
            } else {
                vBar->setValue(vBar->value() + step); // Вниз
            }
            event->accept();
        }
    } else {
        QListWidget::wheelEvent(event);
    }
}
