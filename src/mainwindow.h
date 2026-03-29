/*
 * Main window for Process Pinner.
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QTimer>
#include <QListWidgetItem>

#include "configurationelementwidget.h"
#include "processtab.h"
#include "helpwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QTimer m_timer_blink = QTimer();
    QTimer m_timer_epileptic = QTimer();

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();



public slots:
    void hideTabsIfNeeded();
    void changeTheme();
    void removeItem(QListWidgetItem* item);
    void editConfig(ConfigurationElementWidget* widget);
    void stopAll(bool);
    void startAll(bool){}
    void startSelected(bool);
    void stopSelected(bool);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setTheme();
    void rotateTheme();
    QString loadTheme(const QString &fileName);
    void setThemeTip(const QString &tip);
    void awaitedStopAll();
    void addElement(const ExecutableDTO & dto);
    QByteArray readJson(const QString & jsonFilePath, bool showDia = false);
    void saveAs();
    void save();
    void newList();
    bool askNewList();
    void open();
    void open(const QString& target);
    QByteArray configurationToBytes(bool compact = false);
    void writeFile(const QString & target);

    void setSaved();
    void setUnsaved();
    void reloadFromDisk();
    QString prepareTitle() const;
    Qt::CheckState newSelectorState() const;
    void setSelector(Qt::CheckState);

    void initElementMenu();

private slots:
    void addConfig(bool);
    void onClearConfig(bool);
    void selectorClicked(bool);
    void selectionChanged();
    void initConfigList();
    void savedTitle();
    void unsavedTitle();
    void rightTitle();
    void showElementMenu(const QPoint& pos);
    void setActiveTab(QListWidgetItem * item);
    void buildShortcutsList();


private:
    Ui::MainWindow *ui;
    bool m_darkTheme;
    QString m_theme_light;
    QString m_theme_dark;
    QString m_theme_punish;
    QVector<QString> m_themes;
    QMetaObject::Connection m_theme_connection;
    QMetaObject::Connection m_epileptic_connection;
    unsigned m_theme_counter;
    QString m_stored_theme_tip;
    QMap<QListWidgetItem*, ProcessTab*> m_item2executable;
    QMap<ConfigurationElementWidget*, ProcessTab*> m_config2executable;
    QMap<ProcessTab*, ConfigurationElementWidget*> m_executable2config;
    bool m_saved;
    QString m_current_file;
    QMenu *m_element_menu;
    QAction *m_goto_tab;
    QAction *m_remove;
    QAction *m_edit;
    QAction *m_start;
    QAction *m_stop;

    HelpWindow *m_help;


    static QString __M_title;
    static std::vector<std::pair<QKeySequence, QString>> __M_shortcuts;
    using shortcut_pair = std::pair<QKeySequence, QString>;
    using shortcut_iterator = std::vector<shortcut_pair>::const_iterator;

    enum SaveRun{
        NOTHING = 0,
        HAS_RUN = 1,
        SAVE = 2,
        BOTH = 3
    };

};
#endif // MAINWINDOW_H
