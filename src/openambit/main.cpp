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
#include <QSettings>

#include "single_application.h"

int main(int argc, char *argv[])
{
    SingleApplication a(argc, argv, "openambit_single_application_lock");

    if (a.isRunning()) {
        a.sendMessage("focus");
        return 0;
    }

    // Set application settings
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QCoreApplication::setOrganizationName("Openambit");
    QCoreApplication::setApplicationName("Openambit");

    MainWindow w;

    // Connect single application message bus
    QObject::connect(&a, SIGNAL(messageAvailable(QString)), &w, SLOT(singleApplicationMsgRecv(QString)));

    w.show();
    
    return a.exec();
}
