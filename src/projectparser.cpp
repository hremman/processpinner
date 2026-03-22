/*
 * Projects parser for Process Pinner.
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

#include <QVector>

#include "projectparser.hpp"
#include "tools/parsers/parsers.hpp"

#ifdef Q_OS_WIN
OSType osType = OSType::WINDOWS;
#elif defined(Q_OS_MAC)
OSType osType = OSType::MAC;
#elif defined(Q_OS_LINUX)
OSType osType = OSType::LINUX;
#endif

QVector<SelectorFunction> selectors = {
    gardleSelector,
   // mavenSelector
};

bool parseProject(ExecutableDTO * dto, const QString & projectDirectory, QString * error) {
    ParserFunction parser = nullptr;
    for (SelectorFunction selector: selectors){
        parser = selector(projectDirectory);
        if(parser != nullptr)
            break;
    }
    if(parser != nullptr){
        qInfo() << "    Parser for " << projectDirectory << " found";
        return parser(dto, projectDirectory, error);
    }
    qInfo() << "    Parser for " << projectDirectory << "not found";
    return false;
}


