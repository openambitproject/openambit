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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "devicemanager.h"
#include "settingsdialog.h"
#include "confirmbetadialog.h"
#include <movescount/deviceinfo.h>
#include <movescount/movescount.h>
#include <QMainWindow>
#include <QThread>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSystemTrayIcon>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void syncNow(bool readAll);

public slots:
    void singleApplicationMsgRecv(QString msg);
    void closeRequested();

protected:
    void changeEvent(QEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    void closeEvent(QCloseEvent *event);

private slots:
    void showHideWindow();
    void trayIconClicked(QSystemTrayIcon::ActivationReason reason);

    void showSettings();
    void showReportBug();
    void showAbout();
    void settingsSaved();

    void syncNowClicked();

    void deviceDetected(const DeviceInfo& deviceInfo);
    void deviceRemoved();
    void deviceCharge(quint8 percent);
    void syncFinished(bool success);
    void syncProgressInform(QString message, bool error, bool newRow, quint8 percentDone);

    void newerFirmwareExists(QByteArray fw_version);
    void movesCountAuth(bool authorized);
    void showUploadError(QByteArray data);

    void logItemSelected(QListWidgetItem *current,QListWidgetItem *previous);
    void showContextMenuForLogItem(const QPoint &pos);
    void logItemWriteMovescount();
    void updateLogList();
    
private:
    void startSync();

    void movesCountSetup();

    bool sysTraySupported();

    Ui::MainWindow *ui;
    bool forceClose;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QAction *trayIconSyncAction;
    QAction *trayIconMinimizeRestoreAction;

    Settings settings;
    SettingsDialog *settingsDialog;
    ConfirmBetaDialog *confirmBetaDialog;
    DeviceManager *deviceManager;
    LogStore logStore;
    MovesCountXML movesCountXML;
    MovesCount *movesCount;
    QThread deviceWorkerThread;

    class LogMessageRow : public QHBoxLayout
    {
    public:
        enum Status {
            StatusRunning,
            StatusSuccess,
            StatusFailed
        };
        explicit LogMessageRow(QWidget *parent);
        ~LogMessageRow();

        void setMessage(QString message);
        void setStatus(Status status);

    private:
        QLabel *iconLabel;
        QLabel *textLabel;
    };

    LogMessageRow *currentLogMessageRow;
};

#endif // MAINWINDOW_H
