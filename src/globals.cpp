/*
 * Global things for Process Pinner.
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

#include <QMap>
#include <QDir>
#include "globals.hpp"
namespace Global {
    namespace Consts {
        const QString RC_DIR = ".processpinner";
        const QString RC_DIR_PATH = QDir::home().filePath(RC_DIR);
        const QString RC_FILE = "rc";
        const QString RC_FILE_PATH = QDir::home().filePath(RC_DIR) + QDir::separator() + RC_FILE;
        const QString DEFAULT_LOG_PATH = Global::Consts::RC_DIR_PATH + QDir::separator() + "log";
        const QString DEFAULT_LOG_LEVEL = "debug";


        const QString KEY_LAST_PROGRAM_SET_FILE = "lastSetFile";
        extern const QString KEY_LAST_PROGRAM_SET_SAVED = "lastSetSaved";
        const QString KEY_LAST_DIR = "lastDir";
        const QString KEY_LAST_FILE = "lastFile";
        const QString KEY_THEME = "darkTheme";
        const QString KEY_ASK_INSTALL = "askInstall";
        const QString KEY_LAST_PROGRAM_SET = "lastSet";
        const QString KEY_LOG_PATH = "logPath";
        const QString KEY_LOG_LEVEL = "logLevel";

        namespace Fonts {
            const char * JETBRAINS = "JetBrains Mono";
            const std::vector<const char *> MONO{
                JETBRAINS,
            };

            const std::vector<const char *>PATHS ={
                ":/fonts/JetBrainsMono.ttf",
            };
        }
    }

    namespace Vars {
        QMap<QString, QString> RC = {
            {Consts::KEY_LAST_PROGRAM_SET_FILE, "" },
            {Consts::KEY_LAST_DIR, QDir::homePath() },
            {Consts::KEY_LAST_FILE, QDir::homePath() },
            {Consts::KEY_THEME, "off" },
            {Consts::KEY_ASK_INSTALL, "yes"},
            {Consts::KEY_LAST_PROGRAM_SET, "" },
            {Consts::KEY_LAST_PROGRAM_SET_SAVED, "no" },
            {Consts::KEY_LOG_PATH, Consts::DEFAULT_LOG_PATH},
            {Consts::KEY_LOG_LEVEL, Consts::DEFAULT_LOG_LEVEL}

        };
        bool SAVED = false;
        std::set<const QString *> NAMES;
    }
}
