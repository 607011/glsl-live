// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include "main.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mProject(NULL)
    , mRenderWidget(new RenderWidget)
    , mParametersLayout(new QVBoxLayout)
{
    ui->setupUi(this);
    ui->splitter->addWidget(mRenderWidget);
    QWidget* paramWidget = new QWidget;
    paramWidget->setLayout(mParametersLayout);
    ui->splitter->addWidget(paramWidget);
    ui->vertexShaderHLayout->addWidget(&mVertexShaderEditor);
    ui->fragmentShaderHLayout->addWidget(&mFragmentShaderEditor);
    prepareEditor(mVertexShaderEditor);
    prepareEditor(mFragmentShaderEditor);
    QObject::connect(&mVertexShaderEditor, SIGNAL(textChanged()), SLOT(shaderChanged()));
    QObject::connect(&mFragmentShaderEditor, SIGNAL(textChanged()), SLOT(shaderChanged()));
    QObject::connect(mRenderWidget, SIGNAL(shaderError(QString)), SLOT(badShaderCode(QString)));
    QObject::connect(mRenderWidget, SIGNAL(linkingSuccessful()), SLOT(successfullyLinkedShader()));
    QObject::connect(ui->actionExit, SIGNAL(triggered()), SLOT(close()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered()), SLOT(about()));
    QObject::connect(ui->actionAboutQt, SIGNAL(triggered()), SLOT(aboutQt()));
    QObject::connect(ui->actionOpen, SIGNAL(triggered()), SLOT(openProject()));
    QObject::connect(ui->actionSave, SIGNAL(triggered()), SLOT(saveProject()));
    QObject::connect(ui->actionSaveProjectAs, SIGNAL(triggered()), SLOT(saveProjectAs()));
    QObject::connect(ui->actionNew, SIGNAL(triggered()), SLOT(newProject()));
    restoreSettings();
}

MainWindow::~MainWindow()
{
    delete mParametersLayout;
    delete mRenderWidget;
    delete ui;
}

void MainWindow::restoreSettings(void)
{
    QSettings settings(Company, AppName);
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    ui->splitter->restoreGeometry(settings.value("MainWindow/splitter/geometry").toByteArray());
    const QString& projectFilename = settings.value("Project/filename").toString();
    if (!projectFilename.isEmpty()) {
        openProject(projectFilename);
    }
#if 0
    mVertexShaderFilename = settings.value("Options/VertexShaderEditor/sourceFilename", ":/shaders/vertexshader.glsl").toString();
    QString vs = settings.value("Options/VertexShaderEditor/source").toString();
    if (vs.isEmpty()) {
        loadVertexShader(mVertexShaderFilename);
    }
    else {
        mVertexShaderEditor.setPlainText(vs);
    }
    mFragmentShaderFilename = settings.value("Options/FragmentShaderEditor/sourceFilename", ":/shaders/fragmentshader.glsl").toString();
    QString fs = settings.value("Options/FragmentShaderEditor/source").toString();
    if (fs.isEmpty()) {
        loadFragmentShader(mFragmentShaderFilename);
    }
    else {
        mFragmentShaderEditor.setPlainText(fs);
    }
    mRenderWidget->setShaderSources(mVertexShaderEditor.toPlainText(), mFragmentShaderEditor.toPlainText());
    ui->toolBox->setCurrentIndex(settings.value("Options/Toolbox/currentIndex", 0).toInt());
    mImageFilename = settings.value("Options/imageFilename", ":/images/toad.png").toString();
    if (!mImageFilename.isEmpty()) {
        mRenderWidget->setImage(QImage(mImageFilename));
        mProject.setImage(mRenderWidget->image());
    }
#endif
    updateWindowTitle();
}

void MainWindow::saveSettings(void)
{
    QSettings settings(Company, AppName);
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/Toolbox/currentIndex", ui->toolBox->currentIndex());
    settings.setValue("MainWindow/splitter/geometry", saveGeometry());
#if 0
    settings.setValue("Options/VertexShaderEditor/sourceFilename", mVertexShaderFilename);
    settings.setValue("Options/VertexShaderEditor/source", mVertexShaderEditor.toPlainText());
    settings.setValue("Options/FragmentShaderEditor/sourceFilename", mFragmentShaderFilename);
    settings.setValue("Options/FragmentShaderEditor/source", mFragmentShaderEditor.toPlainText());
    settings.setValue("Options/imageFilename", mImageFilename);
#endif
    settings.setValue("Project/filename", mProject.filename());
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    if (mProject.isDirty()) {
        bool ok = QMessageBox::question(this, tr("Save before exit?"), tr("Your project has changed. Do you want to save the changes before exiting?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
        if (ok)
            saveProject();
    }
    saveSettings();
    e->accept();
}

void MainWindow::resizeEvent(QResizeEvent* e)
{
    e->accept();
}

void MainWindow::prepareEditor(JSEdit& editor) const
{
    editor.setTabStopWidth(2);
    editor.setWordWrapMode(QTextOption::NoWrap);
    QFont monospace;
#if defined(Q_OS_MAC)
    monospace.setPointSize(10);
    monospace.setFamily("Monaco");
#else
    monospace.setPointSize(8);
    monospace.setFamily("Monospace");
#endif
    monospace.setStyleHint(QFont::TypeWriter);
    editor.setFont(monospace);
    editor.setTextWrapEnabled(true);
    editor.setBracketsMatchingEnabled(true);
    editor.setCodeFoldingEnabled(true);
    editor.setStyleSheet("background-color: rgba(20, 20, 20, 200)");
    editor.setColor(JSEdit::Background,    QColor(20, 20, 20, 200));
    editor.setColor(JSEdit::Normal,        QColor("#FFFFFF"));
    editor.setColor(JSEdit::Comment,       QColor("#666666"));
    editor.setColor(JSEdit::Number,        QColor("#DBF76C"));
    editor.setColor(JSEdit::String,        QColor("#5ED363"));
    editor.setColor(JSEdit::Operator,      QColor("#FF7729"));
    editor.setColor(JSEdit::Identifier,    QColor("#FFFFFF"));
    editor.setColor(JSEdit::Keyword,       QColor("#FDE15D"));
    editor.setColor(JSEdit::BuiltIn,       QColor("#9CB6D4"));
    editor.setColor(JSEdit::Cursor,        QColor("#1E346B"));
    editor.setColor(JSEdit::Marker,        QColor("#DBF76C"));
    editor.setColor(JSEdit::BracketMatch,  QColor("#1AB0A6"));
    editor.setColor(JSEdit::BracketError,  QColor("#A82224"));
    editor.setColor(JSEdit::FoldIndicator, QColor("#555555"));
}

void MainWindow::badShaderCode(const QString& msg)
{
    ui->logTextEdit->append(msg);
    ui->toolBox->setItemText(2, tr("Error log*"));
}

void MainWindow::successfullyLinkedShader()
{
    ui->logTextEdit->clear();
    ui->toolBox->setItemText(2, tr("Error log"));
}

void MainWindow::shaderChanged()
{
    mProject.setDirty(true);
    updateShaderSources();
    updateWindowTitle();
}

void MainWindow::newProject(void)
{
    if (mProject.isDirty()) {
        bool ok = QMessageBox::question(this, tr("Really create a new project?"), tr("Your project has changed. Do you want to save the changes before creating a new project?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
        if (ok)
            saveProject();
    }
    mProject.reset();
    mVertexShaderEditor.setPlainText(mProject.vertexShaderSource());
    mFragmentShaderEditor.setPlainText(mProject.fragmentShaderSource());
    mRenderWidget->setImage(mProject.image());
    updateShaderSources();
    updateWindowTitle();
}

void MainWindow::openProject(void)
{
    if (mProject.isDirty()) {
        bool ok = QMessageBox::question(this, tr("Really load another project?"), tr("Your project has changed. Do you want to save the changes before loading another project?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
        if (ok)
            saveProject();
    }
    const QString& filename = QFileDialog::getOpenFileName(this, tr("Load project"), QString(), tr("Project files (*.xml *.glslx *.xmlz *.glslz)"));
    if (filename.isEmpty())
        return;
    openProject(filename);
}


void MainWindow::openProject(const QString& filename)
{
    Q_ASSERT(!filename.isEmpty());
    bool ok = mProject.load(filename);
    if (ok) {
        mVertexShaderEditor.blockSignals(true);
        mVertexShaderEditor.setPlainText(mProject.vertexShaderSource());
        mVertexShaderEditor.blockSignals(false);
        mFragmentShaderEditor.blockSignals(true);
        mFragmentShaderEditor.setPlainText(mProject.fragmentShaderSource());
        mFragmentShaderEditor.blockSignals(false);
        mRenderWidget->setImage(mProject.image());
        updateShaderSources();
    }
    ui->statusBar->showMessage(ok? tr("Project '%1' loaded.").arg(QFileInfo(filename).fileName()) : tr("Loading '%1' failed.").arg(QFileInfo(filename).fileName()), 3000);
    updateWindowTitle();
}

void MainWindow::saveProject(void)
{
    QString filename = mProject.filename();
    if (filename.isEmpty()) {
        filename = QFileDialog::getSaveFileName(this, tr("Save project"), QString(), tr("Project files (*.xml *.glslx *.xmlz *.glslz)"));
        if (filename.isNull())
            return;
    }
    saveProject(filename);
}

void MainWindow::saveProjectAs(void)
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save project"), QString(), tr("Project files (*.xml *.glslx *.xmlz *.glslz)"));
    if (filename.isNull())
        return;
    saveProject(filename);
}

void MainWindow::saveProject(const QString &filename)
{
    mProject.setFilename(filename);
    mProject.setVertexShaderSource(mVertexShaderEditor.toPlainText());
    mProject.setFragmentShaderSource(mFragmentShaderEditor.toPlainText());
    mProject.setImage(mRenderWidget->image());
    bool ok = mProject.save();
    ui->statusBar->showMessage(ok? tr("Project saved as '%1'.").arg(QFileInfo(filename).fileName()) : tr("Saving failed."), 3000);
    updateWindowTitle();
}

void MainWindow::updateShaderSources(void)
{
    mRenderWidget->setShaderSources(mVertexShaderEditor.toPlainText(), mFragmentShaderEditor.toPlainText());
}

void MainWindow::loadVertexShader(const QString& filename)
{
    if (!filename.isEmpty()) {
        QFile file(filename);
        bool success = file.open(QIODevice::ReadOnly | QIODevice::Text);
        if (success) {
            const QString& source = file.readAll();
            mVertexShaderEditor.setPlainText(source);
            mVertexShaderFilename = filename;
            mProject.setVertexShaderSource(source);
            file.close();
        }
    }
}

void MainWindow::loadFragmentShader(const QString& filename)
{
    if (!filename.isEmpty()) {
        QFile file(filename);
        bool success = file.open(QIODevice::ReadOnly | QIODevice::Text);
        if (success) {
            const QString& source = file.readAll();
            mFragmentShaderEditor.setPlainText(source);
            mFragmentShaderFilename = filename;
            mProject.setFragmentShaderSource(source);
            file.close();
        }
    }
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(tr("%1 %2%3").arg(AppName).arg(AppVersion).arg(mProject.filename().isEmpty()? "" : tr(" - %1%2").arg(mProject.filename()).arg(mProject.isDirty()? "*" : "")));
}

void MainWindow::about(void)
{
    QMessageBox::about(this, tr("About %1 %2%3").arg(AppName).arg(AppVersionNoDebug).arg(AppMinorVersion),
                       tr("<p><b>%1</b> is a live coding environment for OpenGL shaders. "
                          "See <a href=\"%2\" title=\"%1 project homepage\">%2</a> for more info.</p>"
                          "<p>Copyright &copy; 2013 %3 &lt;%4&gt;, Heise Zeitschriften Verlag.</p>"
                          "<p>This program is free software: you can redistribute it and/or modify "
                          "it under the terms of the GNU General Public License as published by "
                          "the Free Software Foundation, either version 3 of the License, or "
                          "(at your option) any later version.</p>"
                          "<p>This program is distributed in the hope that it will be useful, "
                          "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                          "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
                          "GNU General Public License for more details.</p>"
                          "You should have received a copy of the GNU General Public License "
                          "along with this program. "
                          "If not, see <a href=\"http://www.gnu.org/licenses/gpl-3.0\">http://www.gnu.org/licenses</a>.</p>")
                       .arg(AppName).arg(AppUrl).arg(AppAuthor).arg(AppAuthorMail));
}

void MainWindow::aboutQt(void)
{
    QMessageBox::aboutQt(this);
}
