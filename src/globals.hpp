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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <QString>
#include <QMap>
#include <set>

namespace Global {
    namespace Consts {
        extern const QString RC_DIR;
        extern const QString RC_DIR_PATH;
        extern const QString RC_FILE;
        extern const QString RC_FILE_PATH;
        extern const QString DEFAULT_LOG_PATH;
        extern const QString DEFAULT_LOG_LEVEL;
        extern const QString KEY_LOG_PATH;
        extern const QString KEY_LOG_LEVEL;
        extern const QString KEY_LAST_PROGRAM_SET_FILE;
        extern const QString KEY_LAST_PROGRAM_SET_SAVED;
        extern const QString KEY_LAST_DIR;
        extern const QString KEY_LAST_FILE;
        extern const QString KEY_THEME;
        extern const QString KEY_ASK_INSTALL;
        extern const QString KEY_LAST_PROGRAM_SET;
        namespace Fonts {
            extern const std::vector<const char *>PATHS;
            extern const std::vector<const char *> MONO;
            extern const char * JETBRAINS;
        }

    }

    namespace Vars {
        extern QMap<QString, QString> RC;
        extern bool SAVED;
        extern std::set<const QString *> NAMES;
    }
}

#endif // GLOBALS_H
