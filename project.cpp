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

QString Project::errorString(void) const
{
    return QObject::tr("%1 (line %2, column %3)").arg(mXml.errorString()).arg(mXml.lineNumber()).arg(mXml.columnNumber());
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
    qDebug() << "Project::save()";
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
    out << "  <parameters>\n";
    for (QQueue<ParameterWidget>::const_iterator p = mParameterWidgets.constBegin(); p != mParameterWidgets.constEnd(); ++p) {
        out << "    <parameter>\n";
        switch (p->type()) {
        case ParameterWidget::Integer:
            out << "      <type>int</type>";
            break;
        case ParameterWidget::Float:
            out << "      <type>float</type>";
            break;
        case ParameterWidget::Boolean:
            out << "      <type>bool</type>";
            break;
        case ParameterWidget::None:
            // fall-through
        default:
            break;
        }
        switch (p->type()) {
        case ParameterWidget::Integer:
            out << "      <minValue>" << p->minValue().toInt() << "</minValue>";
            break;
        case ParameterWidget::Float:
            out << "      <minValue>" << p->minValue().toFloat() << "</minValue>";
            break;
        case ParameterWidget::None:
            // fall-through
        default:
            break;
        }
        switch (p->type()) {
        case ParameterWidget::Integer:
            out << "      <maxValue>" << p->maxValue().toInt() << "</maxValue>";
            break;
        case ParameterWidget::Float:
            out << "      <maxValue>" << p->maxValue().toFloat() << "</maxValue>";
            break;
        case ParameterWidget::None:
            // fall-through
        default:
            break;
        }
        switch (p->type()) {
        case ParameterWidget::Integer:
            out << "      <defaultValue>" << p->defaultValue().toInt() << "</defaultValue>";
            break;
        case ParameterWidget::Float:
            out << "      <defaultValue>" << p->defaultValue().toFloat() << "</defaultValue>";
            break;
        case ParameterWidget::Boolean:
            out << "      <defaultValue>" << p->defaultValue().toBool() << "</defaultValue>";
            break;
        case ParameterWidget::None:
            // fall-through
        default:
            break;
        }
        out << "      <name>" << p->name() << "</name>";
        out << "    </parameter>\n";
    }
    out << "  </parameters>\n";
    out << "</glsl-live-coder-project>\n";
    if (compress) {
        file.write(qCompress(d.toLatin1(), 9));
    }
    else {
        file.write(d.toLatin1());
    }
    file.close();
    mDirty = false;
    return ok;
}

bool Project::load(const QString& filename)
{
    Q_ASSERT(!filename.isEmpty());
    mFilename = filename;
    qDebug() << "Project::load(" << mFilename << ")";
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
    qDebug() << "Project::read(QIODevice* device)";
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

void Project::readShaders(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "shaders");
    qDebug() << "Project::readShaders()";
    while (mXml.readNextStartElement()) {
        qDebug() << ">>" << mXml.name();
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
        qDebug() << "Project::readShaderVertex() >>" << str;
        mVertexShaderSource = str;
    }
    else
        mXml.raiseError(QObject::tr("empty vertex shader: %1").arg(str));

}

void Project::readShaderFragment()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "fragment");
    const QString& str = mXml.readElementText();
    if (!str.isEmpty()) {
        qDebug() << "Project::readShaderFragment() >>" << str;
        mFragmentShaderSource = str;
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
    qDebug() << "Project::readImage()";
    while (mXml.readNextStartElement()) {
        if (mXml.name() == "static") {
            mImageSize.setWidth(mXml.attributes().value("width").toString().toInt());
            mImageSize.setHeight(mXml.attributes().value("height").toString().toInt());
            qDebug() << "mImageSize =" << mImageSize;
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
        qDebug() << "Project::readImageData() >>" << str;
        const QByteArray& imgData = QByteArray::fromBase64(str.toLatin1());
        qDebug() << "imgData.size() =" << imgData.size();
        bool ok = mImage.loadFromData(imgData);
        if (!ok)
            qWarning() << "mImage.loadFromData() failed.";
        qDebug() << "mImage.size() =" << mImage.size();
    }
    else
        mXml.raiseError(QObject::tr("empty fragment shader: %1").arg(str));
}

void Project::readInputWebcam()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "webcam");
    const QString& str = mXml.readElementText();
    if (!str.isEmpty()) {
        qDebug() << "Project::readInputWebcam() >>" << str;
        bool ok = false;
        mWebcam = str.toInt(&ok);
        if (!ok)
            mXml.raiseError(QObject::tr("invalid webcam: %1").arg(str));
    }
    else
        mXml.raiseError(QObject::tr("empty webcam tag: %1").arg(str));
}

void Project::readParameters(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "parameters");
    qDebug() << "Project::readParameters()";
    while (mXml.readNextStartElement()) {
        if (mXml.name() == "parameter") {
            readParameter();
        }
        else {
            mXml.skipCurrentElement();
        }
    }
}

void Project::readParameter()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "parameter");
    while (mXml.readNextStartElement()) {
        if (mXml.name() == "type") {
            readParameterType();
        }
        else if (mXml.name() == "name") {
            readParameterName();
        }
        else if (mXml.name() == "minValue") {
            readParameterMinValue();
        }
        else if (mXml.name() == "maxValue") {
            readParameterMaxValue();
        }
        else if (mXml.name() == "defaultValue") {
            readParameterDefaultValue();
        }
        else {
            mXml.skipCurrentElement();
        }
    }
}

void Project::readParameterType(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "type");
    const QString& str = mXml.readElementText();
    if (!str.isEmpty()) {
        qDebug() << "Project::readParameterType()";
    }
    else
        mXml.raiseError(QObject::tr("empty fragment shader: %1").arg(str));

}

void Project::readParameterName(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "name");

}

void Project::readParameterMinValue(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "minValue");

}

void Project::readParameterMaxValue(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "maxValue");

}

void Project::readParameterDefaultValue(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "defaultValue");

}
