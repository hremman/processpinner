#include "changelogwindow.h"
#include "ui_changelogwindow.h"
#include <QScreen>

ChangelogWindow::ChangelogWindow(const QString& changelogText, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChangelogWindow)
{
    ui->setupUi(this);
    qDebug() << "ChangelogWindow::ChangelogWindow(...)";
    setWindowFlags(Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    ui->text->setText(changelogText);
    layout()->activate();
    int textWidth = ui->text->document()->idealWidth() + 100;
    resize(
        std::min(textWidth, parent->screen()->geometry().width() - 20),
        parent->parentWidget()->sizeHint().height()
    );
}

ChangelogWindow::~ChangelogWindow()
{
    delete ui;
}
