#ifndef QLISTWIDGETSCROLLONE_H
#define QLISTWIDGETSCROLLONE_H

#include <QWidget>
#include <QListWidget>

class QListWidgetScrollOne : public QListWidget
{
    Q_OBJECT
public:
    QListWidgetScrollOne(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
};

#endif // QLISTWIDGETSCROLLONE_H
