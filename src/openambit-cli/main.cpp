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
#include <QString>
#include <QtCore/QCoreApplication>
#include "Task.h"

int main(int argc, char *argv[]) {
    // Set application settings
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QCoreApplication::setOrganizationName("Openambit");
    QCoreApplication::setApplicationName("Openambit-cli");

    for (int x = 1; x < argc; ++x) {
        if (QString(argv[x]) == "--version") {
            printf("%s - Version %s\n", "Openambit", APP_VERSION);
            return 0;
        }
    }

    if (argc < 3) {
        printf ("Usage: openambit-cli <username> <userkey>");
        return 1;
    }

    QCoreApplication a(argc, argv);

    // make the app parent of the task
    Task *task = new Task(&a, argv[1], argv[2],
            true, true, true, true, true,
            /*"test-data/settings.json"*/ NULL);

    // make application stop when the task is done
    QObject::connect(task, SIGNAL(finished()), &a, SLOT(quit()));

    // This will run the task from the application event loop.
    QTimer::singleShot(0, task, SLOT(run()));

    return QCoreApplication::exec();
}
