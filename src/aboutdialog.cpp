#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QTimer>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
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
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
