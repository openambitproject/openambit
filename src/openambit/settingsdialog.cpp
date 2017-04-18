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
#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    readSettings();

    showHideUserSettings();

    connect(ui->listSettingGroups, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));
    connect(ui->checkBoxMovescountEnable, SIGNAL(clicked()), this, SLOT(showHideUserSettings()));
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    ui->stackedWidget->setCurrentIndex(ui->listSettingGroups->row(current));
}

void SettingsDialog::accept()
{
    writeSettings();
    this->close();
}

void SettingsDialog::showHideUserSettings()
{
    if (ui->checkBoxMovescountEnable->isChecked()) {
        ui->lineEditEmail->setHidden(false);
    }
    else {
        ui->lineEditEmail->setHidden(true);
    }
}

void SettingsDialog::readSettings()
{
    settings.beginGroup("generalSettings");
    ui->checkBoxSkipBetaCheck->setChecked(settings.value("skipBetaCheck", false).toBool());
    ui->checkBoxRunningBackground->setChecked(settings.value("runningBackground", true).toBool());
    settings.endGroup();

    settings.beginGroup("syncSettings");
    ui->checkBoxSyncAutomatically->setChecked(settings.value("syncAutomatically", false).toBool());
    ui->checkBoxSyncTime->setChecked(settings.value("syncTime", true).toBool());
    ui->checkBoxSyncOrbit->setChecked(settings.value("syncOrbit", true).toBool());
    ui->checkBoxSyncSportsMode->setChecked(settings.value("syncSportMode", false).toBool());
    ui->checkBoxSyncNavigation->setChecked(settings.value("syncNavigation", false).toBool());
    settings.endGroup();

    settings.beginGroup("movescountSettings");
    ui->checkBoxNewVersions->setChecked(settings.value("checkNewVersions", true).toBool());
    ui->checkBoxMovescountEnable->setChecked(settings.value("movescountEnable", false).toBool());
    ui->lineEditEmail->setText(settings.value("email", "").toString());
    ui->checkBoxDebugFiles->setChecked(settings.value("storeDebugFiles", true).toBool());
    settings.endGroup();
}

void SettingsDialog::writeSettings()
{
    settings.beginGroup("generalSettings");
    settings.setValue("skipBetaCheck", ui->checkBoxSkipBetaCheck->isChecked());
    settings.setValue("runningBackground", ui->checkBoxRunningBackground->isChecked());
    settings.endGroup();

    settings.beginGroup("syncSettings");
    settings.setValue("syncAutomatically", ui->checkBoxSyncAutomatically->isChecked());
    settings.setValue("syncTime", ui->checkBoxSyncTime->isChecked());
    settings.setValue("syncOrbit", ui->checkBoxSyncOrbit->isChecked());
    settings.setValue("syncSportMode", ui->checkBoxSyncSportsMode->isChecked());
    settings.setValue("syncNavigation", ui->checkBoxSyncNavigation->isChecked());
    settings.endGroup();

    settings.beginGroup("movescountSettings");
    settings.setValue("checkNewVersions", ui->checkBoxNewVersions->isChecked());
    settings.setValue("movescountEnable", ui->checkBoxMovescountEnable->isChecked());
    if (ui->checkBoxMovescountEnable->isChecked()) {
        settings.setValue("email", ui->lineEditEmail->text());
    }
    else {
        settings.setValue("email", "");
    }
    settings.setValue("storeDebugFiles", ui->checkBoxDebugFiles->isChecked());
    settings.endGroup();

    emit settingsSaved();
}
