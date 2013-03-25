// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QResizeEvent>
#include "renderwidget.h"
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
    void vertexShaderChanged(void);
    void fragmentShaderChanged(void);

private: //methods
    void saveSettings(void);
    void restoreSettings(void);
    void prepareEditor(JSEdit& editor) const;
    void loadVertexShader(const QString&);
    void loadFragmentShader(const QString&);

private:
    Ui::MainWindow *ui;
    RenderWidget* mRenderWidget;
    QString mVertexShaderFilename;
    QString mFragmentShaderFilename;
    JSEdit mVertexShaderEditor;
    JSEdit mFragmentShaderEditor;
};

#endif // MAINWINDOW_H
