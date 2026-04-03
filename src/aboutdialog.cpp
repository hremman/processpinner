#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QMessageBox>
#include <QFileInfo>
#include "changelogwindow.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    if(__M_thnx_msg.length() == 0){
        auto list = QList<QPair<QString, QString>>{
            {"Alexand Istomin", tr("testing")},

            {tr("Work troubles"), tr("motivation")}//Всегда последнее
        };
        QStringList thnx_list;
        for(QPair<QString, QString> &i: list)
            thnx_list.append(__M_bullit + i.first + __M_delimeter + i.second);
        __M_thnx_msg = thnx_list.join("\n");

    }
    ui->text->setText(
        "<html><body><h1>Process Pinner</h1><p>"+
        tr("Version: %1").arg(QApplication::applicationVersion())+
        "</p><p>"+
        tr("Small tool to simplify the routine mass launch of CLI programs.")+
        "</p><p>Copyright © 2025 Danila Denisov aka hremman</p><p>"
        "<hr>"
        "<p>" + tr("This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.") + "</p>"
        "<p><b>" + tr("This program is distributed WITHOUT ANY WARRANTY.") + "</b></p>"
        "<p><a style=\"color: #0078D7\" href=\"https://www.gnu.org/licenses/gpl-3.0.txt\">" + tr("View License") + "</a></p>"
        "</p></body></html>"
    );
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    ui->text->setOpenExternalLinks(true);
    connect(ui->thnx_btn, &QPushButton::clicked, this, [this](){
        QMessageBox::information(
            this,
            tr("Special thanks"),
            __M_thnx_msg,
            QMessageBox::StandardButton::Close,
            QMessageBox::StandardButton::Close
        );
    });
    if(__M_changelog_path == nullptr || !QFileInfo(__M_changelog_path).exists() )
        ui->changelog_btn->setHidden(true);
    else {
        connect(ui->changelog_btn, &QPushButton::clicked, this, [this](bool){
            QFileInfo changelog(__M_changelog_path);
            if (!QFileInfo(__M_changelog_path).exists())
                changelogError(tr("Changelog file not found!"));
            QFile changelog_file(changelog.absoluteFilePath());
            if (!changelog_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                changelogError(tr("Can not open file!\nReason: ") + changelog_file.errorString());
                return;
            }
            QTextStream in(&changelog_file);
            QString content = in.readAll();
            changelog_file.close();
            if (content.length() == 0) {
                changelogError(tr("Changes log is empty!"));
                return;
            }
            ChangelogWindow clw(content, this);
            clw.exec();
        });
    }
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::changelogError(const QString &caused){
    qDebug() << "AboutDialog::changelogError()";
    QMessageBox::critical(
        this,
        tr("Changelog error!"),
        caused
        );
}

const char * const AboutDialog::__M_bullit = "•";
const char * const AboutDialog::__M_delimeter = " — ";
QString AboutDialog::__M_thnx_msg = "";
