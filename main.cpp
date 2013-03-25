// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "main.h"
#include "mainwindow.h"
#include <QApplication>

const QString Company = "c't";
const QString AppName = "GLSL Live Coder";
const QString AppUrl = "http://code.google.com/p/glsl-live/";
const QString AppAuthor = "Oliver Lau";
const QString AppAuthorMail = "ola@ct.de";
const QString AppVersionNoDebug = "0.1";
const QString AppMinorVersion = "";
#ifdef QT_NO_DEBUG
const QString AppVersion = AppVersionNoDebug + AppMinorVersion;
#else
const QString AppVersion = AppVersionNoDebug + AppMinorVersion + " [DEBUG]";
#endif


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName(Company);
    a.setOrganizationDomain(Company);
    a.setApplicationName(AppName);
    a.setApplicationVersion(AppVersionNoDebug);
    MainWindow w;
    w.show();
    return a.exec();
}
