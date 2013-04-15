/*
    GLSL Live Coder - Live Coding in the GL Shading Language
    Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QApplication>
#include <QTranslator>
#include <QLocale>

#ifndef QT_NO_DEBUG
#include <QtCore/QDebug>
#endif

#include "main.h"
#include "mainwindow.h"

const QString Company = "c't";
const QString AppName = "GLSL Live Coder";
const QString AppUrl = "http://code.google.com/p/glsl-live/";
const QString AppAuthor = "Oliver Lau";
const QString AppAuthorMail = "ola@ct.de";
const QString AppVersionNoDebug = "0.9.7";
const QString AppMinorVersion = "";
#ifdef QT_NO_DEBUG
const QString AppVersion = AppVersionNoDebug + AppMinorVersion;
#else
const QString AppVersion = AppVersionNoDebug + AppMinorVersion + " [DEBUG]";
#endif


int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName(Company);
    a.setOrganizationDomain(Company);
    a.setApplicationName(AppName);
    a.setApplicationVersion(AppVersionNoDebug);
    QCoreApplication::addLibraryPath("plugins");
    QCoreApplication::addLibraryPath("./plugins");

#ifdef Q_OS_MAC
    QCoreApplication::addLibraryPath("../plugins");
#endif

#ifndef QT_NO_DEBUG
    qDebug() << QCoreApplication::libraryPaths();
#endif

    QTranslator translator;
    bool ok = translator.load(":/translations/glsl-live_" + QLocale::system().name());
#ifndef QT_NO_DEBUG
    if (!ok)
        qWarning() << "Could not load translations for" << QLocale::system().name() << "locale";
#endif
    if (ok)
        a.installTranslator(&translator);
    MainWindow w;
    w.show();
    return a.exec();
}
