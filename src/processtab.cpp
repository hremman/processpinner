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

#include "processtab.h"
#include "globals.hpp"
#include "ui_processtab.h"
#include "tools.h"

#include <QString>
#include <QTimer>
#include <QScrollBar>
#include <QStringConverter>
#include <QStyle>
#include <QFileInfo>
#include <QFontDatabase>
#include <QDir>

#if defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
#include <signal.h>
#include <unistd.h>
#else
#include <windows.h>
#endif


ProcessTab::ProcessTab(const ExecutableDTO * const dto, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ProcessTab)
    , m_status_bar(this)
    , m_status_lable(tr("Unknown status"))
    , m_info(0x00, 0xB0, 0x12)
    , m_warning(0xE0, 0x9D, 0x00)
    , m_error(0xCC, 0x12, 0x12)
    , m_std_err(0xE0, 0x68, 0x1D)
    , m_last_message(false)
    , m_dto(dto)
    , m_stderr_decoder(QStringDecoder::Utf8)
    , m_stdout_decoder(QStringDecoder::Utf8)
    , m_user_termination(false)
    , m_restart(false)
    , m_build(false)
    , m_pid(0)
{
    ui->setupUi(this);
    m_status_bar.addWidget(&m_status_lable);
    setScrollBtnTip();
    setStopTip();
    setStartTip();
    m_status_lable.setObjectName("statusIndicator");
    ui->tab_logs->setObjectName("tab_logs");
    layout()->addWidget(&m_status_bar);
#if defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
    m_process.setChildProcessModifier([]() {
        setpgid(0, 0);
    });
#else
    m_process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
    args->flags &= ~CREATE_NO_WINDOW;
    args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
    args->startupInfo->wShowWindow = SW_HIDE;
    });
#endif

    ui->search_warn->setProperty("status", "error");
    ui->search_warn->setHidden(true);

    connect(ui->tab_autoscroll_btn, &QToolButton::clicked, this, &ProcessTab::toggleAuthoscroll);
    connect(ui->clear_logs_btn, &QToolButton::clicked, this, &ProcessTab::clearLogs);

    connect(ui->start_btn, &QToolButton::clicked, this, [this](bool){startApp();});
    connect(ui->stop_btn, &QToolButton::clicked, this, [this](bool){stopApp();});
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &ProcessTab::stdout_);
    connect(&m_process, &QProcess::readyReadStandardError, this, &ProcessTab::stderr_);
    connect(&m_process, &QProcess::errorOccurred, this, &ProcessTab::onFault);
    connect(&m_process, &QProcess::started, this, &ProcessTab::onStarted);
    connect(&m_process, &QProcess::finished, this, &ProcessTab::onFinished);
    connect(ui->search_btn_n, &QToolButton::clicked, this, &ProcessTab::searchNext);
    connect(ui->search_btn_p, &QToolButton::clicked, this, &ProcessTab::searchPrev);
    connect(ui->search_edt, &QLineEdit::textChanged, this, [this](){
        ui->search_warn->setHidden(true);
    });

    QAction *search_btn_n_a = new QAction(this);
    search_btn_n_a->setShortcut(QKeySequence("F3"));
    search_btn_n_a->setShortcutContext(Qt::WindowShortcut);
    connect(search_btn_n_a, &QAction::triggered, ui->search_btn_n, &QToolButton::click);
    this->addAction(search_btn_n_a);
    tools::buildToolTip(tr("Find next entry"), ui->search_btn_n, search_btn_n_a);

    QAction *search_btn_p_a = new QAction(this);
    search_btn_p_a->setShortcut(QKeySequence("Shift+F3"));
    search_btn_p_a->setShortcutContext(Qt::WindowShortcut);
    connect(search_btn_p_a, &QAction::triggered, ui->search_btn_p, &QToolButton::click);
    this->addAction(search_btn_p_a);
    tools::buildToolTip(tr("Find prevous entry"), ui->search_btn_p, search_btn_p_a);

    QAction *start_btn_a = new QAction(this);
    start_btn_a->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F5));
    start_btn_a->setShortcutContext(Qt::WindowShortcut);
    connect(start_btn_a, &QAction::triggered, ui->start_btn, &QToolButton::click);
    start_btn_a->setToolTip(tr("Start/restart application in current tab"));
    this->addAction(start_btn_a);


    QAction *stop_btn_a = new QAction(this);
    stop_btn_a->setShortcut(QKeySequence("Ctrl+F6"));
    stop_btn_a->setShortcutContext(Qt::WindowShortcut);
    connect(stop_btn_a, &QAction::triggered, ui->stop_btn, &QToolButton::click);
    stop_btn_a->setToolTip(tr("Stop application in current tab"));
    this->addAction(stop_btn_a);


    QAction *clear_logs_btn_a = new QAction(this);
    clear_logs_btn_a->setShortcut(QKeySequence("Ctrl+Shift+C"));
    clear_logs_btn_a->setShortcutContext(Qt::WindowShortcut);
    connect(clear_logs_btn_a, &QAction::triggered, ui->clear_logs_btn, &QToolButton::click);
    this->addAction(clear_logs_btn_a);
    tools::buildToolTip(tr("Clear output window"),  ui->clear_logs_btn, clear_logs_btn_a);


    QAction *autoscroll_btn_a = new QAction(this);
    autoscroll_btn_a->setShortcut(QKeySequence("Ctrl+A"));
    autoscroll_btn_a->setShortcutContext(Qt::WindowShortcut);
    connect(autoscroll_btn_a, &QAction::triggered, ui->tab_autoscroll_btn, &QToolButton::click);
    autoscroll_btn_a->setToolTip(tr("Autoscroll output on/off"));
    this->addAction(autoscroll_btn_a);


    QAction *search_btn_h_a = new QAction(this);
    search_btn_h_a->setShortcut(QKeySequence("Ctrl+F"));
    search_btn_h_a->setShortcutContext(Qt::WindowShortcut);
    connect(search_btn_h_a, &QAction::triggered, ui->tab_autoscroll_btn, [this](){
        ui->search_frm->setHidden(false);
        ui->search_edt->setFocus();
    });
    this->addAction(search_btn_h_a);
    search_btn_h_a->setToolTip(tr("Find in logs"));

    connect(ui->search_edt, &QLineEdit::returnPressed, this, [this](){searchNext(true);});

    connect(ui->search_btn_h, &QToolButton::clicked, this, [this](){
        ui->search_frm->setHidden(true);
    });

    ui->search_frm->setHidden(true);

    m_terminate_timer.setSingleShot(true);
    setStatus(ExecutableStatus::NEW);
    enableStartStop(true, false);
    ui->tab_logs->document()->setMaximumBlockCount(2000);
    qInfo() << "    ProcessTab \"" << m_dto->name << "\" inited";
}

ProcessTab::~ProcessTab()
{
    delete ui;
}

void ProcessTab::ctrlC(){
    qDebug() << "void ProcessTab::ctrlC()" << "<" << m_dto->name << ">";
#if defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
    kill(- m_process.processId(), SIGINT);
#else
    if (AttachConsole(static_cast<DWORD>(m_pid))) {
        SetConsoleCtrlHandler(NULL, TRUE);
        if(!GenerateConsoleCtrlEvent(CTRL_C_EVENT, static_cast<DWORD>(m_pid)))
            qDebug() << "    failed to send Ctrl+C event" ;
        FreeConsole();
        SetConsoleCtrlHandler(NULL, FALSE);
    } else {
        qDebug() << "can not send Ctrl+C event becose can not attach";
    }
#endif
}

void ProcessTab::logMessage(LogLevel lvl, const QString &message) {
    qDebug() << "void ProcessTab::logMessage(Level lvl, const QString &message)" << "<" << m_dto->name << ">";
    QColor color;
    switch (lvl) {
        case LogLevel::PP_INFO:
            color = m_info;
            qInfo() << "    " << message;
            break;
        case LogLevel::PP_WARN:
            color = m_warning;
            qWarning() << "    " << message;
            break;
        case LogLevel::PP_ERROR:
            color = m_error;
            qCritical() << "    " << message;
            break;
    }
    QTextCursor cursor = ui->tab_logs->textCursor();
    cursor.movePosition(QTextCursor::End);
    if(m_last_message)
        cursor.insertBlock();
    QTextCharFormat format;
    format.setForeground(color);
    cursor.insertText(message, format);
    m_last_message = true;
    autoscroll();
}

void ProcessTab::append(const QString &text, bool isErr) {
    QTextCursor cursor = ui->tab_logs->textCursor();
    cursor.movePosition(QTextCursor::End);
    if(m_last_message)
        cursor.insertBlock();
    if (isErr){
        QTextCharFormat format;
        format.setForeground(m_std_err);
        cursor.insertText(text, format);
    } else {
        cursor.insertText(text, QTextCharFormat());
    }
    m_last_message = false;
    autoscroll();
}

void ProcessTab::setScrollBtnTip() {
    ui->tab_autoscroll_btn->isChecked();
    ui->tab_autoscroll_btn->setToolTip(
        QString("%1 [%2]")
            .arg(tr("Autoscroll output (Ctrl+A)"))
            .arg(ui->tab_autoscroll_btn->isChecked()? tr("disabled") : tr("enabled"))
    );
}

void ProcessTab::setStopTip() {
    ui->stop_btn->setToolTip(tr("Stop application: \"%1\" (Ctrl+F6)").arg(m_dto->name));
}

void ProcessTab::setStartTip() {
    ui->start_btn->setToolTip(tr("Start/restart application: \"%1\" (Ctrl+F5)").arg(m_dto->name));
}

void ProcessTab::toggleAuthoscroll(bool){
    qDebug() << "void ProcessTab::toggleAuthoscroll(bool)" << "<" << m_dto->name << ">";
    setScrollBtnTip();
    autoscroll();
}

void ProcessTab::autoscroll() {
    if (!ui->tab_autoscroll_btn->isChecked())
        ui->tab_logs->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
}

void ProcessTab::clearLogs(bool){
    qDebug() << "void ProcessTab::clearLogs(bool)" << "<" << m_dto->name << ">";
    ui->tab_logs->clear();
}

void ProcessTab::dtoUpdated(){
    setStopTip();
    setStartTip();
}

bool ProcessTab::isStoped() const {
    return m_process.state() == QProcess::ProcessState::NotRunning;
}


void ProcessTab::startApp(){
    qDebug() << "void ProcessTab::startApp()" << "<" << m_dto->name << ">";
    m_user_termination = false;
    clearLogs(false);
    if (m_process.state() != QProcess::ProcessState::NotRunning) {
        m_restart = true;
        stopApp();
        return;
    }
    if (!build())
        start();
}

void ProcessTab::start(){
    qDebug() << "void ProcessTab::start()" << "<" << m_dto->name << ">";
    qInfo() << "    start " << m_dto->name;
    clearLogs(false);
    m_build = false;
    logMessage(LogLevel::PP_INFO, tr("Launch..."));
    QStringList app_splited = QProcess::splitCommand(m_dto->application);
    QStringList app_args = app_splited.sliced(1, app_splited.size() - 1);
    if (!m_dto->args.isEmpty())
        app_args += m_dto->args;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!m_dto->env.isEmpty())
        for (QMap<QString, QString>::ConstIterator pair = m_dto->env.constBegin(); pair != m_dto->env.constEnd(); ++pair )
            env.insert(pair.key(), pair.value());

    QString fullApplication = tools::buildAbsolutePath(app_splited[0], m_dto->project_directory, m_dto->work_directory);
    if (!m_dto->work_directory.isEmpty())
        m_process.setWorkingDirectory(m_dto->work_directory);
    else{
        if (!m_dto->project_directory.isEmpty())
            m_process.setWorkingDirectory(m_dto->project_directory);
        else {
            if(app_splited[0].contains(QDir::separator())){
                QString pwd = QFileInfo(fullApplication).absolutePath();
                m_process.setWorkingDirectory(pwd);
            }
        }
    }
    m_process.setArguments(app_args);
    m_process.setProcessEnvironment(env);
    m_process.setProgram(fullApplication);
    setStatus(ExecutableStatus::LAUNCH);
    enableStartStop(false, false);
    m_process.start();
}

bool ProcessTab::build(){
    qDebug() << "bool ProcessTab::build()" << "<" << m_dto->name << ">";
    qInfo()  << "    build " << m_dto->name;
    QStringList build_cmd_args = QProcess::splitCommand(m_dto->build_cmd);
    if (m_dto->project_directory.isEmpty() || m_dto->build_cmd.isEmpty())
        return false;
    QFileInfo executable_file(tools::buildAbsolutePath(build_cmd_args[0], m_dto->project_directory, m_dto->work_directory));
    QStringList app_cmd_args = QProcess::splitCommand(m_dto->application);
    QStringList app_args = app_cmd_args.sliced(1) + m_dto->args;
    if (app_cmd_args[0] == "java"){
        qDebug() << "    It's java. Try find jar-file";
        int i = 0;
        for(; i < app_args.size() && app_args[i] == "-jar"; ++i);
        if(i < app_args.size() ){
            QFileInfo app_file(tools::buildAbsolutePath(app_args[i], m_dto->project_directory,""));
            if(app_file.exists() && !m_dto->build_always)
                return false;
        }
    } else {
        QFileInfo app_file(tools::buildAbsolutePath(app_cmd_args[0], m_dto->project_directory, m_dto->work_directory));
        if(app_file.exists() && !m_dto->build_always)
            return false;
    }



    QStringList build_args = build_cmd_args.sliced(1, build_cmd_args.size() - 1);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    m_process.setWorkingDirectory(m_dto->project_directory);
    m_process.setArguments(build_args);
    m_process.setProcessEnvironment(env);
    m_process.setProgram(tools::buildAbsolutePath(build_cmd_args[0], m_dto->project_directory, ""));

    setStatus(ExecutableStatus::BUILD);
    enableStartStop(false, false);
    m_build = true;
    m_process.start();

    return true;
}

void ProcessTab::stopApp(){
    qDebug() << "void ProcessTab::stopApp()" << "<" << m_dto->name << ">";
    if (m_process.state() == QProcess::ProcessState::NotRunning)
        return;
    stop();

    m_terminate_connection = connect(&m_terminate_timer, &QTimer::timeout, this, &ProcessTab::terminateProcess);
    setStatusPid(ExecutableStatus::STOP, m_process.processId());
    enableStartStop(false, false);
    m_terminate_timer.start(10000);
}


void ProcessTab::stop(){
    qDebug() << "void ProcessTab::stop()" << "<" << m_dto->name << ">";
    qDebug() << "    stop " << m_dto->name << " [" << m_pid << "]";
    if (m_process.state() == QProcess::ProcessState::NotRunning)
        return;
    ctrlC();
    m_user_termination = true;
}

void ProcessTab::stdout_(){
    appendDecoded(m_stdout_decoder, m_process.readAllStandardOutput());
}

void ProcessTab::stderr_(){
    appendDecoded(m_stderr_decoder, m_process.readAllStandardError(), true);
}

void ProcessTab::appendDecoded(QStringDecoder & decoder, const QByteArray &raw, bool isErr){
    QString output = decoder(raw);
    append(output, isErr);
}

QString ProcessTab::statusToString(ExecutableStatus value){
    switch(value){
        case ExecutableStatus::BUILD   : return tr("Build"); break;
        case ExecutableStatus::LAUNCH  : return tr("Launch"); break;
        case ExecutableStatus::RUN     : return tr("Run"); break;
        case ExecutableStatus::STOP    : return tr("Stop"); break;
        case ExecutableStatus::STOPPED : return tr("Stopped"); break;
        case ExecutableStatus::FAULT   : return tr("Fault"); break;
        case ExecutableStatus::NEW     : return tr("New"); break;
    }
        return "";
}

void ProcessTab::setStatus(ExecutableStatus status, const QString & tail){
    qDebug() << "void ProcessTab::setStatus(ExecutableStatus status, const QString & tail)" << "<" << m_dto->name << ">";
    m_status_lable.setText(statusToString(status) + tail);
    QString color;
    switch(status){
        case ExecutableStatus::BUILD   : color ="build"; break;
        case ExecutableStatus::LAUNCH  : color ="starting"; break;
        case ExecutableStatus::RUN     : color ="running"; break;
        case ExecutableStatus::STOP    : color ="stop"; break;
        case ExecutableStatus::STOPPED : color ="stopped"; break;
        case ExecutableStatus::FAULT   : color ="error"; break;
        case ExecutableStatus::NEW     : color ="stopped"; break;
    }

    m_status_lable.setProperty("status", color);
    m_status_lable.style()->unpolish(&m_status_lable);
    m_status_lable.style()->polish(&m_status_lable);
    m_status_lable.update();
}

void ProcessTab::setStatusPid(ExecutableStatus status, int pid){
    qDebug() << "void ProcessTab::setStatusPid(ExecutableStatus status, int pid)" << "<" << m_dto->name << ">";
    setStatus(status, QString(" [%1]").arg(pid));
}
void ProcessTab::setStatusRetVal(ExecutableStatus status, int retVal, int pid){
    qDebug() << "void ProcessTab::setStatusRetVal(ExecutableStatus status, int retVal, int pid)" << "<" << m_dto->name << ">";
    setStatus(status, QString(" [%1](%2)").arg(pid).arg(retVal));
}

void ProcessTab::enableStartStop(bool enableStart, bool enableStop){
    qDebug() << "void ProcessTab::enableStartStop(bool enableStart, bool enableStop)" <<enableStart << enableStop << "<" << m_dto->name << ">";
    ui->start_btn->setEnabled(enableStart);
    ui->stop_btn->setEnabled(enableStop);
}

void ProcessTab::onFault(QProcess::ProcessError error){
    qDebug() << "void ProcessTab::onFault(QProcess::ProcessError error)" << "<" << m_dto->name << ">";
#ifdef Q_OS_WIN
    bool isFault = QProcess::ProcessError::Crashed == error && (m_user_termination && (130 == m_process.exitCode() || -1073741510 == m_process.exitCode()));
    if (isFault){
        return;
    }
#endif
    QString msg;
    switch(error){
        case QProcess::ProcessError::FailedToStart: msg = tr("Failed to start application."); break;
        case QProcess::ProcessError::Crashed: msg = tr("Application crashed."); break;
        case QProcess::ProcessError::ReadError: msg = tr("Read process output error."); break;
        case QProcess::ProcessError::UnknownError: msg = tr("An unknown error occurred."); break;
        default: return;
    }
    logMessage(LogLevel::PP_ERROR, tr("Error type: ") + msg);
    logMessage(LogLevel::PP_ERROR, tr("Error description: ") + m_process.errorString());
    if (error != QProcess::ProcessError::FailedToStart){
        logMessage(LogLevel::PP_ERROR, tr("Exit code: %1").arg(m_process.exitCode()));
        setStatusRetVal(ExecutableStatus::FAULT, m_process.exitCode(), m_pid);
    } else{
        setStatus(ExecutableStatus::FAULT);
    }
    emit statusAlert();
    enableStartStop(true, false);
}

void ProcessTab::onStarted(){
    qDebug() << "void ProcessTab::onStarted()" << "<" << m_dto->name << ">";
    if (!m_build) logMessage(LogLevel::PP_INFO, tr("Started"));
    ExecutableStatus status = m_build ? ExecutableStatus::BUILD : ExecutableStatus::RUN;
    m_pid = m_process.processId();
    setStatusPid(status, m_pid);
    enableStartStop(true, true);
    if (!m_build) emit statusRun();
    else emit statusBuild();
}

bool ProcessTab::notNormalFinished(QProcess::ExitStatus exitStatus, int exitCode){
#if defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
    return (!m_user_termination && QProcess::ExitStatus::NormalExit == exitStatus && exitCode) || (QProcess::ExitStatus::NormalExit != exitStatus && !(m_user_termination && (m_user_termination && 130 == exitCode)));
#else
    return (!m_user_termination && QProcess::ExitStatus::NormalExit == exitStatus && exitCode) || (QProcess::ExitStatus::NormalExit != exitStatus && !(m_user_termination && (130 == exitCode || -1073741510 == exitCode)));
#endif
}


void ProcessTab::onFinished(int exitCode, QProcess::ExitStatus exitStatus){
    qDebug() << "void ProcessTab::onFinished(int exitCode, QProcess::ExitStatus exitStatus)" << "<" << m_dto->name << ">";
    disconnect(m_terminate_connection);
    m_terminate_timer.stop();

    LogLevel level = LogLevel::PP_INFO;
    ExecutableStatus status = ExecutableStatus::STOPPED;
    bool isFault = notNormalFinished(exitStatus, exitCode);
    if (isFault){
        emit statusAlert();
        level = LogLevel::PP_ERROR;
        status = ExecutableStatus::FAULT;
    } else
        emit statusStopped();

    stdout_();
    stderr_();

    logMessage(level, tr("Stopped"));
    logMessage(level, tr("Exit code: %1").arg(exitCode));
    setStatusRetVal(status, exitCode, m_pid);

    enableStartStop(true, false);
    if (m_restart){
        m_user_termination = false;
        m_restart = false;
        startApp();
        return;
    }
    if (m_build && !m_user_termination && status != ExecutableStatus::FAULT){
        m_user_termination = false;
        m_build = false;
        start();
        return;
    }
    m_user_termination = false;
    m_restart = false;
    m_build = false;
}


void ProcessTab::terminateProcess(){
    qDebug() << "void ProcessTab::terminateProcess()" << "<" << m_dto->name << ">";
    disconnect(m_terminate_connection);
    m_process.terminate();
    m_terminate_connection = connect(&m_terminate_timer, &QTimer::timeout, this, &ProcessTab::killProcess);
    m_terminate_timer.start(10000);
}
void ProcessTab::killProcess(){
    qDebug() << "void ProcessTab::killProcess()" << "<" << m_dto->name << ">";
    disconnect(m_terminate_connection);
    m_process.kill();
}

void ProcessTab::searchNext(bool){
    qDebug() << "void ProcessTab::searchNext(bool)" << "<" << m_dto->name << ">";
    QString target = ui->search_edt->text();
    if (target.isEmpty()) return;
    ui->tab_autoscroll_btn->setChecked(true);
    toggleAuthoscroll(false);
    if (!ui->tab_logs->find(target)) {
        ui->tab_logs->moveCursor(QTextCursor::Start);
        if(!ui->tab_logs->find(target))
            ui->search_warn->setHidden(false);
    }
}
void ProcessTab::searchPrev(bool){
    qDebug() << "void ProcessTab::searchPrev(bool)" << "<" << m_dto->name << ">";
    QString target = ui->search_edt->text();
    if (target.isEmpty()) return;
    ui->tab_autoscroll_btn->setChecked(true);
    toggleAuthoscroll(false);
    if (!ui->tab_logs->find(target, QTextDocument::FindBackward)) {
        ui->tab_logs->moveCursor(QTextCursor::End);
        if(!ui->tab_logs->find(target, QTextDocument::FindBackward))
            ui->search_warn->setHidden(false);
    }
}

bool ProcessTab::startButtonEnabled() const {
    return ui->start_btn->isEnabled();
}

bool ProcessTab::stopButtonEnabled() const {
    return ui->stop_btn->isEnabled();
}
