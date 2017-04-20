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

#include <QCloseEvent>
#include <QDebug>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QDesktopServices>

#define APPKEY                 "HpF9f1qV5qrDJ1hY1QK1diThyPsX10Mh4JvCw9xVQSglJNLdcwr3540zFyLzIC3e"
#define MOVESCOUNT_DEFAULT_URL "https://uiservices.movescount.com/"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    forceClose(false),
    movesCount(NULL),
    currentLogMessageRow(NULL)
{
    ui->setupUi(this);
    ui->actionE_xit->setShortcut(QKeySequence(QKeySequence::Quit));

    // Setup UI parts
    QIcon warningIcon = QIcon::fromTheme("dialog-warning");
    QIcon infoIcon = QIcon::fromTheme("dialog-information");
    ui->labelNotSupportedIcon->setPixmap(warningIcon.pixmap(8,8));
    ui->labelNotSupportedIcon->setHidden(true);
    ui->labelNotSupported->setHidden(true);
    ui->labelMovescountAuthIcon->setPixmap(warningIcon.pixmap(8,8));
    ui->labelMovescountAuthIcon->setHidden(true);
    ui->labelMovescountAuth->setHidden(true);
    ui->labelNewFirmwareIcon->setPixmap(infoIcon.pixmap(8,8));
    ui->labelNewFirmwareIcon->setHidden(true);
    ui->labelNewFirmware->setHidden(true);
    ui->labelCharge->setHidden(true);
    ui->chargeIndicator->setHidden(true);
    ui->checkBoxResyncAll->setHidden(true);
    ui->buttonSyncNow->setHidden(true);
    ui->syncProgressBar->setHidden(true);


    //check if there is a settings to skip the beta check
    settings.beginGroup("generalSettings");
    bool skip;
    skip = settings.value("skipBetaCheck", false).toBool();
    settings.endGroup();
    if (! skip){
        confirmBetaDialog = new ConfirmBetaDialog(this);
        if (confirmBetaDialog->exec() == false){
            // exit if user doesn't accept the early beta dialog
            exit(1);
        }
    }

    // System tray icon
    trayIconSyncAction = new QAction(QIcon::fromTheme("view-refresh"), tr("Sync now"), this);
    trayIconSyncAction->setDisabled(true);
    trayIconMinimizeRestoreAction = new QAction(tr("Minimize"), this);
    connect(trayIconSyncAction, SIGNAL(triggered()), this, SLOT(syncNowClicked()));
    connect(trayIconMinimizeRestoreAction, SIGNAL(triggered()), this, SLOT(showHideWindow()));

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(trayIconSyncAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(trayIconMinimizeRestoreAction);
    trayIconMenu->addAction(ui->actionE_xit);
    trayIcon = new QSystemTrayIcon(QIcon(":/icon_disconnected"), this);
    trayIcon->setContextMenu(trayIconMenu);
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)));
    trayIcon->setVisible(true);

    // Setup device manager
    deviceManager = new DeviceManager();
    deviceManager->moveToThread(&deviceWorkerThread);
    qRegisterMetaType<DeviceInfo>("DeviceInfo");
    connect(deviceManager, SIGNAL(deviceDetected(const DeviceInfo&)), this, SLOT(deviceDetected(const DeviceInfo&)), Qt::QueuedConnection);
    connect(deviceManager, SIGNAL(deviceRemoved()), this, SLOT(deviceRemoved()), Qt::QueuedConnection);
    connect(deviceManager, SIGNAL(deviceCharge(quint8)), this, SLOT(deviceCharge(quint8)), Qt::QueuedConnection);
    connect(deviceManager, SIGNAL(syncFinished(bool)), this, SLOT(syncFinished(bool)), Qt::QueuedConnection);
    connect(deviceManager, SIGNAL(syncProgressInform(QString,bool,bool,quint8)), this, SLOT(syncProgressInform(QString,bool,bool,quint8)), Qt::QueuedConnection);
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

    // Setup Movescount
    movesCountSetup();
}

MainWindow::~MainWindow()
{
    deviceWorkerThread.quit();
    deviceWorkerThread.wait();
    delete deviceManager;

    if (movesCount != NULL) {
        movesCount->exit();
    }

    delete trayIcon;
    delete trayIconMinimizeRestoreAction;
    delete trayIconSyncAction;
    delete trayIconMenu;

    delete ui;
}

void MainWindow::singleApplicationMsgRecv(QString msg)
{
    if (msg == "focus") {
        // Another instance of application has asked use to gain focus, let's do so!
        if (sysTraySupported() && isHidden()) {
            showHideWindow();
        }
        else {
            raise();
            activateWindow();
        }
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (sysTraySupported() && isMinimized()) {
            QTimer::singleShot(0, this, SLOT(hide()));
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    trayIconMinimizeRestoreAction->setText(tr("Minimize"));
    event->accept();
}

void MainWindow::hideEvent(QHideEvent *event)
{
    trayIconMinimizeRestoreAction->setText(tr("Restore"));
    event->accept();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //check if there is a settings for running in background
    settings.beginGroup("generalSettings");
    bool RunInBg;
    RunInBg = settings.value("runningBackground", true).toBool();
    settings.endGroup();

    if (!sysTraySupported() || forceClose || !RunInBg) {
        trayIcon->setVisible(false);
        event->accept();
    }
    else {
        showHideWindow();
        event->ignore();
    }
}

void MainWindow::closeRequested()
{
    forceClose = true;
    close();
}

void MainWindow::showHideWindow()
{
    if (!sysTraySupported() || isHidden()) {
        showNormal();
    }
    else {
        showMinimized();
    }
}

void MainWindow::trayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        showHideWindow();
    }
}

void MainWindow::showSettings()
{
    settingsDialog = new SettingsDialog(this);
    settingsDialog->setModal(true);
    connect(settingsDialog, SIGNAL(settingsSaved()), this, SLOT(settingsSaved()));
    settingsDialog->show();
}
void MainWindow::showReportBug()
{
    QDesktopServices::openUrl(QUrl("https://github.com/openambitproject/openambit/issues"));
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About %1").arg(QCoreApplication::applicationName()),
                       tr("<h2>%1</h2><b>Version %2</b><br />Using Qt %3").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion()).arg(QString(qVersion())) +
                       "<br /><br /><a href=\"http://openambit.org/\">http://openambit.org</a>");
}

void MainWindow::settingsSaved()
{
    // Update Movescount
    movesCountSetup();
}

void MainWindow::syncNowClicked()
{
    startSync();
}

void MainWindow::deviceDetected(const DeviceInfo& deviceInfo)
{
    if (0 != deviceInfo.access_status) {
        ui->labelNotSupported->setText(strerror(deviceInfo.access_status));
    }
    else {
        // FIXME Should be gotten from the UI file, really
        ui->labelNotSupported->setText(tr("Device not supported yet!"));
    }
    ui->labelDeviceDetected->setText(deviceInfo.name);
    ui->labelSerial->setText(deviceInfo.serial);
    trayIcon->setIcon(QIcon(":/icon_connected"));
    if (0 != deviceInfo.access_status || !deviceInfo.is_supported) {
        ui->labelNotSupportedIcon->setHidden(false);
        ui->labelNotSupported->setHidden(false);
        ui->labelMovescountAuthIcon->setHidden(true);
        ui->labelMovescountAuth->setHidden(true);
        ui->labelNewFirmwareIcon->setHidden(true);
        ui->labelNewFirmware->setHidden(true);
        ui->labelCharge->setHidden(true);
        ui->chargeIndicator->setHidden(true);
        ui->checkBoxResyncAll->setHidden(true);
        ui->buttonSyncNow->setHidden(true);
        trayIconSyncAction->setDisabled(true);
        ui->syncProgressBar->setHidden(true);
    }
    else {
        ui->labelNotSupportedIcon->setHidden(true);
        ui->labelNotSupported->setHidden(true);
        ui->labelMovescountAuthIcon->setHidden(true);
        ui->labelMovescountAuth->setHidden(true);
        ui->labelNewFirmwareIcon->setHidden(true);
        ui->labelNewFirmware->setHidden(true);
        ui->labelCharge->setHidden(false);
        ui->chargeIndicator->setHidden(false);
        ui->checkBoxResyncAll->setHidden(false);
        ui->buttonSyncNow->setHidden(false);
        trayIconSyncAction->setDisabled(false);

        movesCountSetup();
        if (movesCount != NULL) {
            movesCount->setDevice(deviceInfo);
            settings.beginGroup("movescountSettings");
            if (settings.value("checkNewVersions", true).toBool()) {
                movesCount->checkLatestFirmwareVersion();
            }
            if (settings.value("movescountEnable", false).toBool()) {
                movesCount->getDeviceSettings();
            }
            settings.endGroup();
        }

        settings.beginGroup("syncSettings");
        bool syncAutomatically = settings.value("syncAutomatically", false).toBool();
        settings.endGroup();
        if (syncAutomatically) {
            startSync();
        }
    }
}

void MainWindow::deviceRemoved(void)
{
    ui->labelDeviceDetected->setText(tr("No device detected"));
    ui->labelSerial->setText("");
    ui->labelNotSupportedIcon->setHidden(true);
    ui->labelNotSupported->setHidden(true);
    ui->labelMovescountAuthIcon->setHidden(true);
    ui->labelMovescountAuth->setHidden(true);
    ui->labelNewFirmwareIcon->setHidden(true);
    ui->labelNewFirmware->setHidden(true);
    ui->labelCharge->setHidden(true);
    ui->chargeIndicator->setHidden(true);
    ui->checkBoxResyncAll->setHidden(true);
    ui->buttonSyncNow->setHidden(true);
    trayIconSyncAction->setDisabled(true);
    ui->syncProgressBar->setHidden(true);

    trayIcon->toolTip();
    trayIcon->setIcon(QIcon(":/icon_disconnected"));
}

void MainWindow::deviceCharge(quint8 percent)
{
    ui->chargeIndicator->setValue(percent);
    trayIcon->setToolTip(QString(tr("Charging %1%")).arg(percent));
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
        if (isHidden()) {
            trayIcon->showMessage(QCoreApplication::applicationName(), tr("Syncronisation finished"));
        }
    }
    else {
        currentLogMessageRow = new LogMessageRow(0);
        currentLogMessageRow->setMessage(tr("Syncronization failed"));
        currentLogMessageRow->setStatus(LogMessageRow::StatusFailed);
        ui->verticalLayoutLogMessages->addLayout(currentLogMessageRow);
        if (isHidden()) {
            trayIcon->showMessage(QCoreApplication::applicationName(), tr("Syncronisation failed"), QSystemTrayIcon::Critical);
        }
    }
    ui->checkBoxResyncAll->setChecked(false);
    ui->checkBoxResyncAll->setEnabled(true);
    ui->buttonSyncNow->setEnabled(true);
    trayIconSyncAction->setEnabled(true);
    ui->syncProgressBar->setHidden(true);

    trayIcon->setIcon(QIcon(":/icon_connected"));

    updateLogList();
}

void MainWindow::syncProgressInform(QString message, bool error, bool newRow, quint8 percentDone)
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
            if (error) {
                currentLogMessageRow->setStatus(LogMessageRow::StatusFailed);
            }
        }
    }
    ui->syncProgressBar->setValue(percentDone);
    trayIcon->setToolTip(QString(tr("Downloading %1%")).arg(percentDone));
}

void MainWindow::newerFirmwareExists(QByteArray fw_version)
{
    ui->labelNewFirmware->setText(QString(tr("Newer firmware exists (%1.%2.%3)")).arg((int)fw_version[0]).arg((int)fw_version[1]).arg((int)(fw_version[2])));
    ui->labelNewFirmware->setHidden(false);
    ui->labelNewFirmwareIcon->setHidden(false);
}

void MainWindow::movesCountAuth(bool authorized)
{
    ui->labelMovescountAuth->setHidden(authorized);
    ui->labelMovescountAuthIcon->setHidden(authorized);
}

void MainWindow::logItemSelected(QListWidgetItem *current,QListWidgetItem *previous)
{
    LogEntry *logEntry = NULL;

    Q_UNUSED(previous);

    if (current != NULL) {
        logEntry = logStore.read(current->data(Qt::UserRole).toString());
        if (logEntry != NULL) {
            ui->logDetail->showLog(logEntry);
        }

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
        if (movesCount != NULL) {
            movesCount->writeLog(logEntry);
        }
        movesCountXML.writeLog(logEntry);
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
        ui->logsList->insertItem(0, item);
    }
}

void MainWindow::startSync()
{
    ui->checkBoxResyncAll->setEnabled(false);
    ui->buttonSyncNow->setEnabled(false);
    trayIconSyncAction->setEnabled(false);
    currentLogMessageRow = NULL;
    QLayoutItem *tmpItem;
    while ((tmpItem = ui->verticalLayoutLogMessages->takeAt(0)) != NULL) {
        delete tmpItem->widget();
        delete tmpItem;
    }
    ui->syncProgressBar->setHidden(false);
    ui->syncProgressBar->setValue(0);

    trayIcon->setIcon(QIcon(":/icon_syncing"));
    if (isHidden()) {
        trayIcon->showMessage(QCoreApplication::applicationName(), tr("Syncronisation started"));
    }
    emit MainWindow::syncNow(ui->checkBoxResyncAll->isChecked());
}

void MainWindow::movesCountSetup()
{
    bool syncOrbit = false;
    bool syncSportMode = false;
    bool syncNavigation = false;
    bool movescountEnable = false;

    settings.beginGroup("syncSettings");
    syncOrbit = settings.value("syncOrbit", true).toBool();
    syncSportMode = settings.value("syncSportMode", false).toBool();
    syncNavigation = settings.value("syncNavigation", false).toBool();
    settings.endGroup();

    settings.beginGroup("movescountSettings");
    movescountEnable = settings.value("movescountEnable", false).toBool();
    if (syncOrbit || syncSportMode || syncNavigation || movescountEnable) {
        if (movesCount == NULL) {
            movesCount = MovesCount::instance();
            movesCount->setAppkey(APPKEY);
            movesCount->setBaseAddress(settings.value("movescountBaseAddress", MOVESCOUNT_DEFAULT_URL).toString());
            if (settings.value("movescountUserkey", "").toString().length() == 0) {
                settings.setValue("movescountUserkey", movesCount->generateUserkey());
            }
            movesCount->setUserkey(settings.value("movescountUserkey").toString());

            connect(movesCount, SIGNAL(newerFirmwareExists(QByteArray)), this, SLOT(newerFirmwareExists(QByteArray)), Qt::QueuedConnection);
            connect(movesCount, SIGNAL(movesCountAuth(bool)), this, SLOT(movesCountAuth(bool)), Qt::QueuedConnection);
        }
        if (movescountEnable) {
            movesCount->setUsername(settings.value("email").toString());
        }
    }
    settings.endGroup();
}

bool MainWindow::sysTraySupported()
{
    return QSystemTrayIcon::isSystemTrayAvailable();
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
    QString str_icon = "";

    if (status == StatusRunning) {
        if(QIcon::hasThemeIcon("test-ongoing")) {
            icon = QIcon::fromTheme("task-ongoing");
        } else {
            str_icon = QChar(0x1a,0x23);
        }
    }
    else if (status == StatusSuccess) {
        if(QIcon::hasThemeIcon("task-complete")) {
            icon = QIcon::fromTheme("task-complete");
        } else {
            str_icon = QChar(0x13,0x27);
        }
    }
    else if (status == StatusFailed) {
        if(QIcon::hasThemeIcon("task-reject")) {
            icon = QIcon::fromTheme("task-reject");
        } else {
            str_icon = QChar(0x17,0x27);
        }
    }

    if(str_icon != "") {
        iconLabel->setText(str_icon);
    } else {
        iconLabel->setPixmap(icon.pixmap(8,8));
    }
}

