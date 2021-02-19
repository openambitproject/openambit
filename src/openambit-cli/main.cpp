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
#include <QtCore/QCommandLineParser>
#include "Task.h"

int main(int argc, char *argv[]) {
    // Set application settings
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QCoreApplication::setOrganizationName("Openambit");
    QCoreApplication::setApplicationName("Openambit-cli");

    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("\nCommandline application to perform sync between the watch and movescount.com.\n"
                                     "\n"
                                     "Certain features/syncs can be disabled via commandline options.\n"
                                     "For syncing to movescount.com you need to provide the username and userkey as used by OpenAmbit GUI\n");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("username", QCoreApplication::translate("main", "Username for connecting to Movescount."));
    parser.addPositionalArgument("userkey", QCoreApplication::translate("main", "User-key for connecting to Movescount."));

    QCommandLineOption noReadLogsOption(QStringList() << "r" << "no-read-logs",
                                        QCoreApplication::translate("main", "Do not read logs from the watch."));
    parser.addOption(noReadLogsOption);

    QCommandLineOption noSyncTimeOption(QStringList() << "t" << "no-sync-time",
                                        QCoreApplication::translate("main", "Do not sync the time on the watch."));
    parser.addOption(noSyncTimeOption);

    QCommandLineOption noSyncOrbitOption(QStringList() << "o" << "no-sync-orbit",
                                         QCoreApplication::translate("main", "Do not sync GPS orbits on the watch."));
    parser.addOption(noSyncOrbitOption);

    QCommandLineOption noSyncSportModeOption(QStringList() << "s" << "no-sync-sport-mode",
                                             QCoreApplication::translate("main", "Do not sync sport modes to the watch."));
    parser.addOption(noSyncSportModeOption);

    QCommandLineOption noSyncNavigationOption(QStringList() << "n" << "no-sync-navigation",
                                             QCoreApplication::translate("main", "Do not sync navigation to the watch."));
    parser.addOption(noSyncNavigationOption);

    QCommandLineOption noWriteLogs(QStringList() << "l" << "no-write-logs",
                                             QCoreApplication::translate("main", "Do not send move-logs to movescount.com."));
    parser.addOption(noWriteLogs);

    QCommandLineOption writeJSONSettingsFileOption(QStringList() << "w" << "write-config-json",
                                                   QCoreApplication::translate("main", "Write watch-settings to files 'apprules.json' and 'settings.json' in the settings directory at ~/.openambit."));
    parser.addOption(writeJSONSettingsFileOption);

    QCommandLineOption customConfigFileOption(QStringList() << "c" << "custom-config",
                                              QCoreApplication::translate("main", "A custom JSON config file to load settings for the watch. Not applied when --no-syn-sport-mode is specified."),
                                              QCoreApplication::translate("main", "json-file"));
    parser.addOption(customConfigFileOption);

    QCommandLineOption customAppFileOption(QStringList() << "a" << "app-config",
                                              QCoreApplication::translate("main", "A custom JSON config file to load apps for the watch. Not applied when --no-syn-sport-mode is specified."),
                                              QCoreApplication::translate("main", "json-file"));
    parser.addOption(customAppFileOption);

    // Process the actual command line arguments given by the user
    parser.process(a);

    const QStringList args = parser.positionalArguments();
    // source is args.at(0), destination is args.at(1)

    const QString customConfig = parser.value(customConfigFileOption);

    // we have to store the parameters as std::string here
    // to not have them deleted before we use them
    std::string customConfigStr;
    if(customConfig.length() > 0) {
        customConfigStr = customConfig.toStdString();
    }

    const QString customAppConfig = parser.value(customAppFileOption);

    // we have to store the parameters as std::string here
    // to not have them deleted before we use them
    std::string customAppConfigStr;
    if(customAppConfig.length() > 0) {
        customAppConfigStr = customAppConfig.toStdString();
    }

    std::string username;
    if(args.length() >= 1) {
        username = args.at(0).toStdString();
    }

    std::string userkey;
    if(args.length() >= 2) {
        userkey = args.at(1).toStdString();
    }

    // make the app parent of the task
    Task *task = new Task(&a,
            args.length() < 1 ? NULL : username.c_str(),
            args.length() < 2 ? NULL : userkey.c_str(),
            !parser.isSet(noReadLogsOption),
            !parser.isSet(noSyncTimeOption),
            !parser.isSet(noSyncOrbitOption),
            !parser.isSet(noSyncSportModeOption),
            !parser.isSet(noSyncNavigationOption),
            !parser.isSet(noWriteLogs),
            parser.isSet(writeJSONSettingsFileOption),
            customConfig.length() == 0 ? NULL : customConfigStr.c_str(),
            customAppConfig.length() == 0 ? NULL : customAppConfigStr.c_str());

    // make application stop when the task is done
    QObject::connect(task, SIGNAL(finished()), &a, SLOT(quit()));

    // This will run the task from the application event loop.
    QTimer::singleShot(0, task, SLOT(run()));

    return QCoreApplication::exec();
}
