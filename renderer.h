// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __RENDERER_H_
#define __RENDERER_H_

#include <QGLWidget>
#include <QImage>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QScopedPointer>


class RendererPrivate;

// In Renderer wird ein QImage durch das Shader-Programm gejagt und als Datei gespeichert.
// Das hätte man grundsätzlich auch im RenderWidget erledigen können, doch dort hätte man
// den gesamten GL-Kontext für die Dauer der Nutzung von Renderer::process() sichern und
// anschließend wiederherstellen müssen. Da schien mir die Verwendung eines separaten GL-
// Kontextes in einem weiteren QGLWidget die sauberere Lösung zu sein, auch wenn das
// bedeutet, die Mimik zum Kompilieren der Shader (buildProgram()) und Setzen der Uniforms
// doppelt zu implementieren.
class Renderer : public QGLWidget
{
public:
    Renderer(QWidget* parent = NULL);
    ~Renderer();

    typedef QMap<QString, QVariant> UniformMap;

    void buildProgram(const QString& vs, const QString& fs);
    void setUniforms(const UniformMap& uniforms);
    void process(const QImage& inputImage, const QString& outFilename);

private: // methods
    void updateUniforms(void);

private: // members
    QScopedPointer<RendererPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Renderer)
    Q_DISABLE_COPY(Renderer)
};



#endif // __RENDERER_H_
