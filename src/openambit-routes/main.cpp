/*
 * (C) Copyright 2022 Dominik Stadler
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
#include <QtCore/QCommandLineParser>
#include "Task.h"

int main(int argc, char *argv[]) {
    // Set application settings
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QCoreApplication::setOrganizationName("Openambit");
    QCoreApplication::setApplicationName("Openambit-routes");

    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("\nCommandline application to store routes on an Ambit watch\n");
    parser.addHelpOption();
    parser.addVersionOption();

    // Process the actual command line arguments given by the user
    parser.process(a);

    const QStringList args = parser.positionalArguments();
    // directory is args.at(0)

    if (args.length() != 1) {
        printf("\nError: Expecting a single argument with the directory where the route-files and personal_settings.json are stored\n\n");
        return 1;
    }

    // make the app parent of the task
    std::string directory = args.at(0).toStdString();
    Task *task = new Task(&a, directory.c_str());

    // make application stop when the task is done
    QObject::connect(task, SIGNAL(finished()), &a, SLOT(quit()));

    // This will run the task from the application event loop.
    QTimer::singleShot(0, task, SLOT(run()));

    return QCoreApplication::exec();
}
