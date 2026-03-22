/*
 * Main window for Process Pinner.
 * Copyright (C) 2025 Danila Denisov aka hremman
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


#include <QTimer>
#include <QListWidget>
#include <QListWidgetItem>
#include <QProxyStyle>

#include <QStandardPaths>
#include <QDir>
#include <QAction>
#include <QMessageBox>
#include <QProgressDialog>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QFileInfo>
#include <QCloseEvent>
#include <QFileDialog>
#include <QScrollBar>
#include <QPainter>
#include <QPainterPath>
#include <vector>

#include "mainwindow.h"
#include "editconfigdialog.h"
#include "processtab.h"
#include "ui_mainwindow.h"
#include "globals.hpp"
#include "aboutdialog.h"

QString MainWindow::__M_title = "Process Pinner[*]";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_darkTheme(false)
    , m_theme_light()
    , m_theme_dark()
    , m_theme_punish()
    , m_themes()
    , m_epileptic_connection()
    , m_theme_counter(0)
    , m_stored_theme_tip()
    , m_item2executable()
    , m_config2executable()
    , m_element_menu(nullptr)
    , m_goto_tab(nullptr)
    , m_remove(nullptr)
    , m_edit(nullptr)
    , m_start(nullptr)
    , m_stop(nullptr)
{
    ui->setupUi(this);
    qDebug() << "MainWindow::MainWindow()";
    m_theme_light = loadTheme(":/themes/LightTheme.qss");
    m_theme_dark = loadTheme(":/themes/DarkTheme.qss");
    m_theme_punish = loadTheme(":/themes/PunishmentTheme.qss");
    m_themes.append(m_theme_light);
    m_themes.append(m_theme_dark);
    m_themes.append(m_theme_punish);
    ui->help_mn->setVisible(false);

    ui->config_list->setDropIndicatorShown(true);


    setThemeTip(tr("<html><head/><body><p>Change theme</p><p><span style=\" font-size:9pt;\">Don't push to much!</span></p></body></html>"));
    setTheme();

    initElementMenu();
    m_theme_connection = connect(ui->theme_btn, &QToolButton::clicked, this, &MainWindow::changeTheme);
    connect(ui->add_btn, &QToolButton::clicked, this, &MainWindow::addConfig);
    connect(ui->remove_btn, &QToolButton::clicked, this, &MainWindow::onClearConfig);
    connect(&m_timer_epileptic, &QTimer::timeout, this, [this](){
        qDebug() << "revital timer shot";
        m_theme_counter = 0;
        qDebug() << "restire tip:" << m_stored_theme_tip;
        setThemeTip(m_stored_theme_tip);
        qDebug() << "revital timer shot: stop self";
        m_timer_epileptic.stop();
    });

    connect (ui->about_mn, &QAction::triggered, this, [this](){
        qDebug()<<"QMessageBox::about(...)";
        AboutDialog(this).exec();
    });

    connect(ui->start_btn, &QToolButton::clicked, this, &MainWindow::startSelected);
    connect(ui->stop_btn, &QToolButton::clicked, this, &MainWindow::stopSelected);
    connect(ui->stop_all_btn, &QToolButton::clicked, this, &MainWindow::stopAll);
    connect(ui->selector, &QCheckBox::clicked, this, &MainWindow::selectorClicked);

    ui->new_mn->setMenuRole(QAction::NoRole);
    ui->open_mn->setMenuRole(QAction::NoRole);
    ui->save_mn->setMenuRole(QAction::NoRole);
    ui->save_as_mn->setMenuRole(QAction::NoRole);
    ui->reload_mn->setMenuRole(QAction::NoRole);


    connect(ui->new_mn, &QAction::triggered, this, [this](bool){newList();});
    connect(ui->save_mn, &QAction::triggered, this, [this](bool){save();});
    connect(ui->save_as_mn, &QAction::triggered, this, [this](bool){saveAs();});
    connect(ui->reload_mn,  &QAction::triggered, this, [this](bool){reloadFromDisk();});
    connect(ui->open_mn, &QAction::triggered, this, [this](bool){open();});
    ui->exit_mn->setShortcut(QKeySequence::Quit);
    ui->exit_mn->setMenuRole(QAction::QuitRole);
    ui->exit_mn->setShortcutContext(Qt::WindowShortcut);
    connect(ui->exit_mn,  &QAction::triggered, this, &MainWindow::close);

    ui->help_mn->setShortcut(QKeySequence::HelpContents);

    ui->splitter->setStretchFactor(1,1);

    connect(ui->config_list, &QListWidget::itemDoubleClicked, this, &MainWindow::setActiveTab);

    connect(ui->config_list, &QListWidget::customContextMenuRequested, this, &MainWindow::showElementMenu);

    connect(ui->config_list->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int, int max){
        if(max > 0 )
            ui->stub->setMinimumWidth(18);
        else
            ui->stub->setMinimumWidth(0);
    });

    m_current_file = Global::Vars::RC[Global::Consts::KEY_LAST_PROGRAM_SET_FILE];
    m_saved = Global::Vars::RC[Global::Consts::KEY_LAST_PROGRAM_SET_SAVED] == "yes";
    m_darkTheme = (Global::Vars::RC[Global::Consts::KEY_THEME] == QString("on"));

    setTheme();
    qInfo() << "    MainWindow inited";
    QTimer::singleShot(1, this, &MainWindow::initConfigList);
    qDebug() << "    The Omnissiah approves of this communication channel. The purity of the thoughts is confirmed, the psyker resonance is within the normal range. Warp storms do not interfere with data transmission.";
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setTheme(){
    qApp->setStyleSheet(m_darkTheme ? m_theme_dark : m_theme_light);
}

QString MainWindow::loadTheme(const QString &fileName){
    qDebug() << "QString MainWindow::loadTheme(const QString &fileName)";
    QFile file(fileName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        file.close();
        return styleSheet;
    } else {
        qWarning() << "    Failed to load style sheet:" << fileName;
        return "";
    }
}

void MainWindow::rotateTheme(){
    qApp->setStyleSheet(m_themes[(m_theme_counter++)%3]);
}

void MainWindow::hideTabsIfNeeded() {
    ui->processTabs->setHidden(ui->processTabs->count() == 0);
}

void MainWindow::setThemeTip(const QString &tip) {
    ui->theme_btn->setToolTip(tip);
}

void MainWindow::changeTheme(){
    qDebug() << "void MainWindow::changeTheme()";
    m_darkTheme = !m_darkTheme;
    if (29 == m_theme_counter){
        qWarning() << "epilepic mode";
        m_theme_counter = 0;
        ui->theme_btn->setHidden(true);
        ui->theme_btn->setDisabled(true);
        m_timer_blink.start(300);
        m_epileptic_connection = connect(&m_timer_blink, &QTimer::timeout, this, &MainWindow::rotateTheme);
        qDebug() << "    epilepic mode: stop revital timer";
        m_timer_epileptic.stop();
        QTimer::singleShot(20000, [this](){
            QObject::disconnect(m_epileptic_connection);
            ui->theme_btn->setDisabled(false);
            ui->theme_btn->setHidden(false);
            m_timer_blink.stop();
            m_theme_counter = 0;
            setThemeTip(tr("<html><head/><body><p>Change theme</p><p><span style=\" font-size:9pt;\">Was it worth it?</span></p></body></html>"));
            setTheme();
        });
    } else {
        if (19 == m_theme_counter){
            qWarning() << "    punish_theme";
            qApp->setStyleSheet(m_theme_punish);
            m_stored_theme_tip = ui->theme_btn->toolTip();
            qDebug() << "    store current tip:" << m_stored_theme_tip;
            qDebug() << "    start revital timer";
            m_timer_epileptic.start(20000);
        } else
            setTheme();

        if (19 < m_theme_counter){
            qDebug() << "epilepic mode after " << 29 - m_theme_counter;
            setThemeTip( tr("<html><head/><body><p>Change theme</p><span style=\" font-size:9pt;\"><p>Before epiletic %1...</p></span></body></html>").arg(29 - m_theme_counter));
        }
        m_theme_counter += 1;
    }
}

void MainWindow::addConfig(bool){
    qDebug() << "void MainWindow::addConfig(bool)";
    ExecutableDTO dto;
    EditConfigDialog dialog(&dto, this);
    dialog.exec();
    qDebug() << dialog.result();
    if (dialog.result()) {
        addElement(dto);
        setUnsaved();
        selectionChanged();
        rightTitle();
    }
}

void MainWindow::addElement(const ExecutableDTO & dto){
    qDebug() << "void MainWindow::addElement(const ExecutableDTO & dto)";
    auto item = new QListWidgetItem(ui->config_list);
    ui->config_list->addItem(item);
    ConfigurationElementWidget *element = new ConfigurationElementWidget(dto, item, this);
    item->setSizeHint(element->sizeHint());
    ui->config_list->setItemWidget(item, element);
    ProcessTab * tab = new ProcessTab(&(element->dto()), nullptr);
    ui->processTabs->addTab(tab, element->name());
    connect(element, &ConfigurationElementWidget::deleteMe, this, &MainWindow::removeItem);
    connect(element, &ConfigurationElementWidget::editMe, this, &MainWindow::editConfig);
    m_item2executable[item] = tab;
    m_config2executable[element] = tab;
    ui->processTabs->setHidden(false);
    connect(element, &ConfigurationElementWidget::runnableChanged, this, &MainWindow::selectionChanged);
    connect(tab, &ProcessTab::stopped, element, &ConfigurationElementWidget::ledOff);
    connect(tab, &ProcessTab::started, element, &ConfigurationElementWidget::ledOn);
}

void MainWindow::removeItem(QListWidgetItem* item){
    qDebug() << "void MainWindow::removeItem(QListWidgetItem* item)";
    if (!item) return;
    ProcessTab * tabWidget = m_item2executable[item];
    ConfigurationElementWidget* config = qobject_cast<ConfigurationElementWidget*>(ui->config_list->itemWidget(item));
    if (!tabWidget->isStoped()){
        QMessageBox::warning(
            this,
            tr("Attention"),
            tr("You need stop \"%1\" before remove configuration.").arg(config->name())
        );
        return;
    }
    if ( QMessageBox::StandardButton::Yes == QMessageBox::question(this, tr("Remove \"")+config->name() + "\"?",
                                                                  tr("Do you want to remove \"%1\" configuration?").arg(config->name()),
                                                                  QMessageBox::StandardButtons(QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No), QMessageBox::StandardButton::No))
    {
        disconnect(config, &ConfigurationElementWidget::deleteMe, this, &MainWindow::removeItem);
        m_item2executable.remove(item);
        int index = ui->processTabs->indexOf(tabWidget);
        ui->processTabs->removeTab(index);
        delete tabWidget;
        int row = ui->config_list->row(item);
        m_config2executable.remove(config);
        delete ui->config_list->takeItem(row);
        hideTabsIfNeeded();
    }
    setUnsaved();
}

void MainWindow::awaitedStopAll(){
    qDebug() << "void MainWindow::awaitedStopAll()";
    QList<ProcessTab*> tabsToStop;
    for (int tabIndex = 0; tabIndex < ui->processTabs->count(); ++tabIndex){
        auto tab = qobject_cast<ProcessTab*>(ui->processTabs->widget(tabIndex));
        if(tab && !tab->isStoped())
            tabsToStop.append(tab);
    }

    if (!tabsToStop.isEmpty()){
        QProgressDialog progress(this);
        progress.setWindowTitle(tr("Stopping %1").arg(tabsToStop.size()));
        progress.setRange(0, tabsToStop.size());
        progress.setWindowModality(Qt::WindowModal);
        progress.setCancelButton(nullptr);
        progress.setMinimumSize(300,80);
        progress.setMaximumSize(300,800);
        progress.setMinimumDuration(0);
        progress.show();

        std::atomic_int stopped = 0;
        int tabsToStopNum = tabsToStop.size();
        std::vector<std::unique_ptr<QTimer>> killers;
        killers.reserve(tabsToStopNum);
        std::vector<std::unique_ptr<QTimer>> terminators;
        terminators.reserve(tabsToStopNum);
        progress.setLabelText(
            tr("Stopped %1 of %2.")
            .arg(stopped.load())
            .arg(tabsToStopNum)
        );
        QCoreApplication::processEvents();
        QEventLoop loop;

        auto finaliser = [&stopped, tabsToStopNum, &loop, &progress](int, QProcess::ExitStatus){
            stopped++;
            progress.setValue(stopped);
            progress.setLabelText(
                tr("Stopped %1 of %2.")
                    .arg(stopped.load())
                    .arg(tabsToStopNum)
                );
            if(stopped.load() == tabsToStopNum)
                loop.quit();
        };

        for (int i = 0; i < tabsToStopNum; ++i) {
            ProcessTab* tab = tabsToStop[i];
            tab->stop();


            std::unique_ptr<QTimer> kill_timer = std::make_unique<QTimer>(nullptr);
            connect(kill_timer.get(), &QTimer::timeout, tab->process(), &QProcess::kill);
            kill_timer->setSingleShot(true);


            std::unique_ptr<QTimer> terminate_timer = std::make_unique<QTimer>(nullptr);
            connect(terminate_timer.get(), &QTimer::timeout, tab->process(), &QProcess::terminate);
            terminate_timer->setSingleShot(true);

            if (tab->process()->state() != QProcess::NotRunning) {
                terminate_timer->start(10000);
                kill_timer->start(25000);
                connect(tab->process(), &QProcess::finished, this, finaliser);
            } else {
                finaliser(0, QProcess::NormalExit);
            }

            killers.push_back(std::move(kill_timer));
            terminators.push_back(std::move(terminate_timer));
        }
        if (stopped.load() < tabsToStopNum) {
            loop.exec();
        }
    }
}

void MainWindow::onClearConfig(bool){
    qDebug() << "void MainWindow::onClearConfig(bool)";
    if (!ui->config_list->count()) return;
    if ( QMessageBox::StandardButton::Yes ==
        QMessageBox::question(this, tr("Clear all?"),
            tr("Do you want to remove all configurations?\nThis also will stop all processes!"),
            QMessageBox::StandardButtons(QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No),
            QMessageBox::StandardButton::No)
        ) {
        awaitedStopAll();
        ui->config_list->clear();
        m_config2executable.clear();
        m_item2executable.clear();
        ui->processTabs->clear();
        ui->processTabs->setHidden(true);
        ui->selector->setCheckState(Qt::CheckState::Unchecked);
        setUnsaved();
    }
}

void MainWindow::editConfig(ConfigurationElementWidget* widget){
    qDebug() << "void editConfig(ConfigurationElementWidget* widget)";
    EditConfigDialog dialog(&(widget->dto()), this);
    dialog.exec();
    if(dialog.saved()){
        ui->processTabs->setTabText(ui->processTabs->indexOf(m_config2executable[widget]), widget->dto().name);
        m_config2executable[widget]->dtoUpdated();
        widget->setupData();
        setUnsaved();
    }
}

void MainWindow::startSelected(bool){
    qDebug() << "void MainWindow::startSelected(bool)";
    for(int i = 0; i < ui->config_list->count(); ++i) {
        QListWidgetItem* item = ui->config_list->item(i);
        ConfigurationElementWidget* config = qobject_cast<ConfigurationElementWidget*>( ui->config_list->itemWidget(item));
        if(config->isRunnable()){
            m_config2executable[config]->startApp();
        }
    }
}

void MainWindow::stopSelected(bool){
    qDebug() << "void MainWindow::stopSelected(bool)";
    for (auto item = m_config2executable.constBegin(); item != m_config2executable.constEnd(); ++item)
        if(item.key()->isRunnable()) item.value()->stopApp();
}

void MainWindow::stopAll(bool){
    qDebug() << "void MainWindow::stopAll(bool)";
    for (auto item = m_config2executable.constBegin(); item != m_config2executable.constEnd(); ++item)
        item.value()->stopApp();
}

void MainWindow::setSelector(Qt::CheckState newState){
    ui->selector->blockSignals(true);
    ui->selector->setCheckState(newState);
    ui->selector->blockSignals(false);
}

void MainWindow::selectorClicked(bool){
    qDebug() << "void MainWindow::selectorClicked(bool)";
    if (m_config2executable.isEmpty()){
        setSelector(Qt::CheckState::Unchecked);
        return;
    }
    qDebug() << "    checkState" << ui->selector->checkState();
    for (auto i = m_config2executable.constBegin(); i != m_config2executable.constEnd(); i++)
        if(!(i.key()->isRunnable())) i.key()->setRunnable(true);
    setSelector(Qt::CheckState::Checked);
    setUnsaved();
}

Qt::CheckState MainWindow::newSelectorState() const {
    bool hasChecked = false;
    bool hasUnchecked = false;
    for (auto i = m_config2executable.constBegin(); i != m_config2executable.constEnd(); ++i) {
        hasChecked = hasChecked || i.key()->isRunnable();
        hasUnchecked = hasUnchecked || !i.key()->isRunnable();
    }
    return (hasChecked && !hasUnchecked) ? Qt::CheckState::Checked : ((hasChecked && hasUnchecked) ? Qt::CheckState::PartiallyChecked : Qt::CheckState::Unchecked );
}

void MainWindow::selectionChanged(){
    qDebug() << "void MainWindow::selectionChanged()";
    if (m_config2executable.isEmpty()){
        setSelector(Qt::CheckState::Unchecked);
        return;
    }
    Qt::CheckState state = newSelectorState();
    if (ui->selector->checkState() != state) {
        setSelector(state);
        setUnsaved();
    }
}

QByteArray MainWindow::configurationToBytes(bool compact){
    QJsonArray array;
    for(int i = 0; i < ui->config_list->count(); ++i) {
        QListWidgetItem* item = ui->config_list->item(i);
        ConfigurationElementWidget* config = qobject_cast<ConfigurationElementWidget*>( ui->config_list->itemWidget(item));
        array.append(config->dto().toJson());
    }
    QJsonDocument doc(array);
    return doc.toJson( compact ? QJsonDocument::Compact : QJsonDocument::Indented);
}

void MainWindow::closeEvent(QCloseEvent *event){
    qDebug() << "void MainWindow::closeEvent(QCloseEvent *event)";
    for (auto pair_c_t = m_config2executable.constBegin(); pair_c_t != m_config2executable.constEnd(); ++pair_c_t)
        if(!(pair_c_t.value()->isStoped())){
            auto pressed = QMessageBox::question(
                this,
                tr("Stop on exit?"),
                tr("There is run process(es).\nStop it?"),
                QMessageBox::StandardButtons(QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::Cancel),
                QMessageBox::StandardButton::Cancel
            );
            if (pressed == QMessageBox::StandardButton::Cancel){
                event->ignore();
                return;
            } else {
                awaitedStopAll();
            }
        }

    Global::Vars::RC[Global::Consts::KEY_THEME] = m_darkTheme ? "on" : "off";
    if(!m_config2executable.isEmpty() && !m_saved){
        qDebug() << "    Store config in RC";
        QByteArray byteArray = configurationToBytes(true);
        Global::Vars::RC[Global::Consts::KEY_LAST_PROGRAM_SET] = QString::fromUtf8(byteArray);
    } else {
        Global::Vars::RC[Global::Consts::KEY_LAST_PROGRAM_SET] = "";
    }
    Global::Vars::RC[Global::Consts::KEY_LAST_PROGRAM_SET_FILE] = m_current_file;
    Global::Vars::RC[Global::Consts::KEY_LAST_PROGRAM_SET_SAVED] = m_saved ? "yes" : "no";
    QFile rcFile(Global::Consts::RC_FILE_PATH);
    if(rcFile.open(QFile::WriteOnly | QFile::Text)){
        qDebug() << "Dump rc to file";
        QTextStream out(&rcFile);
        out.setEncoding(QStringConverter::Encoding::Utf8);
        out << "#Be careful on edit this file mannual" << Qt::endl;
        out << "#Осторожно редактируйте файл руками" << Qt::endl;
        for (auto property = Global::Vars::RC.constBegin(); property != Global::Vars::RC.constEnd(); property++)
            out << property.key() << '=' << property.value() << Qt::endl;
    } else {
        qWarning() << "    Could not open rc file for dump: " << rcFile.errorString();
    }

    event->accept();
}

QByteArray MainWindow::readJson(const QString & jsonFilePath, bool showDia){
    QFile conffile(jsonFilePath);
    QByteArray json;
    if(conffile.open(QFile::ReadOnly | QFile::Text)){
        json = conffile.readAll();
        conffile.close();
    } else {
        qDebug() << "Open config file error: " << conffile.errorString();
        if(showDia){
            QMessageBox::critical(
                this,
                tr("Open configuration error!"),
                tr("Can not open configuration list file.\nReason: %1").arg(conffile.errorString())
            );
        }
    }
    return json;
}



void MainWindow::initConfigList(){
    qDebug() << "void MainWindow::initConfigList()";
    QByteArray jsonBytes;
    QString & lastSet = Global::Vars::RC[Global::Consts::KEY_LAST_PROGRAM_SET_FILE];
    QFileInfo fileinfo(lastSet);
    bool fileSet = !Global::Vars::RC[Global::Consts::KEY_LAST_PROGRAM_SET_FILE].isEmpty();

    if (fileSet){
        if(fileinfo.exists()){
            if(m_saved){
                jsonBytes = readJson(fileinfo.absoluteFilePath());
                qDebug() << "    Use json string from " + fileinfo.absoluteFilePath();
            } else {
                jsonBytes = Global::Vars::RC[Global::Consts::KEY_LAST_PROGRAM_SET].toUtf8();
                qDebug() << "    Use json string from RC";
            }
            m_current_file = lastSet;
        }
    } else {
        jsonBytes = Global::Vars::RC[Global::Consts::KEY_LAST_PROGRAM_SET].toUtf8();
        //m_saved = false;
        m_current_file = "";
        qDebug() << "    Use json string from RC";
    }

    if (!jsonBytes.isEmpty()) {
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonBytes, &parseError);
        if(jsonDoc.isNull()){
            QMessageBox::critical(
                this,
                tr("Parsing error!"),
                tr("Error happened while parsing %1\nError: %2").arg(parseError.errorString())
            );
        } else {
            if (jsonDoc.isArray()){
                QJsonArray elements = jsonDoc.array();
                int failed = 0;
                for (QJsonArray::const_iterator i = elements.constBegin(); i != elements.constEnd(); ++i) {
                    try{
                        addElement(ExecutableDTO(i->toObject()));
                    } catch( ExecutableDTOException &e){
                        QMessageBox::critical(
                            this, tr("Validation error!"),
                            e.message + "\n  - " + e.errors_list.join("\n  - ")
                        );
                        ++failed;
                    }
                }
                qDebug() << "    Failed to load: " << failed << "/" << elements.size();
                if (failed)
                    QMessageBox::warning(
                        this,
                        tr("Setup errors!"),
                        tr("Failed to load %1 of %2 elements").arg(failed).arg(elements.size())
                    );
            }else{
                QMessageBox::warning(this, tr("Open errors!"), tr("Json is not a list!"));
            }
        }
    } else {
        qDebug() << "    Nothing to load";
    }
    setWindowTitle(prepareTitle());
    setSelector(newSelectorState());
    rightTitle();
    hideTabsIfNeeded();
}

void MainWindow::rightTitle(){
    if(m_saved)
        savedTitle();
    else
        unsavedTitle();
}

QString MainWindow::prepareTitle() const{
    if (m_current_file.isEmpty())
        return __M_title;
    else
        return __M_title + QString(" (%1)").arg(m_current_file);
}


void MainWindow::savedTitle(){
    setWindowModified(false);
}

void MainWindow::unsavedTitle(){
    setWindowModified(true);
}


void MainWindow::saveAs(){
    qDebug() << "void MainWindow::saveAs()";
    QString target = QFileDialog::getSaveFileName(
        this,
        tr("Save configuration as"),
        Global::Vars::RC[Global::Consts::KEY_LAST_FILE],
        "JSON (*.json)"
    );
    if (target.isEmpty()) return;
    if (!target.endsWith(".json")) target = target+".json";
    qDebug() << "Save to: " << target;
    writeFile(target);
    m_current_file = target;
    Global::Vars::RC[Global::Consts::KEY_LAST_FILE] = m_current_file;
    setWindowTitle(prepareTitle());
    setSaved();
}

void MainWindow::save(){
    qDebug() << "void MainWindow::save()";
    if(m_current_file.isEmpty()){
        saveAs();
        return;
    }
    writeFile(m_current_file);
    setSaved();
}

void MainWindow::writeFile(const QString & target){
    QFile targetFile(target);
    if(targetFile.open(QFile::WriteOnly | QFile::Text)){
        auto data = configurationToBytes();
        targetFile.write(data);
    } else {
        qWarning() << "Could not open "<< target <<" for save: " << targetFile.errorString();
    }
}

bool MainWindow::askNewList(){
    qDebug() << "bool MainWindow::askNewList()";
    qDebug() << "    Time to choose mr. ...";
        SaveRun state = m_saved ? SaveRun::NOTHING : SaveRun::SAVE;
        for (auto i = m_config2executable.constBegin(); i != m_config2executable.constEnd(); i++){
            if (!i.value()->isStoped()){
                state = static_cast<SaveRun>(state | SaveRun::HAS_RUN);
                break;
            }
        }
        switch (state) {
        case SaveRun::HAS_RUN: {
            auto pressed = QMessageBox::question(
                this,
                tr("Stop all?"),
                tr("There is run process(es). Stop them?"),
                QMessageBox::StandardButtons(QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::Cancel),
                QMessageBox::StandardButton::Cancel
                );
            if (QMessageBox::StandardButton::Cancel == pressed) return false;
            break;
        }
        case SaveRun::SAVE:{
            auto pressed = QMessageBox::question(
                this,
                tr("Save?"),
                tr("Current configuration not saved. Save?"),
                QMessageBox::StandardButtons(QMessageBox::StandardButton::Save | QMessageBox::StandardButton::Discard | QMessageBox::StandardButton::Cancel),
                QMessageBox::StandardButton::Cancel
                );
            switch (pressed) {
            case QMessageBox::StandardButton::Save:
                save();
                break;
            case QMessageBox::StandardButton::Cancel:
                return false;
            default:
                break;
            }
            break;
        }
        case SaveRun::BOTH: {
            auto pressed = QMessageBox::question(
                this,
                tr("Stop and save?"),
                tr("There is run process(es). Current configuration not saved. How to deal with it?"),
                QMessageBox::StandardButtons(QMessageBox::StandardButton::Save | QMessageBox::StandardButton::Discard | QMessageBox::StandardButton::Cancel),
                QMessageBox::StandardButton::Cancel
                );
            switch (pressed) {
            case QMessageBox::StandardButton::Save:
                save();
                break;
            case QMessageBox::Cancel:
                return false;
            default:
                break;
            }
            break;
        }
        default:
            break;
        }
    return true;
}

void MainWindow::newList(){
    qDebug() << "void MainWindow::newList()";
    if(askNewList()) {
        awaitedStopAll();
        ui->config_list->clear();
        m_config2executable.clear();
        m_item2executable.clear();
        ui->processTabs->clear();
        ui->processTabs->setHidden(true);
        setSelector(Qt::CheckState::Unchecked);
    }
    m_current_file = "";
    setWindowTitle(prepareTitle());
    setSaved();
}

void MainWindow::open(const QString& target){
    if (target.isEmpty()) return;
    QByteArray jsonBytes = readJson(target, true);
    if(jsonBytes.isEmpty()) return;

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonBytes, &parseError);
    if(jsonDoc.isNull()){
        QMessageBox::critical(
            this,
            tr("Parsing error!"),
            tr("Error happened while parsing %1\nError: %2").arg(parseError.errorString())
            );
        return;
    } else {
        QVector<ExecutableDTO> dtos;
        if (jsonDoc.isArray()){
            QJsonArray elements = jsonDoc.array();
            int failed = 0;
            for (QJsonArray::const_iterator i = elements.constBegin(); i != elements.constEnd(); ++i) {
                try{
                    auto tmp = ExecutableDTO(i->toObject());
                    dtos.push_back(tmp);
                } catch( ExecutableDTOException &e){
                    QMessageBox::critical(
                        this, tr("Validation error!"),
                        e.message + "\n  - " + e.errors_list.join("\n  - ")
                        );
                    ++failed;
                }
            }
            if (failed){
                qDebug() << "    Failed to load: " << failed;
                QMessageBox::warning(this, tr("Open errors!"), tr("Failed to load %1 of %2 elements").arg(failed).arg(elements.size()));
            }
        } else {
            QMessageBox::warning(this, tr("Open errors!"), tr("Json is not a list!"));
            qDebug() << "    Json is not a list!";
            return;
        }
        awaitedStopAll();
        ui->config_list->clear();
        m_config2executable.clear();
        m_item2executable.clear();
        ui->processTabs->clear();
        ui->processTabs->setHidden(true);
        setSelector(Qt::CheckState::Unchecked);
        for(const ExecutableDTO & dto: dtos)
            addElement(dto);
    }
}

void MainWindow::open(){
    qDebug() << "void MainWindow::open()";
    if(askNewList()){
        QString target = QFileDialog::getOpenFileName(
            this,
            tr("Open configuration"),
            Global::Vars::RC[Global::Consts::KEY_LAST_FILE],
            "JSON (*.json)"
        );

        open(target);
        Global::Vars::RC[Global::Consts::KEY_LAST_FILE] = target;
        m_current_file = target;
        setWindowTitle(prepareTitle());
        setSelector(newSelectorState());
        setSaved();
    }

}

void MainWindow::setSaved(){
    qDebug() << "void MainWindow::setSaved()";
    m_saved = true;
    ui->save_mn->setEnabled(false);
    savedTitle();
    QApplication::processEvents();
}

void MainWindow::setUnsaved(){
    qDebug() << "void MainWindow::setUnsaved()";
    m_saved = false;
    ui->save_mn->setEnabled(true);
    unsavedTitle();
    QApplication::processEvents();
}

void MainWindow::reloadFromDisk(){
    qDebug() << "void MainWindow::reloadFromDisk()";
    open(m_current_file);
    setSelector(newSelectorState());
    setSaved();
}

void MainWindow::setActiveTab(QListWidgetItem * item){
    if (item){
        ui->processTabs->setCurrentWidget(m_item2executable[item]);
    }
}

class BigIconStyle : public QProxyStyle {
public:
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override {
        if (metric == QStyle::PM_SmallIconSize) {
            return 23; // Устанавливаем желаемый размер
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
};

void MainWindow::initElementMenu(){
#ifdef Q_OS_WINDOWS
    QIcon dClickIcon(":/icons/d_click_windows.png");
#elif Q_OS_DARWIN
    QIcon dClickIcon(":/icons/d_click_macos.png");
#else
    QIcon dClickIcon(":/icons/d_click_linux.png");
#endif

    auto style = new BigIconStyle();

    m_element_menu = new QMenu(this);
    m_element_menu->setStyle(style);
    m_goto_tab = m_element_menu->addAction(dClickIcon, tr("Go to associated tab"));
    m_element_menu->addSeparator();
    m_edit = m_element_menu->addAction(/*QIcon::fromTheme(QIcon::ThemeIcon::InsertText), */tr("Edit"));
    m_remove = m_element_menu->addAction(/*QIcon::fromTheme(QIcon::ThemeIcon::EditClear),*/tr("Remove"));
    m_element_menu->addSeparator();
    m_start = m_element_menu->addAction(/*QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart), */tr("Start/Restart"));
    m_stop = m_element_menu->addAction(/*QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStop), */tr("Stop"));

    connect(m_goto_tab, &QAction::triggered, this, [this](bool){
        auto item = reinterpret_cast<QListWidgetItem *>(m_goto_tab->data().toLongLong());
        setActiveTab(item);
    });

    connect(m_edit, &QAction::triggered, this, [this](bool){
        auto item = reinterpret_cast<QListWidgetItem *>(m_edit->data().toLongLong());
        editConfig(qobject_cast<ConfigurationElementWidget*>(ui->config_list->itemWidget(item)));
    });

    connect(m_remove, &QAction::triggered, this, [this](bool){
        auto item = reinterpret_cast<QListWidgetItem *>(m_remove->data().toLongLong());
        removeItem(item);
    });

    connect(m_start, &QAction::triggered, this, [this](bool){
        auto item = reinterpret_cast<QListWidgetItem *>(m_start->data().toLongLong());
        m_item2executable[item]->startApp();
    });

    connect(m_stop, &QAction::triggered, this, [this](bool){
        auto item = reinterpret_cast<QListWidgetItem *>(m_stop->data().toLongLong());
        m_item2executable[item]->stopApp();
    });
}

void MainWindow::showElementMenu(const QPoint& pos){

    QListWidgetItem *item = ui->config_list->itemAt(pos);
    if (!item) return;
    long long itemPointer = reinterpret_cast<long long>(item); //Грязный трюк. Плохо так делать.
    m_goto_tab->setData(itemPointer);
    m_edit->setData(itemPointer);
    m_remove->setData(itemPointer);
    m_start->setData(itemPointer);
    m_stop->setData(itemPointer);

    m_start->setEnabled(m_item2executable[item]->startButtonEnabled());
    m_stop->setEnabled(m_item2executable[item]->stopButtonEnabled());

    // Выполняем
    m_element_menu->exec(ui->config_list->mapToGlobal(pos));

}
