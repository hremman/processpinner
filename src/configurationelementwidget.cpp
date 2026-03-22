/*
 * Runnable config show widget for Process Pinner.
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

#include "configurationelementwidget.h"
#include "ui_configurationelementwidget.h"

#include <QStyleOption>
#include <QPainter>
#include <QDir>
#include <QProcess>

#include "tools.h"

long long ConfigurationElementWidget::__M_counter = 0;

ConfigurationElementWidget::ConfigurationElementWidget(const ExecutableDTO & dto, QListWidgetItem *handlerItem, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ConfigurationElementWidget)
    , m_handler(handlerItem)
    , m_dto(dto)
{
    ui->setupUi(this);
    ui->name_lbl->setProperty("runnable", "yes");
    connect(ui->runnable_chk, &QCheckBox::checkStateChanged, this, &ConfigurationElementWidget::highlightName);
    connect(ui->conf_remove_btn, &QToolButton::clicked, this, &ConfigurationElementWidget::deleteBtn);
    connect(ui->conf_edit_btn, &QToolButton::clicked, this, &ConfigurationElementWidget::editBtn);
    ui->placeholder_1->setStyleSheet("color: #00FF00; font-size: 10px");
    ui->drag_handle->setCursor(Qt::SizeVerCursor);
    setupData();
    qInfo() << "    ConfigurationElementWidget \"" << m_dto.name << "\" inited";
}

void ConfigurationElementWidget::setIndicator(bool on){
    if (on)
        ui->placeholder_1->setText("⬤");
    else
        ui->placeholder_1->setText("");
}

void ConfigurationElementWidget::ledOn(){ setIndicator(true);}
void ConfigurationElementWidget::ledOff(){ setIndicator(false);}

ConfigurationElementWidget::~ConfigurationElementWidget(){
    qDebug() << "~ConfigurationElementWidget()";
    delete ui;
}

void ConfigurationElementWidget::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ConfigurationElementWidget::highlightName(Qt::CheckState c){
    qDebug() << "void ConfigurationElementWidget::highlightName(Qt::CheckState c)";
    ui->name_lbl->setProperty("runnable", c == Qt::CheckState::Unchecked ? "no" : "yes");
    ui->name_lbl->style()->unpolish(ui->name_lbl);
    ui->name_lbl->style()->polish(ui->name_lbl);
    ui->name_lbl->update();
    m_dto.runnable = (c == Qt::CheckState::Checked);
    emit runnableChanged();
}

void ConfigurationElementWidget::deleteBtn(bool){
    qDebug() << "void ConfigurationElementWidget::deleteBtn(bool)";
    qInfo() << "    ConfigurationElementWidget \"" << m_dto.name << "\" want be deleted";
    emit deleteMe(m_handler);
}

void ConfigurationElementWidget::editBtn(bool){
    qDebug() << "void ConfigurationElementWidget::editBtn(bool)";
    qInfo() << "    ConfigurationElementWidget \"" << m_dto.name << "\" want be edited";
    emit editMe(this);
}

void ConfigurationElementWidget::setupData(){
    qDebug() <<"void ConfigurationElementWidget::setupData()";
    ui->name_lbl->setText(m_dto.name);
    auto splited = QProcess::splitCommand( m_dto.application);
    QString fullApp = tools::buildAbsolutePath(splited[0], m_dto.project_directory, m_dto.work_directory);
    ui->name_lbl->setToolTip(
        m_dto.name + ": " +
        fullApp + " " +
        (splited.sliced(1) + m_dto.args).join(" "));
    ui->runnable_chk->setCheckState(m_dto.runnable? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
}

bool ConfigurationElementWidget::isRunnable() const{
    return ui->runnable_chk->checkState() == Qt::CheckState::Checked;
}

void ConfigurationElementWidget::setRunnable(bool runnable){
    qDebug() << "void ConfigurationElementWidget::setRunnable(bool runnable)";
    QSignalBlocker blocker(this);
    ui->runnable_chk->setCheckState(runnable? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    m_dto.runnable = runnable;
}
