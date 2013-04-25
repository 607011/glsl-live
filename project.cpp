// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QBuffer>
#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QTextCodec>
#include <QtCore/QDebug>
#include <QByteArray>
#include <QVariant>
#include "renderwidget.h"
#include "project.h"
#include "main.h"

class ProjectPrivate {
public:
    ProjectPrivate(void)
        : dirty(false)
        , alphaEnabled(true)
        , imageRecyclingEnabled(false)
        , instantUpdate(false)
        , borderClamping(true)
    { /* ... */ }
    bool dirty;
    QXmlStreamReader xml;
    QString vertexShaderSource;
    QString fragmentShaderSource;
    QString scriptSource;
    QImage image;
    QVariant channelData[Project::MAX_CHANNELS];
    Project::SourceSelector channelSource[Project::MAX_CHANNELS];
    int channelSourceId[Project::MAX_CHANNELS];
    QColor backgroundColor;
    bool alphaEnabled;
    bool imageRecyclingEnabled;
    bool instantUpdate;
    bool borderClamping;
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
    d->alphaEnabled = true;
    d->imageRecyclingEnabled = false;
    d->instantUpdate = false;
    d->borderClamping = true;
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        d->channelData[i] = QVariant();
        d->channelSource[i] = SourceNone;
        d->channelSourceId[i] = -1;
    }
}

bool Project::save(void)
{
    Q_D(Project);
    Q_ASSERT(!d->filename.isEmpty());
    bool ok = save(d->filename);
    return ok;
}

QTextStream& operator<<(QTextStream& out, const QColor& color)
{
    out << "rgba(" << color.red() << "," << color.green() << "," << color.blue() << "," << color.alpha() << ")";
    return out;
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
        << "  </shaders>\n";
#ifdef WITH_SCRIPTING
    out << "  <script><![CDATA[" << d->scriptSource << "]]></script>\n";
#endif
    out << "  <input>\n";
    if (hasImage()) {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        d->image.save(&buffer, "PNG");
        buffer.close();
        out << "    <image><![CDATA[" << ba.toBase64() << "]]></image>\n";
    }
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        const QVariant& ch = d->channelData[i];
        const SourceSelector source = d->channelSource[i];
        if (source != SourceNone) {
            switch (ch.type()) {
            case QVariant::Image: {
                if (source == SourceData) {
                    QByteArray ba;
                    QBuffer buffer(&ba);
                    buffer.open(QIODevice::WriteOnly);
                    ch.value<QImage>().save(&buffer, "PNG");
                    buffer.close();
                    out << "    <channel id=\"" << i << "\"><![CDATA[" << ba.toBase64() << "]]></channel>\n";
                }
                else if (source == SourceWebcam) {
                    out << "    <channel id=\"" << i << "\" source=\"webcam\"></channel>\n";
                }
                break;
            }
            case QVariant::Invalid:
                // fall-through
            default:
                // ignore
                break;
            }
        }
    }
    out << "  </input>\n";
    out << "  <options>\n"
        << "    <clamp>" << d->borderClamping << "</clamp>\n"
        << "    <backgroundcolor>" << d->backgroundColor << "</backgroundcolor>\n"
        << "    <instantupdate>" << d->instantUpdate << "</instantupdate>\n"
        << "    <imagerecycling>" << d->imageRecyclingEnabled << "</imagerecycling>\n"
        << "    <alpha>" << d->alphaEnabled << "</alpha>\n"
        << "  </options>\n"
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

const QVariant &Project::channel(int index) const
{
    return d_ptr->channelData[index];
}

void Project::setDirty(bool dirty)
{
    d_ptr->dirty = dirty;
    emit dirtyStateChanged(dirty);
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

void Project::setChannel(int index, const QImage& img)
{
    Q_ASSERT_X(index >= 0 && index < MAX_CHANNELS, "Project::setChannel()", "image index out of bounds");
    d_ptr->channelData[index] = img;
    d_ptr->channelSource[index] = SourceData;
    setDirty();
}

void Project::setChannel(int index, Project::SourceSelector source, int id)
{
    Q_ASSERT_X(index >= 0 && index < MAX_CHANNELS, "Project::setChannel()", "image index out of bounds");
    qDebug() << "Project::setChannel(" << index << "," << source << "," << id << ")";
    d_ptr->channelData[index] = QVariant();
    d_ptr->channelSource[index] = source;
    d_ptr->channelSourceId[index] = id;
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
            d->xml.raiseError(QObject::tr("The file is not an GLSL Live Coder v0.x project file ."));
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
#ifdef WITH_SCRIPTING
        else if (d->xml.name() == "script") {
            readScript();
        }
#endif
        else if (d->xml.name() == "options") {
            readOptions();
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
        d->xml.raiseError(QObject::tr("empty <vertex>: %1").arg(str));
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
        d->xml.raiseError(QObject::tr("empty <fragment>: %1").arg(str));
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
        else if (d->xml.name() == "channel") {
            readInputChannel();
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
        d->xml.raiseError(QObject::tr("empty <image>: %1").arg(str));
    }
}

void Project::readInputChannel(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "channel");
    bool ok;
    QXmlStreamAttributes attr = d->xml.attributes();
    const QString& sourceStr = attr.value("source").toString();
    const QString& idStr = attr.value("id").toString();
    int id = idStr.toInt(&ok);
    if (!ok)
        raiseError(tr("Invalid data in <channel> id attribute"));
    qDebug() << "id =" << id << " source =" << sourceStr;
    if (!sourceStr.isEmpty()) {
        if (sourceStr == "webcam") {
            d->channelSource[id] = SourceWebcam;
            const QString& sourceIdStr = attr.value("source-id").toString();
            qDebug() << "sourceIdStr =" << sourceIdStr;
            if (!sourceIdStr.isEmpty()) {
                int sourceId = sourceIdStr.toInt(&ok);
                if (ok)
                    d->channelSourceId[id] = sourceId;
                else
                    raiseError(tr("Invalid data in <channel> source-id attribute"));
            }
        }
    }
    const QString& str = d->xml.readElementText();
    if (!str.isEmpty()) {
        const QByteArray& imgData = QByteArray::fromBase64(str.toUtf8());
        QImage img;
        ok = img.loadFromData(imgData);
        if (!ok)
            raiseError(tr("Invalid data in <channel> tag"));
        d->channelData[id] = img;
    }
}

void Project::readOptions(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "options");
    while (d->xml.readNextStartElement()) {
        if (d->xml.name() == "alpha") {
            readAlpha();
        }
        else if (d->xml.name() == "clamp") {
            readClamp();
        }
        else if (d->xml.name() == "backgroundcolor") {
            readBackgroundColor();
        }
        else if (d->xml.name() == "instantupdate") {
            readInstantUpdate();
        }
        else if (d->xml.name() == "imagerecycling") {
            readImageRecycling();
        }
        else {
            d->xml.skipCurrentElement();
        }
    }
}

void Project::readAlpha(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "alpha");
    const QString& str = d->xml.readElementText();
    if (!str.isEmpty()) {
        bool ok;
        int v = str.toInt(&ok);
        if (ok)
            d->alphaEnabled = (1 == v);
    }
    else {
        d->xml.raiseError(QObject::tr("empty <alpha>: %1").arg(str));
    }
}

void Project::readClamp(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "clamp");
    const QString& str = d->xml.readElementText();
    if (!str.isEmpty()) {
        bool ok;
        int v = str.toInt(&ok);
        if (ok)
            d->borderClamping = (1 == v);
    }
    else {
        d->xml.raiseError(QObject::tr("empty <clamp>: %1").arg(str));
    }
}

void Project::readBackgroundColor(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "backgroundcolor");
    const QString& str = d->xml.readElementText();
    if (!str.isEmpty()) {
        QRegExp reC("rgba\\s*\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\)");
        bool ok1, ok2, ok3, ok4;
        if (reC.indexIn(str) > -1) {
            QColor newColor(reC.cap(1).toInt(&ok1),
                            reC.cap(2).toInt(&ok2),
                            reC.cap(3).toInt(&ok3),
                            reC.cap(4).toInt(&ok4));
            if (ok1 && ok2 && ok3 && ok4)
                d->backgroundColor = newColor;
            else
                d->xml.raiseError(QObject::tr("bad data in <backgroundcolor>: %1").arg(str));
        }
    }
    else {
        d->xml.raiseError(QObject::tr("empty <backgroundcolor>: %1").arg(str));
    }
}

void Project::readInstantUpdate(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "instantupdate");
    const QString& str = d->xml.readElementText();
    if (!str.isEmpty()) {
        bool ok;
        int v = str.toInt(&ok);
        if (ok)
            d->instantUpdate = (1 == v);
    }
    else {
        d->xml.raiseError(QObject::tr("empty <instantupdate>: %1").arg(str));
    }
}

void Project::readImageRecycling(void)
{
    Q_D(Project);
    Q_ASSERT(d->xml.isStartElement() && d->xml.name() == "imagerecycling");
    const QString& str = d->xml.readElementText();
    if (!str.isEmpty()) {
        bool ok;
        int v = str.toInt(&ok);
        if (ok)
            d->imageRecyclingEnabled = (1 == v);
    }
    else {
        d->xml.raiseError(QObject::tr("empty <imagerecycling>: %1").arg(str));
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

void Project::enableBorderClamping(bool enabled)
{
    d_ptr->borderClamping = enabled;
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

bool Project::borderClampingEnabled(void) const
{
    return d_ptr->borderClamping;
}

const QColor& Project::backgroundColor(void) const
{
    return d_ptr->backgroundColor;
}

bool Project::hasImage(void) const
{
    return !isEmpty(d_ptr->image);
}

bool Project::isEmpty(const QImage& img)
{
    if (img.isNull())
        return true;
    // check if image is really empty (transparent)
    bool totallyTransparent = true;
    const QRgb* imgData = reinterpret_cast<const QRgb*>(img.bits());
    const QRgb* const imgDataEnd = imgData + img.byteCount() / sizeof(QRgb);
    while (totallyTransparent && imgData < imgDataEnd)
        totallyTransparent = totallyTransparent && (*imgData++ == 0);
    return totallyTransparent;
}
