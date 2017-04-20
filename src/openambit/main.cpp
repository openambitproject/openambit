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
#include <unistd.h>
#include "mainwindow.h"
#include <QSettings>
#include <QTranslator>
#include <QLibraryInfo>

#include "single_application.h"
#include "signalhandler.h"

static void initTranslations(void);

int main(int argc, char *argv[])
{
    // Set application settings
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QCoreApplication::setOrganizationName("Openambit");
    QCoreApplication::setApplicationName("Openambit");

    for(int x = 1; x < argc; ++x) {
        if (QString(argv[x]) == "--version") {
            printf("%s - Version %s\n", "Openambit", APP_VERSION);
            return 0;
        }
    }
    // Handle foreground arguments
    // NOTE: It would be preferable to handle all arguments at the same place,
    // but fork needs to be done before Qt initialize it seems
    bool background = true;
    for (int i=0; i<argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            background = false;
            break;
        }
    }
    if (background) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
        // Windows
#else
        // Fork for background running
        if ( fork() > 0 ) {
            // Exit the parent process
            return 0;
        }
        // Set the child to the new process group leader
        setsid();
#endif
    }

    SingleApplication a(argc, argv, "openambit_single_application_lock");

    if (a.isRunning()) {
        a.sendMessage("focus");
        return 0;
    }

    // Handle forced localisation / translation
    Q_FOREACH(QString argu, a.arguments()) {
        const static QString localeParam = "-locale:";
        if (argu.startsWith(localeParam)) {
           QLocale::setDefault(QLocale(argu.mid(sizeof(localeParam))));
           break;
        }
    }

    // Initialize translations
    initTranslations();

    MainWindow w;

    // Connect single application message bus
    QObject::connect(&a, SIGNAL(messageAvailable(QString)), &w, SLOT(singleApplicationMsgRecv(QString)));

    // Handle signals
    SignalHandler sigHandler;
    QObject::connect(&sigHandler, SIGNAL(signalReceived(int)), &w, SLOT(closeRequested()));

    w.show();
    
    return a.exec();
}

static void initTranslations(void)
{
    QLocale locale;
    QTranslator *qtTranslator = new QTranslator();
    qtTranslator->load("qt_" + locale.name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    if (qtTranslator->isEmpty()) {
        delete qtTranslator;
    }
    else {
        qApp->installTranslator(qtTranslator);
    }

    QTranslator *openambitTranslator = new QTranslator();
    openambitTranslator->load(":/translations/openambit_" + locale.name());
    if (openambitTranslator->isEmpty()) {
        delete openambitTranslator;
    }
    else {
        qApp->installTranslator(openambitTranslator);
    }
}
