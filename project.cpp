// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "project.h"
#include "main.h"
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QtCore/QDebug>
#include <QByteArray>

Project::Project(QObject* parent)
    : QObject(parent)
{
    /* ... */
}

bool Project::save(const QString& fileName)
{
    Q_ASSERT(!fileName.isEmpty());
    QFile file(fileName);
    bool ok = file.open(QIODevice::WriteOnly | QIODevice::Text);
    if (!ok)
        return false;
    QTextStream out(&file);
    out.setAutoDetectUnicode(false);
    out.setCodec(QTextCodec::codecForMib(106/* UTF-8 */));
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        << "<glsl-liver-coder-project version=\"" << AppVersionNoDebug << "\">\n"
        << "</glsl-liver-coder-project>\n";
    file.close();
    return ok;
}

bool Project::load(const QString& fileName)
{
    Q_ASSERT(!fileName.isEmpty());
    qDebug() << "Project::load(\"" << fileName << "\")";
    QFile file(fileName);
    bool success = file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!success)
        return false;
    success = read(&file);
    file.close();
    return success;
}

void Project::read(void)
{
    qDebug() << "Project::read()";
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "glsl-live-coder-project");
    while (mXml.readNextStartElement()) {
        qDebug() << ">" << mXml.name();
        if (mXml.name() == "shaders") {
            readShaders();
        }
        else if (mXml.name() == "input") {
            readInput();
        }
        else if (mXml.name() == "parameters") {
            readParameters();
        }
        else {
            mXml.skipCurrentElement();
        }
    }
}

bool Project::read(QIODevice* device)
{
    qDebug() << "Project::read(QIODevice* device)";
    Q_ASSERT(device != NULL);
    mXml.setDevice(device);
    if (mXml.readNextStartElement()) {
        if (mXml.name() == "glsl-live-coder-project" && mXml.attributes().value("version").toString().startsWith("0.")) {
            read();
        }
        else {
            mXml.raiseError(QObject::tr("The file is not an GLSL Live Coder v0.x project file."));
        }
    }
    return !mXml.error();
}

QString Project::errorString(void) const
{
    return QObject::tr("%1 (line %2, column %3)").arg(mXml.errorString()).arg(mXml.lineNumber()).arg(mXml.columnNumber());
}

void Project::readShaders(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "shaders");
    qDebug() << "Project::readShaders()";
    while (mXml.readNextStartElement()) {
        qDebug() << ">>" << mXml.name();
        if (mXml.name() == "vertex") {
            readVertexShader();
        }
        else if (mXml.name() == "fragment") {
            readFragmentShader();
        }
        else {
            mXml.skipCurrentElement();
        }
    }
}

void Project::readVertexShader()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "vertex");
    const QString& str = mXml.readElementText();
    if (!str.isEmpty()) {
        qDebug() << "Project::readVertexShader()";
        emit vertexShaderFound(str);
    }
    else
        mXml.raiseError(QObject::tr("empty vertex shader: %1").arg(str));

}

void Project::readFragmentShader()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "fragment");
    const QString& str = mXml.readElementText();
    if (!str.isEmpty()) {
        qDebug() << "Project::readFragmentShader()";
        emit fragmentShaderFound(str);
    }
    else
        mXml.raiseError(QObject::tr("empty fragment shader: %1").arg(str));
}

void Project::readInput(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "input");
    qDebug() << "Project::readInput()";
    while (mXml.readNextStartElement()) {
        qDebug() << ">>" << mXml.name();
        if (mXml.name() == "image") {
            readImage();
        }
        else {
            mXml.skipCurrentElement();
        }
    }
}

void Project::readImage()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "image");
    qDebug() << "Project::readImage()";
    while (mXml.readNextStartElement()) {
        if (mXml.name() == "static") {
            readImageData();
        }
        else {
            mXml.skipCurrentElement();
        }
    }
}

void Project::readImageData()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "static");
    const QString& str = mXml.readElementText();
    if (!str.isEmpty()) {
        qDebug() << "Project::readImageData() >>" << str;
        QImage image = QImage::fromData(QByteArray::fromBase64(str.toLatin1()));
        emit imageFound(image);
    }
    else
        mXml.raiseError(QObject::tr("empty fragment shader: %1").arg(str));
}

void Project::readWebcam()
{
}

void Project::readParameters(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "parameters");
    qDebug() << "Project::readInput()";
    while (mXml.readNextStartElement()) {
        mXml.skipCurrentElement();
    }
}

