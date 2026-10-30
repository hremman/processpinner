/*
 * Process Pinner - small tool to simplify the routine mass launch of CLI programs.
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

#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QDir>
#include <QLibraryInfo>
#include <QFontDatabase>

#include "globals.hpp"
#include "loghandler.h"

void loadProjectFonts();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationVersion(APP_VERSION);
    QFile rcFile(Global::Consts::RC_FILE_PATH);

    QMap<QString, QString> rc_readed;
    qDebug() << "    Read rc file: " << Global::Consts::RC_FILE_PATH;
    if (rcFile.exists()){
        if (rcFile.open(QFile::ReadOnly | QFile::Text)){
            QTextStream in(&rcFile);
            in.setEncoding(QStringConverter::Encoding::Utf8);
            while (!in.atEnd() && rc_readed.size() < Global::Vars::RC.size()) {
                QString propertyLine = in.readLine();
                if (propertyLine.startsWith('#')) continue;
                int sep_pos = propertyLine.indexOf('=');
                if (sep_pos < 0) continue;
                QString property = propertyLine.left(sep_pos).trimmed();
                if (property.isEmpty() || !Global::Vars::RC.contains(property)) continue;
                if (rc_readed.contains(property)) continue;
                QString value = propertyLine.sliced(sep_pos + 1).trimmed();
                rc_readed[property] = value;
            }
            rcFile.close();
            qDebug() << "    rc file normaly readed and parsed";
        } else {
            qWarning() << "    Could not open rc file: " << rcFile.errorString();
        }
    }

    Global::Vars::RC.insert(std::move(rc_readed));
    LogHandler::init();
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    bool languageLoaded = false;

    for (const QString &locale : uiLanguages) {
        // Проверяем как полное имя (ru_RU), так и короткое (ru)
        QString localeName = QLocale(locale).name();
        if (translator.load(":/i18n/ProcessPinner_" + localeName)) {
            a.installTranslator(&translator);
            languageLoaded = true;
            break;
        }

        // Вторая попытка для короткого имени (например, просто "ru")
        if (translator.load(":/i18n/ProcessPinner_" + localeName.left(2))) {
            a.installTranslator(&translator);
            languageLoaded = true;
            break;
        }
    }

    // Если ни один перевод не подошел (например, система на en_US),
    // принудительно выгружаемtranslator, чтобы сработал встроенный в код английский
    if (!languageLoaded) {
        a.removeTranslator(&translator);
        qDebug() << "Language fallback to native English successfully applied.";
    }
    if (!QDir::home().exists(Global::Consts::RC_DIR))
        QDir::home().mkdir(Global::Consts::RC_DIR);
    loadProjectFonts();

    MainWindow w;
    w.show();
    return a.exec();
}

void loadFont(const QString &path){
    int fontId = QFontDatabase::addApplicationFont(path);
    if (fontId != -1) {
        QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
        qDebug() << "Font loaded successfully:" << family;
    } else {
        qWarning() << "Failed to load font(" << path << ") from resources!";
    }
}

void loadProjectFonts() {
    std::vector<const char *>fonts ={
        ":/fonts/JetBrainsMono.ttf",
    };
    for(const char * fontPath: fonts)
        if(QFile::exists(fontPath))
            loadFont(fontPath);
}
