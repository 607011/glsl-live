// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <QMainWindow>
#include <QResizeEvent>
#include <QScopedPointer>
#include <QString>

namespace Ui {
class MainWindow;
}

class MainWindowPrivate;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = NULL);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent*);

private slots:
    void about(void);
    void aboutQt(void);
    void badVertexShaderCode(const QString&);
    void badFragmentShaderCode(const QString&);
    void linkerError(const QString&);
    void successfullyLinkedShader(void);
    void parseShadersForParameters();
    void processShaderChange(void);
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
    void saveImageSnapshot(void);

private: //methods
    void saveSettings(void);
    void restoreSettings(void);
    void updateWindowTitle(void);

private:
    Ui::MainWindow *ui;

    QScopedPointer<MainWindowPrivate> d_ptr;
    Q_DECLARE_PRIVATE(MainWindow)
    Q_DISABLE_COPY(MainWindow)
};

#endif // __MAINWINDOW_H_
