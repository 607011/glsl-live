// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QTextStream>
#include <QRegExp>
#include <QCryptographicHash>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QWidget>
#include <QTextBrowser>
#include <QLayout>
#include <QTimer>
#include <QByteArray>
#include <QString>
#include <QVector>
#include <qmath.h>
#include "main.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "doubleslider.h"
#include "project.h"
#include "renderwidget.h"
#include "glsledit/glsledit.h"
#include "util.h"


static void prepareEditor(GLSLEdit* editor);
static void clearLayout(QLayout* layout);


class MainWindowPrivate {
public:
    explicit MainWindowPrivate(void)
        : renderWidget(new RenderWidget)
        , paramWidget(new QWidget)
        , vertexShaderEditor(new GLSLEdit)
        , fragmentShaderEditor(new GLSLEdit)
        , docBrowser(NULL)
    {
        calcSteps();
    }

    Project project;
    RenderWidget* renderWidget;
    QWidget* paramWidget;
    QString vertexShaderFilename;
    QString fragmentShaderFilename;
    QString imageFilename;
    GLSLEdit* vertexShaderEditor;
    GLSLEdit* fragmentShaderEditor;
    QByteArray currentParameterHash;
    QTextBrowser* docBrowser;
    QVector<double> steps;
    static const int MaxRecentFiles = 16;
    QAction* recentProjectsActs[MaxRecentFiles];

    virtual ~MainWindowPrivate()
    {
        safeDelete(renderWidget);
        safeDelete(paramWidget);
        safeDelete(vertexShaderEditor);
        safeDelete(fragmentShaderEditor);
        safeDelete(docBrowser);
    }

private:
    void calcSteps(void)
    {
        for (int i = -9; i < 10; ++i)
            steps << qPow(10, i);
    }

};


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , d_ptr(new MainWindowPrivate)
{
    Q_D(MainWindow);
    ui->setupUi(this);
    for (int i = 0; i < MainWindowPrivate::MaxRecentFiles; ++i) {
        QAction* act = new QAction(this);
        act->setVisible(false);
        d->recentProjectsActs[i] = act;
        QObject::connect(act, SIGNAL(triggered()), SLOT(openRecentProject()));
        ui->menuRecentProjects->addAction(act);
    }
    ui->hsplitter->addWidget(d->renderWidget);
    ui->hsplitter->addWidget(d->paramWidget);
    ui->tabWidget->setMinimumWidth(300);
    ui->vertexShaderHLayout->addWidget(d->vertexShaderEditor);
    ui->fragmentShaderHLayout->addWidget(d->fragmentShaderEditor);
    ui->hsplitter->setStretchFactor(0, 3);
    ui->hsplitter->setStretchFactor(1, 2);
    ui->hsplitter->setStretchFactor(2, 1);
    ui->vsplitter->setStretchFactor(0, 5);
    ui->vsplitter->setStretchFactor(1, 1);
    prepareEditor(d->vertexShaderEditor);
    prepareEditor(d->fragmentShaderEditor);
    QObject::connect(d->vertexShaderEditor, SIGNAL(textChangedDelayed()), SLOT(processShaderChange()));
    QObject::connect(d->fragmentShaderEditor, SIGNAL(textChangedDelayed()), SLOT(processShaderChange()));
    QObject::connect(d->renderWidget, SIGNAL(vertexShaderError(QString)), SLOT(badVertexShaderCode(QString)));
    QObject::connect(d->renderWidget, SIGNAL(fragmentShaderError(QString)), SLOT(badFragmentShaderCode(QString)));
    QObject::connect(d->renderWidget, SIGNAL(linkerError(QString)), SLOT(linkerError(QString)));
    QObject::connect(d->renderWidget, SIGNAL(linkerError(QString)), SLOT(linkerError(QString)));
    QObject::connect(d->renderWidget, SIGNAL(linkingSuccessful()), SLOT(successfullyLinkedShader()));
    QObject::connect(ui->actionExit, SIGNAL(triggered()), SLOT(close()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered()), SLOT(about()));
    QObject::connect(ui->actionAboutQt, SIGNAL(triggered()), SLOT(aboutQt()));
    QObject::connect(ui->actionOpen, SIGNAL(triggered()), SLOT(openProject()));
    QObject::connect(ui->actionSave, SIGNAL(triggered()), SLOT(saveProject()));
    QObject::connect(ui->actionSaveProjectAs, SIGNAL(triggered()), SLOT(saveProjectAs()));
    QObject::connect(ui->actionNew, SIGNAL(triggered()), SLOT(newProject()));
    QObject::connect(ui->actionSaveImageSnapshot, SIGNAL(triggered()), SLOT(saveImageSnapshot()));
    QObject::connect(ui->actionHelp, SIGNAL(triggered()), SLOT(showHelp()));
    QObject::connect(ui->actionFitImageToWindow, SIGNAL(triggered()), d->renderWidget, SLOT(fitImageToWindow()));
    QObject::connect(ui->actionResizeToOriginalImageSize, SIGNAL(triggered()), d->renderWidget, SLOT(resizeToOriginalImageSize()));
    restoreSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::restoreSettings(void)
{
    Q_D(MainWindow);
    QSettings settings(Company, AppName);
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    ui->hsplitter->restoreGeometry(settings.value("MainWindow/hsplitter/geometry").toByteArray());
    ui->vsplitter->restoreGeometry(settings.value("MainWindow/vsplitter/geometry").toByteArray());
    ui->tabWidget->setCurrentIndex(settings.value("MainWindow/tabwidget/currentIndex").toInt());
    appendToRecentFileList(QString(), "Project/recentFiles", ui->menuRecentProjects, d->recentProjectsActs);
    const QString& projectFilename = settings.value("Project/filename").toString();
    if (projectFilename.isEmpty()) {
        newProject();
    }
    else {
        openProject(projectFilename);
    }
    d_ptr->project.setDirty(false);
    updateWindowTitle();
}

void MainWindow::saveSettings(void)
{
    QSettings settings(Company, AppName);
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/tabwidget/currentIndex", ui->tabWidget->currentIndex());
    settings.setValue("MainWindow/hsplitter/geometry", ui->hsplitter->saveGeometry());
    settings.setValue("MainWindow/vsplitter/geometry", ui->vsplitter->saveGeometry());
    settings.setValue("Project/filename", d_ptr->project.filename());
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    if (d_ptr->project.isDirty()) {
        bool ok = QMessageBox::question(this, tr("Save before exit?"), tr("Your project has changed. Do you want to save the changes before exiting?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
        if (ok)
            saveProject();
    }
    saveSettings();
    e->accept();
}

void MainWindow::valueChanged(int v)
{
    if (sender()) {
        const QString& name = sender()->objectName();
        d_ptr->renderWidget->setUniformValue(name, v);
    }
}

void MainWindow::valueChanged(double v)
{
    if (sender()) {
        const QString& name = sender()->objectName();
        d_ptr->renderWidget->setUniformValue(name, v);
    }
}

void MainWindow::valueChanged(bool v)
{
    if (sender()) {
        const QString& name = sender()->objectName();
        d_ptr->renderWidget->setUniformValue(name, v);
    }
}

void MainWindow::saveImageSnapshot(void)
{
    const QString& filename = QFileDialog::getSaveFileName(this, tr("Save image snapshot"), QString(), tr("Image files (*.png *.jpg *.jpeg *.tiff *.ppm)"));
    if (filename.isNull())
        return;
    d_ptr->renderWidget->resultImage().save(filename);
}

void MainWindow::parseShadersForParameters()
{
    Q_D(MainWindow);
    QByteArray ba = d->vertexShaderEditor->toPlainText().toUtf8()
            .append("\n")
            .append(d->fragmentShaderEditor->toPlainText().toUtf8());
    QTextStream in(&ba);
    QRegExp re0("uniform (float|int|bool)\\s+(\\w+).*//\\s*(.*)\\s*$");
    // check if variables' definitions have changed in shader
    QCryptographicHash hash(QCryptographicHash::Sha1);
    while (!in.atEnd()) {
        const QString& line = in.readLine();
        if (re0.indexIn(line) > -1)
            hash.addData(line.toUtf8());
    }
    // if changes occured regenerate widgets
    if (d->currentParameterHash != hash.result()) {
        QRegExp reFI("([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)\\s*,\\s*([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)\\s*,\\s*([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)");
        QRegExp reB("(true|false)");
        d->currentParameterHash = hash.result();
        QVBoxLayout* layout = new QVBoxLayout;
        d->renderWidget->clearUniforms();
        in.seek(0);
        while (!in.atEnd()) {
            const QString& line = in.readLine();
            if (re0.indexIn(line) > -1) {
                const QString& type = re0.cap(1).toUtf8();
                const QString& name = re0.cap(2).toUtf8();
                const QString& minMaxDefault = re0.cap(3).toUtf8();
                if (reFI.indexIn(minMaxDefault) > -1) {
                    const QString& minV = reFI.cap(1).toUtf8();
                    const QString& maxV = reFI.cap(3).toUtf8();
                    const QString& defaultV = reFI.cap(5).toUtf8();
                    if (type == "int") {
                        QVBoxLayout* innerLayout = new QVBoxLayout;
                        QGroupBox* groupbox = new QGroupBox(name);
                        groupbox->setLayout(innerLayout);
                        groupbox->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
                        QSlider* slider = new QSlider(Qt::Horizontal);
                        innerLayout->addWidget(slider);
                        connect(slider, SIGNAL(valueChanged(int)), SLOT(valueChanged(int)));
                        slider->setObjectName(name);
                        slider->setMinimum(minV.toInt());
                        slider->setMaximum(maxV.toInt());
                        slider->setValue(defaultV.toInt());
                        QSpinBox* spinbox = new QSpinBox;
                        innerLayout->addWidget(spinbox);
                        spinbox->setObjectName(name);
                        spinbox->setMinimum(minV.toInt());
                        spinbox->setMaximum(maxV.toInt());
                        spinbox->setValue(defaultV.toInt());
                        connect(slider, SIGNAL(valueChanged(int)), spinbox, SLOT(setValue(int)));
                        connect(spinbox, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
                        layout->addWidget(groupbox);
                    }
                    else if (type == "float") {
                        QVBoxLayout* innerLayout = new QVBoxLayout;
                        QGroupBox* groupbox = new QGroupBox(name);
                        groupbox->setLayout(innerLayout);
                        groupbox->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
                        DoubleSlider* slider = new DoubleSlider(minV.toDouble(), maxV.toDouble(), Qt::Horizontal);
                        slider->setObjectName(name);
                        slider->setDoubleValue(defaultV.toDouble());
                        innerLayout->addWidget(slider);
                        QDoubleSpinBox* spinbox = new QDoubleSpinBox;
                        innerLayout->addWidget(spinbox);
                        connect(slider, SIGNAL(valueChanged(double)), spinbox, SLOT(setValue(double)));
                        connect(spinbox, SIGNAL(valueChanged(double)), SLOT(valueChanged(double)));
                        spinbox->setObjectName(name);
                        spinbox->setMinimum(minV.toDouble());
                        spinbox->setMaximum(maxV.toDouble());
                        spinbox->setValue(defaultV.toDouble());
                        double x = spinbox->maximum() - spinbox->minimum();
                        int decimals = (x < 1)? int(qAbs(log(x))) : 2;
                        spinbox->setDecimals(decimals);
                        for (QVector<double>::const_iterator i = d->steps.constBegin(); i != d->steps.constEnd(); ++i)
                            if (x < *i) { x = *i / 1000; break; }
                        spinbox->setSingleStep(x);
                        connect(spinbox, SIGNAL(valueChanged(double)), slider, SLOT(setDoubleValue(double)));
                        layout->addWidget(groupbox);
                    }
                    else
                        qWarning() << "invalid type:" << type;
                }
                else {
                    if (reB.indexIn(minMaxDefault) > -1) {
                        const QString& v = reB.cap(1).toUtf8();
                        if (type == "bool") {
                            bool b = (v == "true");
                            QCheckBox* checkbox = new QCheckBox(name);
                            connect(checkbox, SIGNAL(toggled(bool)), SLOT(valueChanged(bool)));
                            checkbox->setObjectName(name);
                            checkbox->setCheckable(true);
                            checkbox->setChecked(b);
                            layout->addWidget(checkbox);
                        }
                        else
                            qWarning() << "invalid type:" << type;
                    }
                }
            }
        }
        layout->addStretch(1);
        if (d->paramWidget->layout() != NULL)
            clearLayout(d->paramWidget->layout());
        d->paramWidget->setLayout(layout);
    }
}

void MainWindow::processShaderChange(void)
{
    parseShadersForParameters();
    updateShaderSources();
    d_ptr->project.setDirty(true);
    updateWindowTitle();
}

void MainWindow::badVertexShaderCode(const QString& msg)
{
    ui->logTextEdit->append(msg);
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::badFragmentShaderCode(const QString& msg)
{
    ui->logTextEdit->append(msg);
    ui->tabWidget->setCurrentIndex(1);
}

void MainWindow::linkerError(const QString& msg)
{
    ui->logTextEdit->append(msg);
}

void MainWindow::successfullyLinkedShader(void)
{
    ui->logTextEdit->clear();
}

void MainWindow::newProject(void)
{
    Q_D(MainWindow);
    if (d->project.isDirty()) {
        bool ok = QMessageBox::question(this, tr("Really create a new project?"), tr("Your project has changed. Do you want to save the changes before creating a new project?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
        if (ok)
            saveProject();
    }
    d->project.reset();
    d->vertexShaderEditor->setPlainText(d->project.vertexShaderSource());
    d->fragmentShaderEditor->setPlainText(d->project.fragmentShaderSource());
    d->renderWidget->setImage(d->project.image());
    processShaderChange();
    d->project.setClean();
    updateWindowTitle();
}

void MainWindow::appendToRecentFileList(const QString& filename, const QString& listname, QMenu* menu, QAction* actions[])
{
    QSettings settings(Company, AppName);
    QStringList files = settings.value(listname).toStringList();
    if (!filename.isEmpty()) {
        QFileInfo fInfo(filename);
        if (fInfo.isFile() && fInfo.isReadable()) {
            files.removeAll(fInfo.canonicalFilePath());
            files.prepend(fInfo.canonicalFilePath());
        }
        while (files.size() > MainWindowPrivate::MaxRecentFiles)
            files.removeLast();
    }
    QStringList updatedFiles;
    QStringListIterator file(files);
    while (file.hasNext() && updatedFiles.size() < MainWindowPrivate::MaxRecentFiles) {
        QFileInfo fInfo(file.next());
        // lesbare Dateien behalten, Duplikate verwerfen
        if (!updatedFiles.contains(fInfo.canonicalFilePath()) && fInfo.isFile() && fInfo.isReadable()) {
            const int i = updatedFiles.size();
            const QString& text = tr("&%1 %2").arg(i).arg(fInfo.fileName());
            actions[i]->setText(text);
            actions[i]->setStatusTip(fInfo.canonicalFilePath());
            actions[i]->setData(fInfo.canonicalFilePath());
            actions[i]->setVisible(true);
            updatedFiles.append(fInfo.canonicalFilePath());
        }
    }
    for (int i = files.size(); i < MainWindowPrivate::MaxRecentFiles; ++i)
        actions[i]->setVisible(false);
    menu->setEnabled(updatedFiles.size() > 0);
    settings.setValue(listname, updatedFiles);
}

void MainWindow::openRecentProject(void)
{
    const QAction* const action = qobject_cast<QAction*>(sender());
    if (action)
        openProject(action->data().toString());
}

void MainWindow::openProject(void)
{
    if (d_ptr->project.isDirty()) {
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
    Q_D(MainWindow);
    bool ok = d->project.load(filename);
    if (ok) {
        d->vertexShaderEditor->blockSignals(true);
        d->vertexShaderEditor->setPlainText(d->project.vertexShaderSource());
        d->vertexShaderEditor->blockSignals(false);
        d->fragmentShaderEditor->blockSignals(true);
        d->fragmentShaderEditor->setPlainText(d->project.fragmentShaderSource());
        d->fragmentShaderEditor->blockSignals(false);
        d->renderWidget->setImage(d->project.image());
        processShaderChange();
        d->project.setClean();
        appendToRecentFileList(filename, "Project/recentFileList", ui->menuRecentProjects, d->recentProjectsActs);
    }
    ui->statusBar->showMessage(ok
                               ? tr("Project '%1' loaded.").arg(QFileInfo(filename).fileName())
                               : tr("Loading '%1' failed.").arg(QFileInfo(filename).fileName()), 3000);
    updateWindowTitle();
}

void MainWindow::saveProject(void)
{
    QString filename = d_ptr->project.filename();
    if (filename.isEmpty()) {
        filename = QFileDialog::getSaveFileName(this, tr("Save project"), QString(), tr("Project files (*.xml *.xmlz)"));
        if (filename.isNull())
            return;
    }
    saveProject(filename);
}

void MainWindow::saveProjectAs(void)
{
    const QString& filename = QFileDialog::getSaveFileName(this, tr("Save project"), QString(), tr("Project files (*.xml *.xmlz)"));
    if (filename.isNull())
        return;
    saveProject(filename);
}

void MainWindow::saveProject(const QString &filename)
{
    Q_D(MainWindow);
    d->project.setFilename(filename);
    d->project.setVertexShaderSource(d->vertexShaderEditor->toPlainText());
    d->project.setFragmentShaderSource(d->fragmentShaderEditor->toPlainText());
    d->project.setImage(d->renderWidget->image());
    bool ok = d->project.save();
    ui->statusBar->showMessage(ok
                               ? tr("Project saved as '%1'.").arg(QFileInfo(filename).fileName())
                               : tr("Saving failed."), 3000);
    if (ok)
        appendToRecentFileList(filename, "Project/recentFileList", ui->menuRecentProjects, d->recentProjectsActs);
    updateWindowTitle();
}

void MainWindow::updateShaderSources(void)
{
    Q_D(MainWindow);
    d->renderWidget->setShaderSources(d->vertexShaderEditor->toPlainText(),
                                      d->fragmentShaderEditor->toPlainText());
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(tr("%1 %2%3")
                   .arg(AppName)
                   .arg(AppVersion)
                   .arg(d_ptr->project.filename().isEmpty()
                        ? ""
                        : tr(" - %1%2")
                          .arg(d_ptr->project.filename())
                          .arg(d_ptr->project.isDirty()? "*" : "")));
}

void MainWindow::showHelp(void)
{
    Q_D(MainWindow);
    QFile docFile(":/doc/index.html");
    docFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if (d->docBrowser == NULL) {
        d->docBrowser = new QTextBrowser;
        d->docBrowser->setOpenExternalLinks(true);
        d->docBrowser->setOpenLinks(true);
    }
    d->docBrowser->setText(docFile.readAll());
    d->docBrowser->show();
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


static void prepareEditor(GLSLEdit* editor)
{
    editor->setTabStopWidth(2);
    editor->setWordWrapMode(QTextOption::NoWrap);
    QFont monospace;
#if defined(Q_OS_MAC)
    monospace.setPointSize(10);
    monospace.setFamily("Monaco");
#else
    monospace.setPointSize(8);
    monospace.setFamily("Monospace");
#endif
    monospace.setStyleHint(QFont::TypeWriter);
    editor->setFont(monospace);
    editor->setTextWrapEnabled(true);
    editor->setBracketsMatchingEnabled(true);
    editor->setCodeFoldingEnabled(true);
    editor->setStyleSheet("background-color: rgba(20, 20, 20, 200)");
    editor->setColor(GLSLEdit::Background,    QColor(20, 20, 20, 200));
    editor->setColor(GLSLEdit::Normal,        QColor("#eeeeee"));
    editor->setColor(GLSLEdit::Comment,       QColor("#baaab0"));
    editor->setColor(GLSLEdit::Number,        QColor("#e0f060"));
    editor->setColor(GLSLEdit::Operator,      QColor("#f08030"));
    editor->setColor(GLSLEdit::Identifier,    QColor("#eeeeee"));
    editor->setColor(GLSLEdit::Keyword,       QColor("#60d040"));
    editor->setColor(GLSLEdit::BuiltIn,       QColor("#9cb6d4"));
    editor->setColor(GLSLEdit::Cursor,        QColor("#1e346b"));
    editor->setColor(GLSLEdit::Marker,        QColor("#e0f060"));
    editor->setColor(GLSLEdit::BracketMatch,  QColor("#1ab0a6"));
    editor->setColor(GLSLEdit::BracketError,  QColor("#a82224"));
    editor->setColor(GLSLEdit::FoldIndicator, QColor("#555555"));
}

static void clearLayout(QLayout* layout)
{
    Q_ASSERT(layout != NULL);
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != NULL) {
        QLayout* subLayout = item->layout();
        QWidget* widget = item->widget();
        if (subLayout) {
            subLayout->removeItem(item);
            clearLayout(subLayout);
        }
        else if (widget) {
            widget->hide();
            delete widget;
        }
        else {
            delete item;
        }
    }
    delete layout;
}
