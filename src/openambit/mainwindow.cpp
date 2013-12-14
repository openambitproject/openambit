/*
 * (C) Copyright 2013 Emil Ljungdahl
 *
 * This file is part of Openambit.
 *
 * Openambit is free software: you can redistribute it and/or modify
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contributors:
 *
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QListWidgetItem>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Setup UI parts
    QIcon warningIcon = QIcon::fromTheme("dialog-warning");
    ui->labelNotSupportedIcon->setPixmap(warningIcon.pixmap(8,8));
    ui->labelNotSupportedIcon->setHidden(true);
    ui->labelNotSupported->setHidden(true);
    ui->labelCharge->setHidden(true);
    ui->chargeIndicator->setHidden(true);
    ui->checkBoxResyncAll->setHidden(true);
    ui->buttonSyncNow->setHidden(true);
    ui->syncProgressBar->setHidden(true);

    connect(ui->actionSettings, SIGNAL(triggered()), this, SLOT(showSettings()));

    // Setup device manager
    deviceManager = new DeviceManager();
    deviceManager->moveToThread(&deviceWorkerThread);
    qRegisterMetaType<ambit_device_info_t>("ambit_device_info_t");
    connect(deviceManager, SIGNAL(deviceDetected(ambit_device_info_t,bool)), this, SLOT(deviceDetected(ambit_device_info_t,bool)), Qt::QueuedConnection);
    connect(deviceManager, SIGNAL(deviceRemoved()), this, SLOT(deviceRemoved()), Qt::QueuedConnection);
    connect(deviceManager, SIGNAL(deviceCharge(quint8)), this, SLOT(deviceCharge(quint8)), Qt::QueuedConnection);
    connect(deviceManager, SIGNAL(syncFinished(bool)), this, SLOT(syncFinished(bool)), Qt::QueuedConnection);
    connect(deviceManager, SIGNAL(syncProgressInform(QString,bool,quint8)), this, SLOT(syncProgressInform(QString,bool,quint8)), Qt::QueuedConnection);
    connect(ui->buttonDeviceReload, SIGNAL(clicked()), deviceManager, SLOT(detect()));
    connect(ui->buttonSyncNow, SIGNAL(clicked()), this, SLOT(syncNowClicked()));
    connect(this, SIGNAL(syncNow(bool)), deviceManager, SLOT(startSync(bool)));
    deviceWorkerThread.start();
    deviceManager->start();
    deviceManager->detect();

    // Setup log list
    connect(ui->logsList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(logItemSelected(QListWidgetItem*,QListWidgetItem*)));
    ui->logsList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->logsList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenuForLogItem(QPoint)));

    updateLogList();
}

MainWindow::~MainWindow()
{
    deviceWorkerThread.quit();
    deviceWorkerThread.wait();
    delete ui;
}

void MainWindow::showSettings()
{
    settingsDialog = new SettingsDialog(this);
    settingsDialog->setModal(true);
    settingsDialog->show();
}

void MainWindow::syncNowClicked()
{
    ui->checkBoxResyncAll->setEnabled(false);
    ui->buttonSyncNow->setEnabled(false);
    currentLogMessageRow = NULL;
    QLayoutItem *tmpItem;
    while ((tmpItem = ui->verticalLayoutLogMessages->takeAt(0)) != NULL) {
        delete tmpItem->widget();
        delete tmpItem;
    }
    ui->syncProgressBar->setHidden(false);
    ui->syncProgressBar->setValue(0);
    emit MainWindow::syncNow(ui->checkBoxResyncAll->isChecked());
}

void MainWindow::deviceDetected(ambit_device_info_t deviceInfo, bool supported)
{
    ui->labelDeviceDetected->setText(deviceInfo.name);
    ui->labelSerial->setText(deviceInfo.serial);
    if (!supported) {
        ui->labelNotSupportedIcon->setHidden(false);
        ui->labelNotSupported->setHidden(false);
        ui->labelCharge->setHidden(true);
        ui->chargeIndicator->setHidden(true);
        ui->checkBoxResyncAll->setHidden(true);
        ui->buttonSyncNow->setHidden(true);
        ui->syncProgressBar->setHidden(true);
    }
    else {
        ui->labelNotSupportedIcon->setHidden(true);
        ui->labelNotSupported->setHidden(true);
        ui->labelCharge->setHidden(false);
        ui->chargeIndicator->setHidden(false);
        ui->checkBoxResyncAll->setHidden(false);
        ui->buttonSyncNow->setHidden(false);
    }
}

void MainWindow::deviceRemoved(void)
{
    ui->labelDeviceDetected->setText(tr("No device detected"));
    ui->labelSerial->setText("");
    ui->labelNotSupportedIcon->setHidden(true);
    ui->labelNotSupported->setHidden(true);
    ui->labelCharge->setHidden(true);
    ui->chargeIndicator->setHidden(true);
    ui->checkBoxResyncAll->setHidden(true);
    ui->buttonSyncNow->setHidden(true);
    ui->syncProgressBar->setHidden(true);
}

void MainWindow::deviceCharge(quint8 percent)
{
    ui->chargeIndicator->setValue(percent);
}

void MainWindow::syncFinished(bool success)
{
    if (currentLogMessageRow != NULL) {
        currentLogMessageRow->setStatus(LogMessageRow::StatusSuccess);
    }
    if (success) {
        currentLogMessageRow = new LogMessageRow(0);
        currentLogMessageRow->setMessage(tr("Syncronization complete"));
        currentLogMessageRow->setStatus(LogMessageRow::StatusSuccess);
        ui->verticalLayoutLogMessages->addLayout(currentLogMessageRow);
    }
    else {
        currentLogMessageRow = new LogMessageRow(0);
        currentLogMessageRow->setMessage(tr("Syncronization failed"));
        currentLogMessageRow->setStatus(LogMessageRow::StatusFailed);
        ui->verticalLayoutLogMessages->addLayout(currentLogMessageRow);
    }
    ui->checkBoxResyncAll->setChecked(false);
    ui->checkBoxResyncAll->setEnabled(true);
    ui->buttonSyncNow->setEnabled(true);
    ui->syncProgressBar->setHidden(true);

    updateLogList();
}

void MainWindow::syncProgressInform(QString message, bool newRow, quint8 percentDone)
{
    if (newRow) {
        if (currentLogMessageRow != NULL) {
            currentLogMessageRow->setStatus(LogMessageRow::StatusSuccess);
        }
        currentLogMessageRow = new LogMessageRow(0);
        currentLogMessageRow->setMessage(message);
        currentLogMessageRow->setStatus(LogMessageRow::StatusRunning);
        ui->verticalLayoutLogMessages->addLayout(currentLogMessageRow);
    }
    else {
        if (currentLogMessageRow != NULL) {
            currentLogMessageRow->setMessage(message);
        }
    }
    ui->syncProgressBar->setValue(percentDone);
}

void MainWindow::logItemSelected(QListWidgetItem *current,QListWidgetItem *previous)
{
    LogEntry *logEntry = NULL;

    Q_UNUSED(previous);

    if (current != NULL) {
        logEntry = logStore.read(current->data(Qt::UserRole).toString());

        delete logEntry;
    }
}

void MainWindow::showContextMenuForLogItem(const QPoint &pos)
{
    QMenu contextMenu(tr("Context menu"), this);
    QAction *action = new QAction(tr("Write Movescount file"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(logItemWriteMovescount()));
    contextMenu.addAction(action);
    contextMenu.exec(mapToGlobal(pos));
}

void MainWindow::logItemWriteMovescount()
{
    LogEntry *logEntry = NULL;

    logEntry = logStore.read(ui->logsList->selectedItems().at(0)->data(Qt::UserRole).toString());
    if (logEntry != NULL) {
        movesCount.sendLog(logEntry);
        delete logEntry;
    }
}

void MainWindow::updateLogList()
{
    QList<LogStore::LogDirEntry> entries = logStore.dir();
    ui->logsList->clear();
    foreach (LogStore::LogDirEntry entry, entries) {
        QListWidgetItem *item = new QListWidgetItem(entry.time.toString());
        item->setText(entry.time.toString());
        item->setData(Qt::UserRole, QVariant(entry.filename));
        ui->logsList->addItem(item);
    }
}

MainWindow::LogMessageRow::LogMessageRow(QWidget *parent) :
    QHBoxLayout(parent)
{
    iconLabel = new QLabel(parent);
    iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    textLabel = new QLabel(parent);
    this->addWidget(iconLabel);
    this->addWidget(textLabel);
}

MainWindow::LogMessageRow::~LogMessageRow()
{
    this->removeWidget(iconLabel);
    this->removeWidget(textLabel);
    delete iconLabel;
    delete textLabel;
}

void MainWindow::LogMessageRow::setMessage(QString message)
{
    textLabel->setText(message);
}

void MainWindow::LogMessageRow::setStatus(Status status)
{
    QIcon icon;

    if (status == StatusRunning) {
        icon = QIcon::fromTheme("task-ongoing");
    }
    else if (status == StatusSuccess) {
        icon = QIcon::fromTheme("task-complete");
    }
    else if (status == StatusFailed) {
        icon = QIcon::fromTheme("task-reject");
    }
    iconLabel->setPixmap(icon.pixmap(8,8));
}
