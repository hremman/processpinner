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


    void changelogError(const QString &caused);

    static const char * const __M_bullit;
    static const char * const __M_delimeter;
    static QString __M_thnx_msg;
    constexpr static const char* __M_changelog_path =
#if defined(Q_OS_LINUX)
        "/home/ddenisov/Repo/qt/ProcessPinner/changelog"
#elif defined(Q_OS_WINDOWS)
        "chagelog"
#elif defined(Q_OS_MACOS)
        "chagelog"
#else
        nullptr;
#endif
        ;
};

#endif // ABOUTDIALOG_H
