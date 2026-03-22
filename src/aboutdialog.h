#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QResizeEvent>
#include <QPair>
#include <QList>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog();

private:
    Ui::AboutDialog *ui;

    static const char * const __M_bullit;
    static const char * const __M_delimeter;
    static QString __M_thnx_msg;
};

#endif // ABOUTDIALOG_H
