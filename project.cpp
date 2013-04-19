// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QBuffer>
#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QTextCodec>
#include <QtCore/QDebug>
#include <QByteArray>
#include "project.h"
#include "main.h"

class ProjectPrivate {
public:
    ProjectPrivate(void)
        : dirty(false)
        , alphaEnabled(true)
        , imageRecyclingEnabled(false)
        , instantUpdate(false)
    { /* ... */ }
    bool dirty;
    QXmlStreamReader xml;
    QString vertexShaderSource;
    QString fragmentShaderSource;
    QString scriptSource;
    QImage image;
    QColor backgroundColor;
    bool alphaEnabled;
    bool imageRecyclingEnabled;
    bool instantUpdate;
    QString filename;
    QString errorString;
};

Project::Project(QObject* parent)
    : QObject(parent)
    , d_ptr(new ProjectPrivate)
{ /* ... */ }

Project::~Project()
{ /* ... */ }

void Project::reset(void)
{
    Q_D(Project);
    setClean();
    d->image = QImage();
    d->filename = QString();
    d->vertexShaderSource = QString();
    d->fragmentShaderSource = QString();
}

bool Project::save(void)
{
    Q_D(Project);
    Q_ASSERT(!d->filename.isEmpty());
    bool ok = save(d->filename);
    return ok;
}

bool Project::save(const QString& filename)
{
    Q_D(Project);
    Q_ASSERT(!filename.isEmpty());
    resetErrors();
    QFile file(filename);
    int flags = QIODevice::WriteOnly;
    bool compress = filename.endsWith("z");
    if (!compress)
        flags |= QIODevice::Text;
    bool ok = file.open((QIODevice::OpenMode)flags);
    if (!ok) {
        raiseError(tr("Cannot open file '%1' for writing").arg(filename));
        return false;
    }
    QString dstr;
    QTextStream out(&dstr);
    out.setAutoDetectUnicode(false);
    out.setCodec(QTextCodec::codecForMib(106/* UTF-8 */));
    out << "<glsl-live-coder-project version=\"" << AppVersionNoDebug << "\">\n"
        << "  <shaders>\n"
        << "    <vertex><![CDATA[" << d->vertexShaderSource << "]]></vertex>\n"
        << "    <fragment><![CDATA[" << d->fragmentShaderSource << "]]></fragment>\n"
        << "  </shaders>\n"
        << "  <script><![CDATA[" << d->scriptSource << "]]></script>\n"
        << "  <input>\n";
    if (!d->image.isNull()) {
        // check if image is really empty (transparent)
        bool totallyTransparent = true;
        const unsigned long* imgData = reinterpret_cast<unsigned long*>(d->image.bits());
        const unsigned long* const imgDataEnd = imgData + d->image.byteCount() / sizeof(unsigned long);
        while (imgData < imgDataEnd && totallyTransparent)
            totallyTransparent = totallyTransparent && (*imgData++ == 0);
        if (!totallyTransparent) {
            QByteArray ba;
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);
            d->image.save(&buffer, "PNG");
            buffer.close();
            out << "    <image><![CDATA[" << ba.toBase64() << "]]></image>\n";
        }
    }
    out << "  </input>\n"
        << "</glsl-live-coder-project>\n";
    qint64 bytesWritten = file.write(compress? qCompress(dstr.toUtf8(), 9) : dstr.toUtf8());
    ok = ok && (bytesWritten > 0);
    if (bytesWritten <= 0)
        raiseError(tr("Error writing data (%1 bytes written)").arg(bytesWritten));
    file.close();
    setClean();
    return ok;
}

bool Project::load(const QString& filename)
{
    Q_D(Project);
    Q_ASSERT(!filename.isEmpty());
    resetErrors();
    d->image = QImage();
    d->filename = filename;
    int flags = QIODevice::ReadOnly;
    bool compressed = filename.endsWith("z");
    if (!compressed)
        flags |= QIODevice::Text;
    QFile file(d->filename);
    bool success = file.open((QIODevice::OpenMode)flags);
    if (!success) {
        raiseError(tr("Cannot open file '%1' for reading").arg(filename));
        return false;
    }
    QByteArray ba = file.readAll();
    QBuffer buffer(&ba);
    buffer.open(QIODevice::ReadOnly);
    if (compressed)
        ba = qUncompress(ba);
    success = read(&buffer);
    if (!success)
        raiseError(tr("Cannot read from I/O device. (%1)").arg(filename));
    file.close();
    setClean();
    return success;
}

QString Project::errorString(void) const
 {
    if (!d_ptr->xml.errorString().isEmpty()) {
        return QObject::tr("%1 (line %2, column %3)")
            .arg(d_ptr->xml.errorString())
            .arg(d_ptr->xml.lineNumber())
            .arg(d_ptr->xml.columnNumber());
    }
    else if (!d_ptr->errorString.isEmpty()) {
        return d_ptr->errorString;
    }
    return QString();
}

void Project::resetErrors(void)
{
    d_ptr->errorString = QString();
}

void Project::raiseError(const QString& msg)
{
    d_ptr->errorString = msg;
}

bool Project::isDirty(void) const
{
    return d_ptr->dirty;
}

const QString& Project::filename(void) const
{
    return d_ptr->filename;
}

const QString& Project::vertexShaderSource(void) const
{
    return d_ptr->vertexShaderSource;
}

const QString& Project::fragmentShaderSource(void) const
{
    return d_ptr->fragmentShaderSource;
}

const QString &Project::scriptSource(void) const
{
    return d_ptr->scriptSource;
}

const QImage& Project::image(void) const
{
    return d_ptr->image;
}

void Project::setDirty(bool dirty)
{
    d_ptr->dirty = dirty;
}

void Project::setClean(bool clean)
{
    setDirty(!clean);
}

void Project::setVertexShaderSource(const QString& source)
{
    d_ptr->vertexShaderSource = source;
    setDirty();
}

void Project::setFragmentShaderSource(const QString& source)
{
    d_ptr->fragmentShaderSource = source;
    setDirty();
}

void Project::setScriptSource(const QString& source)
{
    d_ptr->scriptSource = source;
    setDirty();
}

void Project::setImage(const QImage& image)
{
    d_ptr->image = image;
    setDirty();
}

void Project::setFilename(const QString& filename)
{
    d_ptr->filename = filename;
    setDirty();
}

bool Project::read(QIODevice* device)
{
    Q_D(Project);
    Q_ASSERT(device != NULL);
    resetErrors();
    d->xml.setDevice(device);
    if (d->xml.readNextStartElement()) {
        if (d->xml.name() == "glsl-live-coder-project" && d->xml.attributes().value("version").toString().startsWith("0.")) {
            read();
        }
        else {
            d->xml.raiseError(QObject::tr("The file is not an GLSL Live Coder v0.x project file."));
        }
    }
    return !d->xml.error();
}

void Project::read(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "glsl-live-coder-project");
    while (d->xml.readNextStartElement()) {
        if (d->xml.name() == "shaders") {
            readShaders();
        }
        else if (d->xml.name() == "input") {
            readInput();
        }
        else if (d->xml.name() == "script") {
            readScript();
        }
        else {
            d->xml.skipCurrentElement();
        }
    }
}

void Project::readShaders(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "shaders");
    while (d->xml.readNextStartElement()) {
        if (d->xml.name() == "vertex") {
            readShaderVertex();
        }
        else if (d->xml.name() == "fragment") {
            readShaderFragment();
        }
        else {
            d->xml.skipCurrentElement();
        }
    }
}

void Project::readScript(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "script");
    d->scriptSource = d->xml.readElementText();
}

void Project::readShaderVertex(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "vertex");
    const QString& str = d->xml.readElementText();
    if (!str.isEmpty()) {
        d->vertexShaderSource = str;
    }
    else {
        d->xml.raiseError(QObject::tr("empty vertex shader: %1").arg(str));
    }
}

void Project::readShaderFragment(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "fragment");
    const QString& str = d->xml.readElementText();
    if (!str.isEmpty()) {
        d->fragmentShaderSource = str;
    }
    else {
        d->xml.raiseError(QObject::tr("empty fragment shader: %1").arg(str));
    }
}

void Project::readInput(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "input");
    while (d->xml.readNextStartElement()) {
        if (d->xml.name() == "image") {
            readInputImage();
        }
        else {
            d->xml.skipCurrentElement();
        }
    }
}

void Project::readInputImage(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "image");
    const QString& str = d->xml.readElementText();
    if (!str.isEmpty()) {
        const QByteArray& imgData = QByteArray::fromBase64(str.toUtf8());
        bool ok = d->image.loadFromData(imgData);
        if (!ok)
            raiseError(tr("Invalid data in <image> tag"));
    }
    else {
        d->xml.raiseError(QObject::tr("empty image: %1").arg(str));
    }
}

void Project::enableAlpha(bool enabled)
{
    d_ptr->alphaEnabled = enabled;
}

void Project::enableImageRecycling(bool enabled)
{
    d_ptr->imageRecyclingEnabled = enabled;
}

void Project::enableInstantUpdate(bool enabled)
{
    d_ptr->instantUpdate = enabled;
}

void Project::setBackgroundColor(const QColor& color)
{
    d_ptr->backgroundColor = color;
}

bool Project::alphaEnabled(void) const
{
    return d_ptr->alphaEnabled;
}

bool Project::imageRecyclingEnabled(void) const
{
    return d_ptr->imageRecyclingEnabled;
}

bool Project::instantUpdateEnabled(void) const
{
    return d_ptr->instantUpdate;
}

const QColor& Project::backgroundColor(void) const
{
    return d_ptr->backgroundColor;
}

