/*
 * Gradle project parser for Process Pinner.
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

#include <QFile>
#include <QDir>
#include <QStringList>
#include "../../projectparser.hpp"


const QString rootProjectName = "rootProject.name";

bool parseGradle(ExecutableDTO * dto, const QString & projectDirectory, QString* error);
bool findInProperties(const QString & projectDirectory, const QString & name, QString* result);
ParserFunction gardleSelector(const QString & projectDirectory){
    QDir project(projectDirectory);
    if (project.exists("build.gradle.kts") && project.exists("settings.gradle.kts")) {
        return parseGradle; // Возвращаем адрес функции
    } else{
        return nullptr;
    }
}

bool parseGradle(ExecutableDTO * dto, const QString & projectDirectory, QString* error){
    QFile settingsGradleKts(QDir(projectDirectory).absoluteFilePath("settings.gradle.kts"));
    if(settingsGradleKts.open(QFile::ReadOnly | QFile::Text)){
        QTextStream isteram(&settingsGradleKts);
        isteram.setEncoding(QStringConverter::Encoding::Utf8);
        QStringList lines = isteram.readAll().split("\n");
        int baseLineNo = 0;
        for (; baseLineNo < lines.size(); ++baseLineNo){
            QString & line = lines[baseLineNo];
            if(line.contains(rootProjectName)) {
                if(line.trimmed().startsWith(rootProjectName))
                    break;
            }
        }
        if (baseLineNo == lines.size()) {
            error->clear();
            (*error) = QObject::tr("Can't find rootProject.name assigning in settings.gradle.kts");
            return false;
        }
        QString projectLine = lines[baseLineNo].trimmed();
        int delPos = projectLine.indexOf('=');
        if(delPos == projectLine.size()-1 ||delPos < 0){
            error->clear();
            (*error) = QObject::tr("Multiline rootProject.name assigning not supported");
            return false;
        }

        QString name = projectLine.mid(delPos+1).trimmed();
        if (name.startsWith('"') && name.endsWith('"') ){
            name = name.removeFirst().removeLast();
            if (name.contains("$") && name.indexOf('$') != (name.size() - 1)) {
                if (name.count("$") > 1){
                    (*error) = QObject::tr("Support only one substitution in kotlin patterns (") + projectLine + ")";
                    return false;
                }
                delPos = name.indexOf('$');
                if(name[delPos + 1] == '{') {
                    (*error) = QObject::tr("No support expressions in kotlin patterns (") + projectLine + ")";
                    return false;
                }
                QString before = name.left(delPos);
                QString tail = name.mid(delPos+1);
                int delPos2 = tail.indexOf(QRegularExpression("[^\\w_]"));
                if (delPos2 != -1){
                    tail = tail.mid(delPos2);
                } else{
                    tail = "";
                }
                name = name.mid(before.size()+1).remove(delPos2, tail.size());
                QString result;
                if(!findInProperties(projectDirectory, name, &result) ){
                    (*error) = result;
                    return false;
                }
                dto->name = before+result+tail;
            } else {
                dto->name = name;
            }
        } else {
            QString result;
            if(!findInProperties(projectDirectory, name, &result) ){
                (*error) = result;
                return false;
            }
            dto->name = result;
        }
    } else {
        qDebug() << "Can not open " << QDir(projectDirectory).absoluteFilePath("settings.gradle.kt") << ". Reason: " <<settingsGradleKts.errorString();
        error->clear();
        (*error) = QObject::tr("Can't open settings.gradle.kt: ") +  settingsGradleKts.errorString();
        return false;
    }

    dto->project_directory = projectDirectory;
    dto->work_directory = projectDirectory;
    if (osType == OSType::WINDOWS){
        dto->build_cmd = ".\\gradlew.bat shadowJar";
        dto->application = "java";
        dto->args.append("-jar");
        dto->args.append(QString("build\\libs\\%1-all.jar").arg(dto->name));
    } else {
        dto->build_cmd = "./gradlew shadowJar";
        dto->application = "java";
        dto->args.append("-jar");
        dto->args.append(QString("build/libs/%1-all.jar").arg(dto->name));
    }


    return true;
}

bool findInProperties(const QString & projectDirectory, const QString & name, QString* result){
    QFile gradleProperties(QDir(projectDirectory).absoluteFilePath("gradle.properties"));
    if(!gradleProperties.open(QFile::ReadOnly | QFile::Text)){
        qDebug() << "Can not open " << QDir(projectDirectory).absoluteFilePath("gradle.properties") << ". Reason: " <<gradleProperties.errorString();
        (*result) = QObject::tr("Can't open gradle.properties: ") +  gradleProperties.errorString();
        return false;
    }
    QTextStream isteram(&gradleProperties);
    isteram.setEncoding(QStringConverter::Encoding::Utf8);
    QStringList lines = isteram.readAll().split("\n");
    for (int i = 0; i < lines.size(); ++i){
        QString& line = lines[i];
        if(line.startsWith(name)){
            int delPos = line.indexOf('=');
            (*result) = line.mid(delPos+1).trimmed();
            return true;
        }
    }
    (*result) = QObject::tr("No found property %1 in gradle.properties").arg(name);
    return false;
}
