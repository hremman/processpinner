/*
 * Data class about runnable for Process Pinner.
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

#ifndef EXECUTABLEDTO_H
#define EXECUTABLEDTO_H
#include <QJsonObject>
#include <QException>

class ExecutableDTOException : public QException
{
public:
    ExecutableDTOException(const QString &msg): message(msg) {}
    ExecutableDTOException(const QStringList & errors, const QString &msg = ""): message(msg), errors_list(errors) {}

    QString message;
    QStringList errors_list;
    void raise() const override { throw *this; }
    ExecutableDTOException *clone() const override { return new ExecutableDTOException(*this); }
};



class ExecutableDTO
{
public:
    ExecutableDTO();
    ExecutableDTO(
        const QString & name,
        const QString & projectDirectory,
        const QString & buildCmd,
        bool build_always,
        const QString & workDirectory,
        const QString & application,
        const QStringList & args,
        const QMap<QString, QString> & env,
        bool runnable
        );
    ExecutableDTO(const QJsonObject & json);


    QJsonObject toJson() const;

    static ExecutableDTO fromJson(const QJsonObject & object){
        return ExecutableDTO(object);
    }

    void validate(const QMap<QString, QString> &nameMapper = __M_name_mapper);

public:
    constexpr static const char * S_NAME = "name";
    QString name;
    constexpr static const char * S_PROJECT_DIR = "projectDirectory";
    QString project_directory;
    constexpr static const char * S_BUILD_CMD = "buildCmd";
    QString build_cmd;
    constexpr static const char * S_BUILD_ALWAYS = "buildAlways";
    bool build_always;
    constexpr static const char * S_WORK_DIR = "workDirectory";
    QString work_directory;
    constexpr static const char * S_APP = "application";
    QString application;
    constexpr static const char * S_ARGS = "args";
    QStringList args;
    constexpr static const char * S_ENV = "env";
    QMap<QString, QString> env;
    constexpr static const char * S_RUNNABLE = "runnable";
    bool runnable;

private:
    static const QMap<QString, QString> __M_name_mapper;
};

#endif // EXECUTABLEDTO_H
