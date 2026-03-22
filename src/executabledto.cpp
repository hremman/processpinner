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

#include <QJsonArray>
#include <QTranslator>
#include <QObject>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QProcess>
#include <exception>
#include "executabledto.h"
#include "tools.h"

QString fieldRequiredMsg(const QString & field) {
    return QObject::tr("Field \"%1\" requeired!").arg(field);
}

QString fieldShouldBeMsg(const QString & field, const QString & what) {
    return QObject::tr("Field \"%1\" should be %2!").arg(field).arg(what);
}

QString fsEntityNotExistsMsg(const QString & type, const QString & path) {
    return QObject::tr("Path %1 \"%2\" not exists!").arg(type).arg(path);
}

QString fsEntityNotAbsoluteMsg(const QString & type, const QString & path) {
    return QObject::tr("Path %1 \"%2\" should be absolute!").arg(type).arg(path);
}

const QMap<QString, QString> ExecutableDTO::__M_name_mapper = {
    {S_NAME, QObject::tr("Name")},
    {S_PROJECT_DIR, QObject::tr("Project directory")},
    {S_BUILD_CMD, QObject::tr("Build command")},
    {S_BUILD_ALWAYS, QObject::tr("Build always")},
    {S_WORK_DIR, QObject::tr("Work dir")},
    {S_APP, QObject::tr("Application")},
    {S_ARGS, QObject::tr("Application arguments")},
    {S_ENV, QObject::tr("Environment variables")}
};

ExecutableDTO::ExecutableDTO()
    : name()
    , project_directory()
    , build_cmd()
    , build_always()
    , work_directory()
    , application()
    , args()
    , env()
    , runnable(true)
{}
ExecutableDTO::ExecutableDTO(
    const QString & name,
    const QString & projectDirectory,
    const QString & buildCmd,
    bool buildAlways,
    const QString & workDirectory,
    const QString & application,
    const QStringList & args,
    const QMap<QString, QString> & env,
    bool runnable
    )
    : name(name)
    , project_directory(projectDirectory)
    , build_cmd(buildCmd)
    , build_always(buildAlways)
    , work_directory(workDirectory)
    , application(application)
    , args(args)
    , env(env)
    , runnable(runnable)
{}


ExecutableDTO::ExecutableDTO(const QJsonObject & object) : ExecutableDTO(){
    QStringList errors;
    QString tmp_name;
    QString tmp_app;

    if ( !object.contains(S_NAME)) {
        errors.append(fieldRequiredMsg(S_NAME));
        tmp_name = "error";
    } else {
        tmp_name = object[S_NAME].toString().trimmed();
    }
    if (!object.contains(S_APP)) {
        errors.append(fieldRequiredMsg(S_APP));
        tmp_app = "error";
    } else {
        tmp_app = object[S_APP].toString().trimmed();
    }

    if (object.contains(S_ARGS))
        if (!object[S_ARGS].isArray()) errors.append(fieldShouldBeMsg(S_ENV, QObject::tr("a list")));

    if (object.contains(S_ENV))
        if (!object[S_ENV].isObject()) errors.append(fieldShouldBeMsg(S_ENV, QObject::tr("an object (plain map string to string)")));
    name = tmp_name;
    application = tmp_app;
    project_directory = object.contains(S_PROJECT_DIR) ? object[S_PROJECT_DIR].toString().trimmed() : "";
    build_cmd = object.contains(S_BUILD_CMD) ? object[S_BUILD_CMD].toString().trimmed() : "";
    build_always = object.contains(S_BUILD_ALWAYS) ? object[S_BUILD_ALWAYS].toBool() : false;
    runnable = object.contains(S_RUNNABLE) ? object[S_RUNNABLE].toBool() : false;
    work_directory = object.contains(S_WORK_DIR) ? object[S_WORK_DIR].toString().trimmed() : "";
    if (object.contains(S_ARGS)){
        QVariantList var_list = object[S_ARGS].toArray().toVariantList();
        for(auto i : var_list)
            if(i.canConvert<QString>())
                args.append(i.toString());
    }
    if (object.contains(S_ENV)){
        QVariantMap map = object[S_ENV].toObject().toVariantMap();
        for (auto item = map.begin(); item != map.end();++item)
            if (item.value().canConvert<QString>())
                env[item.key()] = item.value().toString();
    }

    try{
        validate();
    } catch (ExecutableDTOException &e){
        throw ExecutableDTOException(e.errors_list + errors, QObject::tr("During json parsing and validation detected next errors:"));
    }

    if (!errors.isEmpty())
        throw ExecutableDTOException(errors, QObject::tr("During json parsing and validation detected next errors:"));
}


QJsonObject ExecutableDTO::toJson() const{
    QJsonObject object;
    object[S_NAME] = name;
    object[S_APP] = application;
    object[S_RUNNABLE] = runnable;
    if (!project_directory.isEmpty()) object[S_PROJECT_DIR] = project_directory;
    if (!build_cmd.isEmpty()) object[S_BUILD_CMD] = build_cmd;
    if (!build_cmd.isEmpty()) object[S_BUILD_ALWAYS] = build_always;
    if (!work_directory.isEmpty()) object[S_WORK_DIR] = work_directory;
    if (!args.isEmpty()) object[S_ARGS] = QJsonArray::fromStringList(args);
    if (!env.isEmpty()){
        QVariantMap map;
        for (auto i = env.begin(); i != env.end(); i++)
            map[i.key()] = i.value();
        object[S_ENV] = QJsonObject::fromVariantMap(map);
    }
    return object;
}

void ExecutableDTO::validate(const QMap<QString, QString> &nameMapper){
    qDebug() << "void ExecutableDTO::validate(const QMap<QString, QString> &nameMapper)";
    QStringList errors;
    if (name.isEmpty())
        errors.append(fieldRequiredMsg(nameMapper[S_NAME]));
    if (application.isEmpty())
        errors.append(fieldRequiredMsg(nameMapper[S_APP]));
    if (!project_directory.isEmpty()) {
        if(!QDir::isAbsolutePath(project_directory))
            errors.append(fsEntityNotAbsoluteMsg(nameMapper[S_PROJECT_DIR], project_directory));
        else
            if(!QDir(project_directory).exists())
                errors.append(fsEntityNotExistsMsg(nameMapper[S_PROJECT_DIR], project_directory));
    }
    if(!build_cmd.isEmpty()){
        if (project_directory.isEmpty())
            errors.append(QObject::tr("When %1 set, %2 must be set too.").arg(nameMapper[S_BUILD_CMD]).arg(nameMapper[S_PROJECT_DIR]));
        else{
            QString buildExecutable = QProcess::splitCommand(build_cmd)[0];
            qDebug() << "    build executable: " << buildExecutable;
            QFileInfo buildExecutableInfo(buildExecutable);
            if(buildExecutableInfo.isAbsolute()){
                if (!buildExecutableInfo.exists())
                    errors.append(fsEntityNotExistsMsg(nameMapper[S_BUILD_CMD], buildExecutable));
            } else {
                if (buildExecutable != buildExecutableInfo.fileName()){
                    buildExecutableInfo = QFileInfo(QDir(project_directory).filePath(buildExecutable));
                    if(!buildExecutableInfo.exists())
                        errors.append(fsEntityNotExistsMsg(nameMapper[S_BUILD_CMD], buildExecutableInfo.absoluteFilePath()));
                }
            }
        }
    }
    if (!work_directory.isEmpty()){
        qDebug() << "    cwd: " << work_directory;
        if(!QDir::isAbsolutePath(work_directory)){
            errors.append(fsEntityNotAbsoluteMsg(nameMapper[S_WORK_DIR], work_directory));
        } else if(project_directory.isEmpty() && !QDir(work_directory).exists()) {
            errors.append(fsEntityNotExistsMsg(nameMapper[S_WORK_DIR], work_directory));
        }
    }
    if (!application.isEmpty()){
        QString appExecutable = QProcess::splitCommand(application)[0];
        qDebug() << "    application: " << appExecutable;
        QFileInfo appExecutableInfo(appExecutable); //buildAbsolutePath(appExecutable, project_directory, work_directory)
        if(appExecutableInfo.fileName() != appExecutable){
            qDebug() << "    application absolute path: " << tools::buildAbsolutePath(appExecutable, project_directory, work_directory);
            if (build_cmd.isEmpty()) {
                if(!appExecutableInfo.exists())
                    errors.append(fsEntityNotExistsMsg(nameMapper[S_APP], appExecutable));
            }
        }
    }

    if (!errors.isEmpty())
        throw ExecutableDTOException(errors, QObject::tr("During validation detected next errors:"));
}

/*
Важные нюансы для 2026 года:

    Наследование: Если вы не вызываете setProcessEnvironment, процесс наследует окружение родителя целиком.
    Регистр в Windows: Помните, что в Windows имена переменных окружения нечувствительны к регистру (Path == PATH), в то время как в Linux они различаются. Qt через QProcessEnvironment старается сглаживать это поведение, но на Windows поиск всегда будет регистронезависимым.
    Пустые строки в Linux: В отличие от Windows, в Linux переменная может существовать и быть именно "пустой" (длиной 0), не удаляясь из списка. Если ваш код должен быть кроссплатформенным, лучше явно использовать env.remove("VAR"), если вы хотите именно удалить переменную.
"Внимание: Для ОС Windows пустые переменные окружения автоматически удаляются. Если вам нужно передать 'пустой' параметр, используйте специальное значение (например, 'none' или '0'), которое ваше приложение сможет интерпретировать корректно."
*/

