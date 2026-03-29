#include <QSignalBlocker>
#include "helpwindow.h"
#include "ui_helpwindow.h"

HelpWindow::HelpWindow(
    const std::vector<std::pair<QKeySequence, QString>> & sks,
    QWidget *parent
)
    : QWidget(parent)
    , ui(new Ui::HelpWindow)
{
    ui->setupUi(this);
    //setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    setWindowFlags(Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);
    prepareHotkeysTab(sks);

}

void HelpWindow::prepareHotkeysTab(const std::vector<std::pair<QKeySequence, QString>> & sks){
    for (const std::pair<QKeySequence, QString> & i: sks){
        int newRow = ui->hotkeys_table->rowCount();
        ui->hotkeys_table->insertRow(newRow);
        auto ksItem = new QTableWidgetItem(i.first.toString(QKeySequence::NativeText));
        ksItem->setData(Qt::UserRole, QVariant(i.first));
        ui->hotkeys_table->setItem(
            newRow,
            __M_KS_COL,
            ksItem
            );
        ui->hotkeys_table->setItem(
            newRow,
            __M_DEF_COL,
            new QTableWidgetItem(i.second)
            );
    }

    connect(ui->hotkeys_find_text, &QLineEdit::textChanged, this, [this](const QString & reff){
        auto bs1 = QSignalBlocker(ui->hotkeys_find_ks);
        auto bs2 = QSignalBlocker(ui->hotkeys_table);
        ui->hotkeys_find_ks->clear();
        ui->hotkeys_table->clearSelection();
        if(reff.isEmpty()) return;
        QString lreff = reff.toLower();
        for (int i = 0; i < ui->hotkeys_table->rowCount(); ++i){
            QString definition = ui->hotkeys_table->item(i, __M_DEF_COL)->text().toLower();
            if(definition.contains(lreff)){
                ui->hotkeys_table->selectRow(i);
            }
        }
    });

    connect(ui->hotkeys_find_ks, &QKeySequenceEdit::keySequenceChanged, this, [this](const QKeySequence& ks){
        auto bs1 = QSignalBlocker(ui->hotkeys_find_text);
        auto bs2 = QSignalBlocker(ui->hotkeys_table);
        ui->hotkeys_find_text->clear();

        if(ks.isEmpty()) return;
        for (int i = 0; i < ui->hotkeys_table->rowCount(); ++i){
            QKeySequence item_ks =  ui->hotkeys_table->item(i, __M_KS_COL)->data(Qt::UserRole).value<QKeySequence>();
            if(item_ks == ks){
                ui->hotkeys_table->selectRow(i);
            }
        }
    });

    connect (ui->hotkeys_table, &QTableWidget::itemSelectionChanged, this, [this](){
        auto bs1 = QSignalBlocker(ui->hotkeys_find_text);
        auto bs2 = QSignalBlocker(ui->hotkeys_find_ks);
        auto bs3 = QSignalBlocker(ui->hotkeys_table);
        ui->hotkeys_table->clearSelection();
        ui->hotkeys_find_text->clear();
        ui->hotkeys_find_ks->clear();
    });
}

HelpWindow::~HelpWindow()
{
    delete ui;
}
