/*
 * Process tab widget for Process Pinner.
 * Copyright (C) 2025  Danila Denisov aka hremman
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PROCESSTAB_H
#define PROCESSTAB_H

#include <QLabel>
#include <QWidget>
#include <QStatusBar>
#include <QTime>
#include <QTimer>
#include <QProcess>

#include "executabledto.h"


namespace Ui {
class ProcessTab;
}

class ProcessTab : public QWidget
{
    Q_OBJECT
public:
    enum class ExecutableStatus{
        BUILD,
        LAUNCH,
        RUN,
        STOP,
        STOPPED,
        FAULT,
        NEW
    };
    static QString statusToString(ExecutableStatus);
public:
    explicit ProcessTab(const ExecutableDTO * const dto, QWidget *parent = nullptr);
    ~ProcessTab();

    bool isStoped() const;
    const QProcess * process() {
        return &m_process;
    }
    bool startButtonEnabled() const;
    bool stopButtonEnabled() const;

signals:
    void stopped();
    void started();


public slots:
    void setScrollBtnTip();
    void clearLogs(bool);
    void dtoUpdated();

    void startApp();
    void stopApp();
    void stop();
    const QString& name() const{
        return m_dto->name;
    }


private:
    enum class LogLevel{
        PP_INFO,
        PP_WARN,
        PP_ERROR
    };

    void logMessage(LogLevel lvl, const QString & message);
    void append(const QString & message, bool isErr = false);
    void enableStartStop(bool start, bool stop);


    Ui::ProcessTab *ui;
    QStatusBar m_status_bar;
    QLabel m_status_lable;
    QColor m_info;
    QColor m_warning;
    QColor m_error;
    QColor m_std_err;
    bool m_last_message;
    const ExecutableDTO * const m_dto;
    QProcess m_process;
    QStringDecoder m_stderr_decoder;
    QStringDecoder m_stdout_decoder;
    ExecutableStatus m_status;
    QTimer m_terminate_timer;
    QMetaObject::Connection m_terminate_connection;
    bool m_user_termination;

    bool m_restart;
    bool m_build;
    qint64 m_pid;

private:
    void setStopTip();
    void setStartTip();
    void setRestartTip();
    void autoscroll();
    void appendDecoded(QStringDecoder & decoder, const QByteArray &raw, bool isErr = false);
    void setStatus(ExecutableStatus, const QString & tail = "");
    void setStatusPid(ExecutableStatus, int pid);
    void setStatusRetVal(ExecutableStatus, int retVal, int pid);
    void start();
    bool build();
    void ctrlC();

    bool notNormalFinished(QProcess::ExitStatus exitStatus, int exitCode);
    bool notFault();

private slots:
    void toggleAuthoscroll(bool);
    void stdout_();
    void stderr_();
    void onFault(QProcess::ProcessError error);
    void onStarted();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus = QProcess::ExitStatus::NormalExit);
    void terminateProcess();
    void killProcess();

    void searchNext(bool);
    void searchPrev(bool);
};

#endif // PROCESSTAB_H
