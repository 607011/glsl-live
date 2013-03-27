// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "project.h"
#include "main.h"
#include <QBuffer>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QtCore/QDebug>
#include <QByteArray>

Project::Project(QString filename, QObject* parent)
    : QObject(parent)
    , mDirty(false)
    , mWebcam(-1)
{
    if (!filename.isEmpty())
        load(filename);
}

void Project::reset()
{
    mDirty = false;
    mWebcam = -1;
    mImage = QImage();
    mVertexShaderSource = QString();
    mFragmentShaderSource = QString();
}

bool Project::save(void)
{
    Q_ASSERT(!mFilename.isEmpty());
    bool ok = save(mFilename);
    return ok;
}

bool Project::save(const QString& filename)
{
    Q_ASSERT(!filename.isEmpty());
    QFile file(filename);
    int flags = QIODevice::WriteOnly;
    bool compress = filename.endsWith("z");
    if (!compress)
        flags |= QIODevice::Text;
    bool ok = file.open((QIODevice::OpenMode)flags);
    if (!ok) {
        qWarning() << "file.open() failed.";
        return false;
    }
    QString d;
    QTextStream out(&d);
    out.setAutoDetectUnicode(false);
    out.setCodec(QTextCodec::codecForMib(106/* UTF-8 */));
    out << "<glsl-live-coder-project version=\"" << AppVersionNoDebug << "\">\n"
        << "  <shaders>\n"
        << "    <vertex><![CDATA[" << mVertexShaderSource << "]]></vertex>\n"
        << "    <fragment><![CDATA[" << mFragmentShaderSource << "]]></fragment>\n"
        << "  </shaders>\n";
    out << "  <input>\n";
    if (!mImage.isNull() || mWebcam >= 0) {
        out << "    <image>\n";
        if (!mImage.isNull()) {
            QByteArray ba;
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);
            mImage.save(&buffer, "PNG");
            buffer.close();
            out << "      <static><![CDATA[" << ba.toBase64() << "]]></static>\n";
        }
        else if (mWebcam >= 0) {
            out << "      <webcam>" << mWebcam << "</webcam>\n";
        }
        out << "    </image>\n";
    }
    out << "  </input>\n";
    out << "</glsl-live-coder-project>\n";
    if (compress) {
        file.write(qCompress(d.toUtf8(), 9));
    }
    else {
        file.write(d.toUtf8());
    }
    file.close();
    mDirty = false;
    return ok;
}

bool Project::load(const QString& filename)
{
    Q_ASSERT(!filename.isEmpty());
    mFilename = filename;
    int flags = QIODevice::ReadOnly;
    bool compressed = filename.endsWith("z");
    if (!compressed)
        flags |= QIODevice::Text;
    QFile file(mFilename);
    bool success = file.open((QIODevice::OpenMode)flags);
    if (!success) {
        qWarning() << "file.open() failed.";
        return false;
    }
    QByteArray ba = file.readAll();
    QBuffer buffer(&ba);
    buffer.open(QIODevice::ReadOnly);
    if (compressed)
        ba = qUncompress(ba);
    success = read(&buffer);
    if (!success)
        qWarning() << "Project.read(QIODevice* device) failed.";
    file.close();
    mDirty = false;
    return success;
}

bool Project::read(QIODevice* device)
{
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

void Project::read(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "glsl-live-coder-project");
    while (mXml.readNextStartElement()) {
        if (mXml.name() == "shaders") {
            readShaders();
        }
        else if (mXml.name() == "input") {
            readInput();
        }
        else {
            mXml.skipCurrentElement();
        }
    }
}

void Project::readShaders(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "shaders");
    while (mXml.readNextStartElement()) {
        if (mXml.name() == "vertex") {
            readShaderVertex();
        }
        else if (mXml.name() == "fragment") {
            readShaderFragment();
        }
        else {
            mXml.skipCurrentElement();
        }
    }
}

void Project::readShaderVertex()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "vertex");
    const QString& str = mXml.readElementText();
    if (!str.isEmpty()) {
        mVertexShaderSource = str;
    }
    else {
        mXml.raiseError(QObject::tr("empty vertex shader: %1").arg(str));
    }
}

void Project::readShaderFragment()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "fragment");
    const QString& str = mXml.readElementText();
    if (!str.isEmpty()) {
        mFragmentShaderSource = str;
    }
    else {
        mXml.raiseError(QObject::tr("empty fragment shader: %1").arg(str));
    }
}

void Project::readInput(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "input");
    while (mXml.readNextStartElement()) {
        if (mXml.name() == "image") {
            readInputImage();
        }
        else {
            mXml.skipCurrentElement();
        }
    }
}

void Project::readInputImage()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "image");
    while (mXml.readNextStartElement()) {
        if (mXml.name() == "static") {
            readInputImageData();
        }
        else {
            mXml.skipCurrentElement();
        }
    }
}

void Project::readInputImageData()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "static");
    const QString& str = mXml.readElementText();
    if (!str.isEmpty()) {
        const QByteArray& imgData = QByteArray::fromBase64(str.toUtf8());
        bool ok = mImage.loadFromData(imgData);
        if (!ok)
            qWarning() << "mImage.loadFromData() failed.";
    }
    else {
        mXml.raiseError(QObject::tr("empty fragment shader: %1").arg(str));
    }
}

void Project::readInputWebcam()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "webcam");
    const QString& str = mXml.readElementText();
    if (!str.isEmpty()) {
        bool ok = false;
        mWebcam = str.toInt(&ok);
        if (!ok)
            mXml.raiseError(QObject::tr("invalid webcam: %1").arg(str));
    }
    else {
        mXml.raiseError(QObject::tr("empty webcam tag: %1").arg(str));
    }
}
