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

#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include "editconfigdialog.h"
#include "tools.h"
#include "ui_editconfigdialog.h"
#include "globals.hpp"
#include "projectparser.hpp"

EditConfigDialog::EditConfigDialog(ExecutableDTO * dto, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditConfigDialog)
    , m_dto(dto)
    , m_saved(false)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &EditConfigDialog::save);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &EditConfigDialog::discard);
    connect(ui->proj_btn, &QToolButton::clicked, this, [this](bool){
        QString proj = selectDir(tr("Choose project directory"));
        if(!proj.isEmpty()){
            ui->proj_edt->setText(proj);
        }
    });
    connect(ui->cwd_btn, &QToolButton::clicked, this, [this](bool){
        QString cwd = selectDir(tr("Choose working directory"));
        if(!cwd.isEmpty()){
            ui->cwd_edt->setText(cwd);
        }
    });
    connect(ui->build_btn, &QToolButton::clicked, this, [this](bool){
        QString build = selectFile(tr("Choose build command"));
        if(!build.isEmpty()){
            ui->build_edt->setText(build);
        }
    });
    connect(ui->app_btn, &QToolButton::clicked, this, [this](bool){
        QString app = selectFile(tr("Choose application"));
        if(!app.isEmpty()){
            auto pair = tools::selectExecutor(app);
            ui->app_edt->setText(pair.first);
            ui->env_txt->appendPlainText(pair.second.join("\n"));
        }
    });
    connect(ui->impoert_btn, &QPushButton::clicked, this, &EditConfigDialog::importproject);
    ui->app_lbl->setToolTip(
        tr("<html><head/><body><p><span style=\" color:#c01c28;\">Required!</span><br/>Application for run.</p><p>You can specify:</p><p>- absolute path</p><p>- path relative to the work directory or project directory if set</p><p>- only the name to search in the PATH.</p><p>The relative path must start with the sequence &quot;.%1&quot; or directory like \"dirName%1executable\".</p></body></html>")
            .arg(QDir::separator())
        );
    ui->build_lbl->setToolTip(
        tr("<html><head/><body><p>The command is executed before launching the application to build it.</p><p>You can specify:</p><p>- absolute path,</p><p>- path relative to the project directory</p><p>- only the name to search in the PATH.</p><p>The relative path must start with the sequence &quot;.%1&quot; or directory like \"dirName%1executable\".</p></body></html>")
            .arg(QDir::separator())
        );

    setUp(*m_dto);

}

void EditConfigDialog::setUp(const ExecutableDTO &dto){
    ui->name_edt->setText(dto.name);
    ui->proj_edt->setText(dto.project_directory);
    ui->build_edt->setText(dto.build_cmd);
    ui->build_always_chk->setCheckState(dto.build_always? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ui->cwd_edt->setText(dto.work_directory);
    ui->app_edt->setText(dto.application);
    ui->args_txt->appendPlainText(dto.args.isEmpty() ? "" : dto.args.join("\n"));
    if (!dto.env.isEmpty()){
        QStringList list;
        for (auto i = dto.env.begin(); i != dto.env.end(); i++)
            list.append(i.key() + "=" + i.value());
        ui->env_txt->appendPlainText(list.join("\n"));
    }
}

EditConfigDialog::~EditConfigDialog() {
    delete ui;
}

void validationError(QWidget * parent, const QString & msg){
    qDebug() << "void validationError(QWidget * parent, const QString & msg)";
    QMessageBox::critical(parent, EditConfigDialog::tr("Validation error!"), msg);
}

void EditConfigDialog::save() {
    qDebug() << "void EditConfigDialog::save()";
    static QMap<QString, QString> nameMapper = {
        {ExecutableDTO::S_NAME, ui->name_lbl->text()},
        {ExecutableDTO::S_PROJECT_DIR, ui->proj_lbl->text()},
        {ExecutableDTO::S_BUILD_CMD, ui->build_lbl->text()},
        {ExecutableDTO::S_BUILD_ALWAYS, ui->build_always_lbl->text()},
        {ExecutableDTO::S_WORK_DIR, ui->cwd_lbl->text()},
        {ExecutableDTO::S_APP, ui->app_lbl->text()},
        {ExecutableDTO::S_ARGS, ui->args_lbl->text()},
        {ExecutableDTO::S_ENV, ui->env_lbl->text()}
    };

    QStringList temp_env_var_pairs = ui->env_txt->toPlainText().split('\n', Qt::SkipEmptyParts);
    QMap<QString, QString> tmp_env_vars;
    for(const QString & i: temp_env_var_pairs){
        int pos = i.indexOf('=');
        QString var = i.left(pos).trimmed();
        if (!var.isEmpty())
            tmp_env_vars[var] = i.mid(pos+1);
    }
    ExecutableDTO tmp_dto(
        ui->name_edt->text().trimmed(),
        ui->proj_edt->text().trimmed(),
        ui->build_edt->text().trimmed(),
        ui->build_always_chk->checkState() == Qt::CheckState::Checked,
        ui->cwd_edt->text().trimmed(),
        ui->app_edt->text().trimmed(),
        ui->args_txt->toPlainText().split('\n', Qt::SkipEmptyParts),
        tmp_env_vars,
        m_dto->runnable
        );
    try{
        tmp_dto.validate(nameMapper);
    } catch (ExecutableDTOException &e){
        validationError(this, e.message + "\n  - " + e.errors_list.join("\n  - "));
        return;
    }

    m_dto->name = tmp_dto.name;
    m_dto->project_directory = tmp_dto.project_directory;
    m_dto->build_cmd = tmp_dto.build_cmd;
    m_dto->build_always = tmp_dto.build_always;
    m_dto->work_directory = tmp_dto.work_directory;
    m_dto->application = tmp_dto.application;
    m_dto->args = tmp_dto.args;
    m_dto->env = tmp_dto.env;
    m_dto->runnable = tmp_dto.runnable;
    m_saved = true;
    accept();
}
void EditConfigDialog::discard() {
    reject();
}

QString EditConfigDialog::selectFile(const QString &title){
    qDebug() << "QString EditConfigDialog::selectFile(const QString &title): " << title;
    QString filepath = QFileDialog::getOpenFileName(
        this,
        title,
        Global::Vars::RC[Global::Consts::KEY_LAST_FILE]
        //nullptr,
        //nullptr,
        //QFileDialog::DontUseNativeDialog
    );
    qDebug() << "Selected file: " << filepath;
    if (!filepath.isNull()){
        QFileInfo file(filepath);
        Global::Vars::RC[Global::Consts::KEY_LAST_FILE] = file.absoluteDir().absolutePath();
    }
    return filepath;
}

QString EditConfigDialog::selectDir(const QString &title){
    qDebug() << "QString EditConfigDialog::selectDir(const QString &title): " << title;
    QString dir = QFileDialog::getExistingDirectory(
        this,
        title,
        Global::Vars::RC[Global::Consts::KEY_LAST_DIR]
        //QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly
    );
    qDebug() << "Selected dir: " << dir;
    if(!dir.isEmpty()) {
        Global::Vars::RC[Global::Consts::KEY_LAST_DIR] = dir;
    }
    return dir;
}

void EditConfigDialog::importproject(bool){
    qDebug() << "void EditConfigDialog::importproject(bool)";
    QString proj = selectDir(tr("Choose project directory"));
    if(proj.isEmpty()){
        return;
    }

    ExecutableDTO temp_dto;
    QString errorSrtring;
    if(!parseProject(&temp_dto, proj, &errorSrtring)){
        QMessageBox::critical(
            this,
            EditConfigDialog::tr("Parsing error!"),
            tr("Project parsing error: %1").arg(errorSrtring)
        );
        return;
    }
    setUp(temp_dto);
}




