#ifndef NOTIFIER_H
#define NOTIFIER_H

#pragma once
#include <QObject>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QApplication>
#include <QStyle>
#include "../mainwindow.h"

class Notifier : public QObject {
    Q_OBJECT
public:
    static Notifier& instance(MainWindow* ptr = nullptr) {
        static Notifier inst(ptr);
        return inst;
    }

    void notify(const QString &title, const QString &message, bool warn = false);
    void hideNotify();
    static inline bool isAvalibale() {return QSystemTrayIcon::isSystemTrayAvailable();}

private:
    Notifier(MainWindow *mw = nullptr);
    QSystemTrayIcon *m_tray;
    MainWindow *m_mw;
    #ifdef Q_OS_LINUX
        static const auto infoIcon = QSystemTrayIcon::Warning;
        static const auto warnIcon = QSystemTrayIcon::Critical;
    #else
        static const auto infoIcon = QSystemTrayIcon::Information;
        static const auto warnIcon = QSystemTrayIcon::Warning;
    #endif
private slots:
    void restoreMainWindow();
};


#endif // NOTIFIER_H
