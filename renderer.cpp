#include "renderer.h"


static const int PROGRAM_VERTEX_ATTRIBUTE = 0;
static const int PROGRAM_TEXCOORD_ATTRIBUTE = 1;
static const QVector2D TexCoords[4] =
{
    QVector2D(0, 0),
    QVector2D(0, 1),
    QVector2D(1, 0),
    QVector2D(1, 1)
};
static const QVector2D Vertices[4] =
{
    QVector2D(-1.0, -1.0),
    QVector2D(-1.0,  1.0),
    QVector2D( 1.0, -1.0),
    QVector2D( 1.0,  1.0)
};

Renderer::Renderer(void)
    : QGLContext(QGLFormat::defaultFormat())
    , mShaderProgram(new QGLShaderProgram)
    , mVertexShader(new QGLShader(QGLShader::Vertex))
    , mFragmentShader(new QGLShader(QGLShader::Fragment))
    , mFBO(NULL)
{
    init();
}

Renderer::Renderer(const Renderer& o)
    : QGLContext(QGLFormat::defaultFormat())
    , mShaderProgram(o.mShaderProgram)
    , mVertexShader(o.mVertexShader)
    , mFragmentShader(o.mFragmentShader)
    , mFBO(NULL)
{
    init();
}

Renderer::~Renderer()
{
    mShaderProgram->removeAllShaders();
    safeDelete(mShaderProgram);
    safeDelete(mVertexShader);
    safeDelete(mFragmentShader);
}

void Renderer::init()
{
    makeCurrent();
    QGLFormat glFormat;
    glFormat.setAlpha(false);
    glFormat.setDoubleBuffer(false);
    glFormat.setDepth(false);
    glFormat.setDirectRendering(false);
    glFormat.setOverlay(false);
    setFormat(glFormat);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glGenTextures(1, &mTextureHandle);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    mT.start();
}

void Renderer::updateUniforms(void)
{
    QStringListIterator k(mUniforms.keys());
    while (k.hasNext()) {
        const QString& key = k.next();
        const QVariant& value = mUniforms[key];
        switch (value.type()) {
        case QVariant::Int:
            mShaderProgram->setUniformValue(key.toUtf8().data(), value.toInt());
            break;
        case QVariant::Double:
            mShaderProgram->setUniformValue(key.toUtf8().data(), (GLfloat)value.toDouble());
            break;
        case QVariant::Bool:
            mShaderProgram->setUniformValue(key.toUtf8().data(), value.toBool());
            break;
        default:
            break;
        }
    }
}

QImage Renderer::process(const QImage& image)
{
    makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT);
    if (mFBO == NULL || mFBO->size() != image.size())
        safeRenew(mFBO, new QGLFramebufferObject(image.size()));
    qDebug() << "Renderer::process()" << image.size();
    mFBO->bind();
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
    mShaderProgram->setUniformValue("uTexture", 0);
    mShaderProgram->setUniformValue("uResolution", QSizeF(image.size()));
    mShaderProgram->setUniformValue("uT", GLfloat(1e-3 * mT.elapsed()));
    updateUniforms();
    glViewport(0, 0, image.width(), image.height());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    mFBO->release();
    return mFBO->toImage();
}

void Renderer::makeCurrent(void)
{
    QGLContext::makeCurrent();
}

void Renderer::buildProgram(const QString& vs, const QString& fs)
{
    if (vs.isEmpty() || fs.isEmpty())
        return;
    mShaderProgram->removeAllShaders();
    mShaderProgram->addShader(mVertexShader);
    mShaderProgram->addShader(mFragmentShader);
    mVertexShader->compileSourceCode(vs);
    mFragmentShader->compileSourceCode(fs);
    mShaderProgram->link();
    mShaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    mShaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    mShaderProgram->bind();
    mShaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    mShaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    mShaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, Vertices);
    mShaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, TexCoords);
    qDebug() << "Renderer::buildProgram(" << vs.left(30) << "," << fs.left(30) << ")";
    qDebug() << "mShaderProgram->isLinked() =" << mShaderProgram->isLinked();
}
