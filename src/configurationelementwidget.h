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

#ifndef CONFIGURATIONELEMENTWIDGET_H
#define CONFIGURATIONELEMENTWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include "executabledto.h"

namespace Ui {
class ConfigurationElementWidget;
}

class ConfigurationElementWidget : public QWidget
{
    Q_OBJECT

    static long long __M_counter;


public:
    enum class IndicatorState{
        OFF = 0,
        BUILD,
        RUN,
        ALERT,
        NUM,
        LAST = NUM - 1
    };
    explicit ConfigurationElementWidget(const ExecutableDTO & dto, QListWidgetItem *handlerItem, QWidget *parent = nullptr);
    ~ConfigurationElementWidget();
    void paintEvent(QPaintEvent *event);

    const QString & name() const {
        return m_dto.name;
    }

    ConfigurationElementWidget & dto(const ExecutableDTO & dto){
        m_dto = dto;
        setupData();
        return *this;
    }

    const ExecutableDTO & dto() const{
        return m_dto;
    }

    ExecutableDTO & dto(){
        return m_dto;
    }

    bool isRunnable() const;
    void setRunnable(bool rnnable);

    void setupData();

    void setIndicator(IndicatorState state);

signals:
    void deleteMe(QListWidgetItem *);
    void editMe(ConfigurationElementWidget *);
    void runnableChanged();

public slots:
    void ledOnRun();
    void ledOnBuild();
    void ledOnAlert();
    void ledOff();

private slots:
    void highlightName(Qt::CheckState);
    void deleteBtn(bool);
    void editBtn(bool);

private:
    Ui::ConfigurationElementWidget *ui;
    QListWidgetItem *m_handler;
    ExecutableDTO m_dto;

    static std::array<QString, int(IndicatorState::NUM)> __M_indicators_colors;
    static std::array<QString, int(IndicatorState::NUM)> __M_indicators;

};

#endif // CONFIGURATIONELEMENTWIDGET_H
