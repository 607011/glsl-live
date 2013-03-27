// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <QMainWindow>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWidget>
#include <QByteArray>
#include "renderwidget.h"
#include "project.h"
#include "glsledit/glsledit.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = NULL);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent*);
    void resizeEvent(QResizeEvent*);

private slots:
    void about(void);
    void aboutQt(void);
    void badShaderCode(const QString&);
    void successfullyLinkedShader(void);
    void parseShadersForParameters();
    void shaderChanged(void);
    void updateShaderSources(void);
    void newProject(void);
    void openProject(void);
    void openProject(const QString& filename);
    void saveProject(void);
    void saveProject(const QString& filename);
    void saveProjectAs(void);
    void valueChanged(int);
    void valueChanged(double);
    void valueChanged(bool);

private: //methods
    void saveSettings(void);
    void restoreSettings(void);
    void prepareEditor(GLSLEdit& editor) const;
    void loadVertexShader(const QString&);
    void loadFragmentShader(const QString&);
    void updateWindowTitle(void);
    void clearLayout(QLayout*);

private:
    Ui::MainWindow *ui;
    Project mProject;
    RenderWidget* mRenderWidget;
    QWidget* mParamWidget;
    QString mVertexShaderFilename;
    QString mFragmentShaderFilename;
    QString mImageFilename;
    GLSLEdit mVertexShaderEditor;
    GLSLEdit mFragmentShaderEditor;
    QByteArray mCurrentParameterHash;
};

#endif // __MAINWINDOW_H_
