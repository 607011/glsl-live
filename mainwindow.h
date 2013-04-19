// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __MAINWINDOW_H_
#define __MAINWINDOW_H_

#include <QMainWindow>
#include <QCloseEvent>
#include <QScopedPointer>
#include <QList>
#include <QString>
#include <QAction>
#include <QMenu>

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

public slots:
    void debug(const QString& message);

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
    void openRecentProject(void);
    void appendToRecentFileList(const QString& fileName, const QString& listName, QMenu* menu, QAction* actions[]);
    void newProject(void);
    void openProject(void);
    void openProject(const QString& filename);
    void saveProject(void);
    void saveProject(const QString& filename);
    void saveProjectAs(void);
    void loadImage(void);
    void loadImage(const QString& filename);
    void valueChanged(int);
    void valueChanged(double);
    void valueChanged(bool);
    void imageDropped(const QImage&);
    void saveImageSnapshot(void);
    void batchProcess(void);
    void zoom(void);
    void chooseBackgroundColor(void);
    void setFPS(double);
    void renderStarted(void);
    void renderStopped(void);
    void addFragmentShaderEditor(void);

#ifdef ENABLE_SCRIPTING
    void processScriptChange(void);
    void executeScript(void);
#endif

private: //methods
    void saveSettings(void);
    void restoreSettings(void);
    void updateWindowTitle(void);
    void processBatch(const QList<QString>& filenames, const QString& outDir);
    void fetchFragmentCodeFromProjectAndCreateEditors(void);
    void addFragmentShaderEditor(const QString& source);
    void removeAllFragmentShaderEditors(void);

private:
    Ui::MainWindow *ui;

    QScopedPointer<MainWindowPrivate> d_ptr;
    Q_DECLARE_PRIVATE(MainWindow)
    Q_DISABLE_COPY(MainWindow)
};

#endif // __MAINWINDOW_H_
