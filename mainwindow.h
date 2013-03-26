// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <QMainWindow>
#include <QResizeEvent>
#include <QVBoxLayout>
#include "renderwidget.h"
#include "project.h"
#include "jsedit/jsedit.h"

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
    void shaderChanged(void);
    void newProject(void);
    void openProject(void);
    void openProject(const QString& filename);
    void saveProject(void);
    void saveProject(const QString& filename);
    void saveProjectAs(void);

private: //methods
    void saveSettings(void);
    void restoreSettings(void);
    void prepareEditor(JSEdit& editor) const;
    void loadVertexShader(const QString&);
    void loadFragmentShader(const QString&);
    void updateWindowTitle(void);
    void updateShaderSources(void);

private:
    Ui::MainWindow *ui;
    Project mProject;
    RenderWidget* mRenderWidget;
    QVBoxLayout* mParametersLayout;
    QString mVertexShaderFilename;
    QString mFragmentShaderFilename;
    QString mImageFilename;
    JSEdit mVertexShaderEditor;
    JSEdit mFragmentShaderEditor;
};

#endif // __MAINWINDOW_H_
