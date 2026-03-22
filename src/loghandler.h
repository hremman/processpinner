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

#ifndef LOGHANDLER_H
#define LOGHANDLER_H

#include <QMutex>
#include <cstdio>
#include <vector>
#include <string>
#include <type_traits>
#include <cinttypes>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

template<class T> struct always_false : std::false_type {};

class LogStreamer{
public:
    LogStreamer(std::FILE* descriptor = nullptr, bool autoflush = false);
    ~LogStreamer();
    void flush();
    LogStreamer & operator << (const std::string & msg);
    LogStreamer & operator << (const std::string_view & msg);
    inline LogStreamer & operator << (char * msg){
        return operator <<(const_cast<const char *>(msg));
    }
    LogStreamer & operator << (const char * msg);
    template<typename T>
    LogStreamer & operator << (T data){
        if (!m_descriptor) return *this;
        if constexpr (std::is_unsigned_v<T>){
            if constexpr(sizeof(T) == 2) {std::fprintf(m_descriptor, "%" PRIu16, data); autoflush(); return *this;} else
            if constexpr(sizeof(T) == 4) {std::fprintf(m_descriptor, "%" PRIu32, data); autoflush(); return *this;} else
                {std::fprintf(m_descriptor, "%" PRIu64, data); autoflush(); return *this;}
        } else if constexpr (std::is_integral_v<T>){
            if constexpr(sizeof(T) == 2) {std::fprintf(m_descriptor, "%" PRIi16, data); autoflush(); return *this;} else
            if constexpr(sizeof(T) == 4) {std::fprintf(m_descriptor, "%" PRIi32, data); autoflush(); return *this;} else
                {std::fprintf(m_descriptor, "%" PRIi64, data); autoflush(); return *this;}
        } else if constexpr (std::is_floating_point<T>::value){
                if constexpr(sizeof(T) <= 4) {std::fprintf(m_descriptor, "%lf", data); autoflush(); return *this;} else
                {std::fprintf(m_descriptor, "%LF", data); autoflush(); return *this;}
        }
        static_assert(always_false<T>::value, "Unsupported type");
    }

    void emergeceWrite(const char *);
    void close();

private:
    void autoflush();

private:
    std::FILE* m_descriptor;
    bool m_closable;
    bool m_autoflush;
};

class LogHandler
{
public:
    enum class LogLevel {
        DEBUG = 0,
        INFO,
        WARNING,
        CRITICAL,
        FATAL,

        TOP_LEVEL = FATAL
    };
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static void init();
private:
    LogStreamer m_file_streamer;
    LogStreamer m_stdout_streamer;
    LogLevel m_level;
    QMutex m_mutex;

private:
    LogHandler(LogHandler::LogLevel level = LogLevel::INFO, std::FILE* descriptor = nullptr);
    void logMessage(LogHandler::LogLevel level, const QString& msg);
#ifdef Q_OS_WIN64
    static LONG WINAPI windowsExceptionFilter(struct _EXCEPTION_POINTERS* info);
#endif
    static void emergenceMsg(int sig);
    static const char* levels[];
    static LogHandler* __M_singleton_ptr;
    static bool __M_inited;
};

#endif // LOGHANDLER_H
