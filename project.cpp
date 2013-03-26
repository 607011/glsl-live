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
    , mWebcam(-1)
{
    /* ... */
}

QString Project::errorString(void) const
{
    return QObject::tr("%1 (line %2, column %3)").arg(mXml.errorString()).arg(mXml.lineNumber()).arg(mXml.columnNumber());
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
        << "  <shaders>\n"
        << "    <vertex><![CDATA["
        << mVertexShaderSource << "]]>"
        << "    </vertex>\n"
        << "    <fragment><![CDATA["
        << mFragmentShaderSource << "]]>"
        << "    </fragment>\n"
        << "  </shaders>\n"
        << "  <input>\n";
    if (!mImage.isNull() || mWebcam >= 0) {
        out << "    <image>\n";
        if (!mImage.isNull()) {
            out << "      <static><![CDATA["
                << QByteArray((const char*)mImage.bits(), mImage.byteCount()).toBase64() << "]]>"
                << "      </static>\n";
        }
        else if (mWebcam >= 0) {
            out << "      <webcam>" << mWebcam << "</webcam>\n";
        }
        out << "    </image>\n";
    }
    out << "  </input>\n"
        << "  <parameters>\n";
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
    out << "  </parameters>\n"
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
        qDebug() << "Project::readShaderVertex()";
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
        qDebug() << "Project::readShaderFragment()";
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
        mImage = QImage::fromData(QByteArray::fromBase64(str.toLatin1()));
    }
    else
        mXml.raiseError(QObject::tr("empty fragment shader: %1").arg(str));
}

void Project::readInputWebcam()
{
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
        else if (mXml.name() == "slider") {
            readParameterSlider();
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

void Project::readParameterSlider(void)
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "slider");
    while (mXml.readNextStartElement()) {
        if (mXml.name() == "direction") {
            readParameterSliderDirection();
        }
        else {
            mXml.skipCurrentElement();
        }
    }
}

void Project::readParameterSliderDirection()
{
    Q_ASSERT(mXml.isStartElement() && mXml.name() == "direction");
    const QString& str = mXml.readElementText();
    if (!str.isEmpty()) {
        if (str == "horizontal") {
            // TODO
        }
        else if (str == "vertical") {
            // TODO
        }
        else {
            mXml.raiseError(QObject::tr("invalid slider direction: %1").arg(str));

        }
    }
    else
        mXml.raiseError(QObject::tr("empty slider direction: %1").arg(str));

}
