#ifndef HELPWINDOW_H
#define HELPWINDOW_H

#include <QWidget>

namespace Ui {
class HelpWindow;
}

class HelpWindow : public QWidget
{
    Q_OBJECT

public:
    explicit HelpWindow(
        const std::vector<std::pair<QKeySequence, QString>> & sks,
        QWidget *parent = nullptr
    );
    ~HelpWindow();

private:
    void prepareHotkeysTab(const std::vector<std::pair<QKeySequence, QString>> & sks);

private:
    Ui::HelpWindow *ui;

    const static int __M_KS_COL = 0;
    const static int __M_DEF_COL = 1;
};

#endif // HELPWINDOW_H
