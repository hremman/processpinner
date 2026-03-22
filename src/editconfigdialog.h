/*
 * Runnable config edit dialog for Process Pinner.
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

#ifndef EDITCONFIGDIALOG_H
#define EDITCONFIGDIALOG_H

#include <QDialog>
#include "executabledto.h"

namespace Ui {
class EditConfigDialog;
}

class EditConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditConfigDialog(ExecutableDTO * dto, QWidget *parent = nullptr);
    ~EditConfigDialog();

    bool saved() const {
        return m_saved;
    }

private:
    void setUp(const ExecutableDTO &dto);

private slots:
    void save();
    void discard();

    QString selectFile(const QString &title);
    QString selectDir(const QString &title);

    void importproject(bool);


private:
    Ui::EditConfigDialog *ui;
    ExecutableDTO * m_dto;
    bool m_saved;
};

#endif // EDITCONFIGDIALOG_H
