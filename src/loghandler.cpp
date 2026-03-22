/*
 * File logger for Process Pinner.
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

#include "loghandler.h"
#include "globals.hpp"
#include <QDateTime>
#include <QDir>
#include <QStringBuilder>
#include <map>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <iostream>

LogStreamer::LogStreamer(std::FILE* descriptor, bool autoflush )
    : m_descriptor(descriptor)
    , m_closable(m_descriptor != stdin && m_descriptor != stderr && m_descriptor != stdout )
    , m_autoflush(autoflush)
{}

LogStreamer::~LogStreamer(){
    close();
}
void LogStreamer::close(){
    if(m_descriptor && m_closable){
        flush();
        fclose(m_descriptor);
    }
}

void LogStreamer::flush(){
    if(m_descriptor) std::fflush(m_descriptor);
}

void LogStreamer::autoflush(){
    if(m_autoflush) flush();
}

LogStreamer & LogStreamer::operator << (const std::string & msg){
    if (!m_descriptor) return *this;
    std::fprintf(m_descriptor, "%.*s", static_cast<int>(msg.size()), msg.data());
    autoflush();
    return *this;
}

LogStreamer & LogStreamer::operator << (const std::string_view & msg){
    if (!m_descriptor) return *this;
    std::fprintf(m_descriptor, "%.*s", static_cast<int>(msg.size()), msg.data());
    autoflush();
    return *this;
}

LogStreamer & LogStreamer::operator << (const char * msg){
    if (!m_descriptor) return *this;
    std::fprintf(m_descriptor, "%s", msg);
    autoflush();
    return *this;
}

void LogStreamer::emergeceWrite(const char * msg){
    if (!m_descriptor) return;
    operator <<(msg);
    flush();
}

bool LogHandler::__M_inited = false;
LogHandler* LogHandler::__M_singleton_ptr = nullptr;

const char * LogHandler::levels[] = {
    "Debug   ",
    "Info    ",
    "Warning ",
    "Critical",
    "Fatal   ",
};

std::map<std::string, LogHandler::LogLevel> levelMap = {
    {"debug",    LogHandler::LogLevel::DEBUG},
    {"info",     LogHandler::LogLevel::INFO},
    {"warning",  LogHandler::LogLevel::WARNING},
    {"critical", LogHandler::LogLevel::CRITICAL},
    {"fatal",    LogHandler::LogLevel::FATAL},
};

void LogHandler::init(){
    if (__M_inited) return;
    LogStreamer sstdout(stdout);
    LogLevel level = LogLevel::INFO;
    std::string l = Global::Vars::RC[Global::Consts::KEY_LOG_LEVEL].toLower().toStdString();
    if (levelMap.count(l))
        level = levelMap[l];
    FILE *file = std::fopen(Global::Vars::RC[Global::Consts::KEY_LOG_PATH].toStdString().data(), "w");
    if(file){
        __M_singleton_ptr = new LogHandler(level, file);
    } else {
        char reason[256] = {0};
        const char * err_msg = strerror(errno);
        std::strncpy(reason, strerror(errno), std::min(255, static_cast<int>(strlen(err_msg))));
        sstdout << "can not open log: " << reason;
        __M_singleton_ptr = new LogHandler(level, nullptr);
    }
    qInstallMessageHandler(LogHandler::messageHandler);
    std::signal(SIGSEGV, emergenceMsg);
    std::signal(SIGABRT, emergenceMsg);
    std::signal(SIGFPE,  emergenceMsg);
    std::signal(SIGILL,  emergenceMsg);
#ifdef Q_OS_WIN64
    SetUnhandledExceptionFilter(windowsExceptionFilter);
#endif
    __M_inited = true;
}
void LogHandler::messageHandler(QtMsgType type, [[maybe_unused]] const QMessageLogContext &context, const QString &msg){
    if (!__M_inited) return;
    LogLevel level = LogLevel::DEBUG;
    switch (type) {
        case QtMsgType::QtDebugMsg: level=LogLevel::DEBUG; break;
        case QtMsgType::QtInfoMsg: level=LogLevel::INFO; break;
        case QtMsgType::QtWarningMsg: level=LogLevel::WARNING; break;
        case QtMsgType::QtCriticalMsg: level=LogLevel::CRITICAL; break;
        case QtMsgType::QtFatalMsg: level=LogLevel::FATAL; break;
    }
    __M_singleton_ptr->logMessage(level, msg);
}

const char* getSignalName(int sig) {
    switch (sig) {
    case SIGSEGV: return "Segmentation Fault (SIGSEGV)";
    case SIGABRT: return "Aborted (SIGABRT)";
    case SIGFPE:  return "Arithmetic Error (SIGFPE)";
    case SIGILL:  return "Illegal Instruction (SIGILL)";
    case SIGINT:  return "Terminal Interrupt (SIGINT)";
    case SIGTERM: return "Termination Request (SIGTERM)";
    default:      return "Unknown Signal";
    }
}

#ifdef Q_OS_WIN64
LONG WINAPI LogHandler::windowsExceptionFilter(struct _EXCEPTION_POINTERS* info) {
    char msg[64];
    std::snprintf(msg, sizeof(msg), "EMERGENCY SHUTDOWN: WinException 0x%lx\n", info->ExceptionRecord->ExceptionCode);
    __M_singleton_ptr->m_file_streamer.emergeceWrite(msg);
    __M_singleton_ptr->m_file_streamer.close();
    __M_singleton_ptr->m_stdout_streamer.emergeceWrite(msg);
    __M_singleton_ptr->m_stdout_streamer.close();
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void LogHandler::emergenceMsg(int sig){
    if (!__M_inited) return;
    char msg[128] = {0};
    std::snprintf(msg, sizeof(msg), "EMERGENCY SHUTDOWN: %s\n", getSignalName(sig));
    __M_singleton_ptr->m_file_streamer.emergeceWrite(msg);
    __M_singleton_ptr->m_file_streamer.close();
    __M_singleton_ptr->m_stdout_streamer.emergeceWrite(msg);
    __M_singleton_ptr->m_stdout_streamer.close();
    _exit(sig);
}

void LogHandler::logMessage(LogHandler::LogLevel level, const QString& msg){
    if (m_level <= level){
        QMutexLocker locker(&m_mutex);
        QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QString message = time % " | " % levels[static_cast<int>(level)] % " | " % msg;
        m_file_streamer << message.toStdString() << "\n";
        m_stdout_streamer << message.toStdString() << "\n";
        if (LogLevel::FATAL == level){
            m_file_streamer.flush();
            m_stdout_streamer.flush();
        }
    }
}

LogHandler::LogHandler(LogHandler::LogLevel level, std::FILE* descriptor)
    : m_file_streamer(descriptor)
    , m_stdout_streamer(stdout, true)
    , m_level(level)
{}

