// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QGridLayout>
#include <QSettings>
#include "main.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mRenderWidget(new RenderWidget)
{
    ui->setupUi(this);
    ui->horizontalLayout->addWidget(mRenderWidget);
    ui->vertexShaderHLayout->addWidget(&mVertexShaderEditor);
    ui->fragmentShaderHLayout->addWidget(&mFragmentShaderEditor);

    prepareEditor(mVertexShaderEditor);
    prepareEditor(mFragmentShaderEditor);
    QObject::connect(&mVertexShaderEditor, SIGNAL(textChanged()), SLOT(vertexShaderChanged()));
    QObject::connect(&mFragmentShaderEditor, SIGNAL(textChanged()), SLOT(fragmentShaderChanged()));

    restoreSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::restoreSettings(void)
{
    QSettings settings(Company, AppName);
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    mFragmentShaderFilename = settings.value("Options/FragmentShaderEditor/scriptFilename", ":/shaders/fragmentshader.glsl").toString();
    loadFragmentShader(mFragmentShaderFilename);
    mVertexShaderFilename = settings.value("Options/VertexShaderEditor/scriptFilename", ":/shaders/vertexshader.glsl").toString();
    loadVertexShader(mVertexShaderFilename);
}


void MainWindow::saveSettings(void)
{
    QSettings settings(Company, AppName);
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("Options/VertexShaderEditor/scriptFilename", mVertexShaderFilename);
    settings.setValue("Options/VertexShaderEditor/script", mVertexShaderEditor.toPlainText());
    settings.setValue("Options/FragmentShaderEditor/scriptFilename", mFragmentShaderFilename);
    settings.setValue("Options/FragmentShaderEditor/script", mFragmentShaderEditor.toPlainText());
}


void MainWindow::closeEvent(QCloseEvent* e)
{
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

void MainWindow::vertexShaderChanged(void)
{
    qDebug() << "MainWindow::vertexShaderChanged()";
    mRenderWidget->setShaderSources(mVertexShaderEditor.toPlainText(), mFragmentShaderEditor.toPlainText());
}

void MainWindow::fragmentShaderChanged(void)
{
    qDebug() << "MainWindow::fragmentShaderChanged()" << mFragmentShaderEditor.toPlainText();
    mRenderWidget->setShaderSources(mVertexShaderEditor.toPlainText(), mFragmentShaderEditor.toPlainText());
}

void MainWindow::loadVertexShader(const QString& filename)
{
    if (!filename.isEmpty()) {
        QFile file(filename);
        bool success = file.open(QIODevice::ReadOnly | QIODevice::Text);
        if (success) {
            const QString& script = file.readAll();
            mVertexShaderEditor.setPlainText(script);
            file.close();
            mVertexShaderFilename = filename;
        }
    }
}

void MainWindow::loadFragmentShader(const QString& filename)
{
    if (!filename.isEmpty()) {
        QFile file(filename);
        bool success = file.open(QIODevice::ReadOnly | QIODevice::Text);
        if (success) {
            const QString& script = file.readAll();
            mFragmentShaderEditor.setPlainText(script);
            file.close();
            mFragmentShaderFilename = filename;
        }
    }
}
