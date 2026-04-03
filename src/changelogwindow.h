#ifndef CHANGELOGWINDOW_H
#define CHANGELOGWINDOW_H

#include <QDialog>

namespace Ui {
class ChangelogWindow;
}

class ChangelogWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ChangelogWindow(const QString& changelogText, QWidget *parent = nullptr);
    ~ChangelogWindow();

private:
    Ui::ChangelogWindow *ui;
};

#endif // CHANGELOGWINDOW_H
