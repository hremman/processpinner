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

#ifndef PROJECTPARSER_H
#define PROJECTPARSER_H

#include <QString>
#include "executabledto.h"


enum OSType{
    WINDOWS = 0,
    LINUX = 1,
    MAC = 2,
};
extern OSType osType;


using ParserFunction = bool (*)(ExecutableDTO *, const QString &, QString*);
using SelectorFunction = ParserFunction (*) (const QString &);

bool parseProject(ExecutableDTO * dto, const QString & projectDirectory, QString * error = nullptr);




#endif // PROJECTPARSER_H
