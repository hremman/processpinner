#include <QGuiApplication>
#include <QIcon>
#include <QMenu>
#include <QAction>
#include "notifier.h"


Notifier::Notifier(MainWindow *mw)
    : m_tray(nullptr)
    , m_mw(mw)

{
    if (isAvalibale()) {
        m_tray = new QSystemTrayIcon(this);

        // ВАЖНО: Укажите иконку вашего приложения.
        // Если её нет, берем стандартную системную, иначе ОС может проигнорировать вызов.
        m_tray->setIcon(QIcon(":/logo-512.png"));

        // --- Создаем контекстное меню для иконки ---
        QMenu *trayMenu = new QMenu();

        QAction *restoreAction = trayMenu->addAction(tr("Restore"));
        connect(restoreAction, &QAction::triggered, this, &Notifier::restoreMainWindow);

        trayMenu->addSeparator();

        QAction *quitAction = trayMenu->addAction(tr("Exit"));
        // Важно: qApp->quit() закрывает приложение намертво, игнорируя closeEvent
        connect(quitAction, &QAction::triggered, this, [this](){
            if(this->m_mw){
                restoreMainWindow();
                if(m_mw->terminate()) QCoreApplication::quit();
            }
        });

        m_tray->setContextMenu(trayMenu);

        // --- Обработка клика по самой иконке (ЛКМ / Двойной клик) ---
        connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
                restoreMainWindow();
            }
        });

        m_tray->show();
    }
}

void Notifier::restoreMainWindow() {
    qDebug() << "Notifier::restoreMainWindow";
    if (!m_mw) qWarning() << "Notifier::restoreMainWindow: " << m_mw;
    else {
        m_mw->show();
        m_mw->raise();
        m_mw->activateWindow();
    }
}

void Notifier::notify(const QString &title, const QString &message, bool warn) {
    qDebug() << "notify: " << title;
    if (!m_tray || !QSystemTrayIcon::isSystemTrayAvailable()) return;
    if (!warn && (QGuiApplication::applicationState() == Qt::ApplicationActive)) return;

    // Включаем видимость перед отправкой (нужно для некоторых платформ)
    if (!m_tray->isVisible()) {
        m_tray->show();
    }

    // Отправляем системное уведомление
    m_tray->showMessage(title, message, warn ? warnIcon : infoIcon, 5000);
    //m_tray->hide();
}

void Notifier::hideNotify(){
    if (!m_tray || !QSystemTrayIcon::isSystemTrayAvailable()) return;
    m_tray->showMessage(tr("Minimized in tray"), "", QSystemTrayIcon::Information, 1000);
}
