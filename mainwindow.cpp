// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QSettings>
#include <QProgressBar>
#include <QMessageBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QByteArray>
#include <QTextStream>
#include <QRegExp>
#include <QStringListIterator>
#include <QCryptographicHash>
#include <QDateTime>
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
#include <QMap>
#include <QEvent>
#include <QMimeData>
#include <QListView>
#include <QClipboard>
#include <qmath.h>
#include "main.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "doubleslider.h"
#include "colorpicker.h"
#include "project.h"
#include "channelwidget.h"
#include "renderwidget.h"
#include "renderer.h"
#include "editors/glsl/glsledit.h"
#include "util.h"
#include "editors/js/jsedit.h"
#include "scriptrunner.h"
#include "mediainput.h"

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
#include <Windows.h>
#include <Dbt.h>
#include <Ks.h>
#endif

static void prepareEditor(AbstractEditor*);
static void emptyLayout(QLayout*);


class MainWindowPrivate {
private:
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    DEV_BROADCAST_DEVICEINTERFACE di;
#endif

public:
    explicit MainWindowPrivate(WId winId)
        : project(new Project)
        , colorDialog(new QColorDialog)
        , renderWidget(new RenderWidget)
        , scriptRunner(new ScriptRunner(renderWidget))
        , scriptEditor(new JSEdit)
        , vertexShaderEditor(new GLSLEdit)
        , fragmentShaderEditor(new GLSLEdit)
        , docBrowser(new QTextBrowser)
        , programHasJustStarted(3) // dirty hack
    {
#ifndef WITH_WINDOWS_MEDIA_FOUNDATION
        Q_UNUSED(winId);
#endif

        for (int i = -9; i < 10; ++i)
            steps << qPow(10, i);
        docBrowser->setOpenExternalLinks(true);
        docBrowser->setOpenLinks(true);
        docBrowser->setSource(QUrl("qrc:/doc/index.html"));

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
        di.dbcc_size = sizeof(di);
        di.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        di.dbcc_classguid = KSCATEGORY_CAPTURE;
        di.dbcc_name[0] = 0;
        if (RegisterDeviceNotification((HANDLE)winId, (LPVOID)&di, (DWORD)DEVICE_NOTIFY_WINDOW_HANDLE) == NULL) {
            qWarning() << "RegisterDeviceNotification failed. Error:" << HRESULT_FROM_WIN32(GetLastError());
        }
#endif
    }

    Project* project;
    QColorDialog* colorDialog;
    RenderWidget* renderWidget;
    ChannelWidget* channelWidget[Project::MAX_CHANNELS];
    ScriptRunner* scriptRunner;
    JSEdit* scriptEditor;
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
    QString lastImageOpenDir;
    QString lastImageSaveDir;
    QString lastProjectOpenDir;
    QString lastProjectSaveDir;
    QString lastBatchOpenDir;
    QString lastBatchSaveDir;
    int programHasJustStarted;
    QScriptValue onMousePosChanged;
    QMap<QString, QVariant> oldValues;

    ~MainWindowPrivate()
    {
        safeDelete(project);
        safeDelete(colorDialog);
        safeDelete(renderWidget);
        safeDelete(vertexShaderEditor);
        safeDelete(fragmentShaderEditor);
        safeDelete(docBrowser);
        safeDelete(scriptEditor);
        safeDelete(scriptRunner);
    }
};


QScriptValue scriptPrintFunction(QScriptContext* context, QScriptEngine* engine);

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , d_ptr(new MainWindowPrivate(winId()))
{
    Q_D(MainWindow);

    MediaInput::startup();

    ui->setupUi(this);
    QObject::connect(d->renderWidget, SIGNAL(ready()), SLOT(initAfterGL()));

    for (int i = 0; i < MainWindowPrivate::MaxRecentFiles; ++i) {
        QAction* act = new QAction(this);
        act->setVisible(false);
        d->recentProjectsActs[i] = act;
        QObject::connect(act, SIGNAL(triggered()), SLOT(openRecentProject()));
        ui->menuRecentProjects->addAction(act);
    }
    ui->vsplitter2->insertWidget(0, d->renderWidget);
    ui->tabVertexShader->layout()->addWidget(d->vertexShaderEditor);
    ui->tabFragmentShader->layout()->addWidget(d->fragmentShaderEditor);
    ui->vsplitter->setStretchFactor(0, 5);
    ui->vsplitter->setStretchFactor(1, 1);

    prepareEditor(d->vertexShaderEditor);
    prepareEditor(d->fragmentShaderEditor);

    ui->scriptEditorVerticalLayout->addWidget(d->scriptEditor);
    prepareEditor(d->scriptEditor);
    QObject::connect(d->scriptEditor, SIGNAL(textChanged()), SLOT(processScriptChange()));

    QObject::connect(d->project, SIGNAL(dirtyStateChanged(bool)), SLOT(updateWindowTitle()));
    QObject::connect(d->vertexShaderEditor, SIGNAL(textChangedDelayed()), SLOT(processShaderChange()));
    QObject::connect(d->fragmentShaderEditor, SIGNAL(textChangedDelayed()), SLOT(processShaderChange()));
    QObject::connect(d->renderWidget, SIGNAL(vertexShaderError(QString)), SLOT(badVertexShaderCode(QString)));
    QObject::connect(d->renderWidget, SIGNAL(fragmentShaderError(QString)), SLOT(badFragmentShaderCode(QString)));
    QObject::connect(d->renderWidget, SIGNAL(linkerError(QString)), SLOT(linkerError(QString)));
    QObject::connect(d->renderWidget, SIGNAL(linkerError(QString)), SLOT(linkerError(QString)));
    QObject::connect(d->renderWidget, SIGNAL(linkingSuccessful()), SLOT(successfullyLinkedShader()));
    QObject::connect(d->renderWidget, SIGNAL(imageDropped(QImage)), SLOT(imageDropped(QImage)));
    QObject::connect(d->renderWidget, SIGNAL(fpsChanged(double)), SLOT(setFPS(double)));
    QObject::connect(d->renderWidget, SIGNAL(mousePosChanged(QPointF)), SLOT(setMousePos(QPointF)));
    QObject::connect(ui->actionExit, SIGNAL(triggered()), SLOT(close()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered()), SLOT(about()));
    QObject::connect(ui->actionAboutQt, SIGNAL(triggered()), SLOT(aboutQt()));
    QObject::connect(ui->actionOpen, SIGNAL(triggered()), SLOT(openProject()));
    QObject::connect(ui->actionCloseProject, SIGNAL(triggered()), SLOT(closeProject()));
    QObject::connect(ui->actionLoadImage, SIGNAL(triggered()), SLOT(loadImage()));
    QObject::connect(ui->actionSave, SIGNAL(triggered()), SLOT(saveProject()));
    QObject::connect(ui->actionSaveProjectAs, SIGNAL(triggered()), SLOT(saveProjectAs()));
    QObject::connect(ui->actionNew, SIGNAL(triggered()), SLOT(newProject()));
    QObject::connect(ui->actionSaveImageSnapshot, SIGNAL(triggered()), SLOT(saveImageSnapshot()));
    QObject::connect(ui->actionBatchProcess, SIGNAL(triggered()), SLOT(batchProcess()));
    QObject::connect(ui->actionHelp, SIGNAL(triggered()), SLOT(showHelp()));
    QObject::connect(ui->actionReloadImage, SIGNAL(triggered()), SLOT(reloadImage()));
    QObject::connect(ui->actionTimerActive, SIGNAL(toggled(bool)), SLOT(setTimerActive(bool)));
    QObject::connect(ui->actionFitImageToWindow, SIGNAL(triggered()), d->renderWidget, SLOT(fitImageToWindow()));
    QObject::connect(ui->actionResizeToOriginalImageSize, SIGNAL(triggered()), d->renderWidget, SLOT(resizeToOriginalImageSize()));
    QObject::connect(ui->actionEnableAlpha, SIGNAL(toggled(bool)), d->renderWidget, SLOT(enableAlpha(bool)));
    QObject::connect(ui->actionEnableAlpha, SIGNAL(toggled(bool)), d->project, SLOT(enableAlpha(bool)));
    QObject::connect(ui->actionRecycleImage, SIGNAL(toggled(bool)), d->renderWidget, SLOT(enableImageRecycling(bool)));
    QObject::connect(ui->actionRecycleImage, SIGNAL(toggled(bool)), d->project, SLOT(enableImageRecycling(bool)));
    QObject::connect(ui->actionInstantUpdate, SIGNAL(toggled(bool)), d->renderWidget, SLOT(enableInstantUpdate(bool)));
    QObject::connect(ui->actionInstantUpdate, SIGNAL(toggled(bool)), d->project, SLOT(enableInstantUpdate(bool)));
    QObject::connect(ui->actionClampToBorder, SIGNAL(toggled(bool)), d->renderWidget, SLOT(clampToBorder(bool)));
    QObject::connect(ui->actionClampToBorder, SIGNAL(toggled(bool)), d->project, SLOT(enableBorderClamping(bool)));
    QObject::connect(ui->actionNextFrame, SIGNAL(triggered()), d->renderWidget, SLOT(feedbackOneFrame()));
    QObject::connect(ui->actionChooseBackgroundColor, SIGNAL(triggered()), SLOT(chooseBackgroundColor()));
    QObject::connect(ui->actionCopyImageToClipboard, SIGNAL(triggered()), SLOT(copyToClipboard()));
    QObject::connect(ui->actionPasteImageFromClipboard, SIGNAL(triggered()), SLOT(pasteFromClipboard()));
    QObject::connect(d->colorDialog, SIGNAL(colorSelected(QColor)), d->renderWidget, SLOT(setBackgroundColor(QColor)));
    QObject::connect(d->colorDialog, SIGNAL(colorSelected(QColor)), d->project, SLOT(setBackgroundColor(QColor)));
    QObject::connect(d->colorDialog, SIGNAL(currentColorChanged(QColor)), d->renderWidget, SLOT(setBackgroundColor(QColor)));
    QObject::connect(d->scriptRunner, SIGNAL(debug(const QString&)), SLOT(debug(const QString&)));
    QObject::connect(d->renderWidget, SIGNAL(frameReady()), d->scriptRunner, SLOT(onFrame()));
    QObject::connect(ui->scriptExecutePushButton, SIGNAL(clicked()), SLOT(executeScript()));

    QScriptEngine* engine = d->scriptRunner->engine();
    QScriptValue fPrint = engine->newFunction(scriptPrintFunction);
    fPrint.setData(engine->newQObject(ui->logTextEdit));
    engine->globalObject().setProperty("print", fPrint);

    const QStringList& webcamList = MediaInput::availableDevices();
    for (int i = 0; i < Project::MAX_CHANNELS; ++i) {
        d->channelWidget[i] = new ChannelWidget(i);
        d->channelWidget[i]->setAvailableWebcams(webcamList);
        QObject::connect(d->channelWidget[i], SIGNAL(imageDropped(int, QImage)), SLOT(imageDropped(int, QImage)));
        QObject::connect(d->channelWidget[i],
                         SIGNAL(rawFrameReady(const uchar*, int, int, int, Project::SourceSelector)),
                         SLOT(frameDropped(const uchar*, int, int, int, Project::SourceSelector)));
        QObject::connect(d->channelWidget[i],
                         SIGNAL(rawFrameReady(const uchar*, int, int, Project::SourceSelector)),
                         SLOT(frameDropped(const uchar*, int, int, Project::SourceSelector)));
        QObject::connect(d->channelWidget[i], SIGNAL(camInitialized(int)), SLOT(setCamReady(int)));
        ui->channelLayout->addWidget(d->channelWidget[i]);
    }
    ui->channelLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
    ui->channelScrollArea->setBackgroundRole(QPalette::Dark);

    restoreSettings();
}

void MainWindow::initAfterGL(void)
{
    Q_D(MainWindow);
    QSettings settings(Company, AppName);
    const QString& projectFilename = settings.value("Project/filename").toString();
    if (projectFilename.isEmpty())
        newProject();
    else
        openProject(projectFilename);
    d->project->setClean();
    updateWindowTitle();
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
    d->docBrowser->restoreGeometry(settings.value("DocBrowser/geometry").toByteArray());
    ui->actionEnableAlpha->setChecked((settings.value("Options/alphaEnabled", true).toBool()));
    ui->actionClampToBorder->setChecked((settings.value("Options/clampToBorder", true).toBool()));
    d->renderWidget->setScale(settings.value("Options/zoom", 1.0).toDouble());
    d->colorDialog->setCurrentColor(settings.value("Options/backgroundColor", QColor(20, 20, 20)).value<QColor>());
    bool showdoc = settings.value("DocBrowser/show", false).toBool();
    if (showdoc)
        showHelp();
    const QVariantList& hsz = settings.value("MainWindow/hsplitter/sizes").toList();
    if (!hsz.isEmpty()) {
        QListIterator<QVariant> h(hsz);
        QList<int> hSizes;
        while (h.hasNext())
            hSizes.append(h.next().toInt());
        ui->hsplitter->setSizes(hSizes);
    }
    const QVariantList& vsz = settings.value("MainWindow/vsplitter/sizes").toList();
    if (!vsz.isEmpty()) {
        QListIterator<QVariant> v(vsz);
        QList<int> vSizes;
        while (v.hasNext())
            vSizes.append(v.next().toInt());
        ui->vsplitter->setSizes(vSizes);
    }
    const QVariantList& vsz2 = settings.value("MainWindow/vsplitter2/sizes").toList();
    if (!vsz2.isEmpty()) {
        QListIterator<QVariant> v(vsz2);
        QList<int> vSizes;
        while (v.hasNext())
            vSizes.append(v.next().toInt());
        ui->vsplitter2->setSizes(vSizes);
    }
    ui->tabWidget->setCurrentIndex(settings.value("MainWindow/tabwidget/currentIndex").toInt());
    appendToRecentFileList(QString(), "Project/recentFiles", ui->menuRecentProjects, d->recentProjectsActs);
    d->lastProjectOpenDir = settings.value("Project/lastOpenDir").toString();
    d->lastProjectSaveDir = settings.value("Project/lastSaveDir").toString();
}

void MainWindow::saveSettings(void)
{
    Q_D(MainWindow);
    QSettings settings(Company, AppName);
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("DocBrowser/geometry", d->docBrowser->saveGeometry());
    settings.setValue("DocBrowser/show", d->docBrowser->isVisible());
    settings.setValue("MainWindow/tabwidget/currentIndex", ui->tabWidget->currentIndex());
    const QList<int>& hsz = ui->hsplitter->sizes();
    if (!hsz.isEmpty()) {
        QListIterator<int> h(hsz);
        QVariantList hSizes;
        while (h.hasNext())
            hSizes.append(h.next());
        settings.setValue("MainWindow/hsplitter/sizes", hSizes);
    }
    const QList<int>& vsz = ui->vsplitter->sizes();
    if (!vsz.isEmpty()) {
        QListIterator<int> v(vsz);
        QVariantList vSizes;
        while (v.hasNext())
            vSizes.append(v.next());
        settings.setValue("MainWindow/vsplitter/sizes", vSizes);
    }
    const QList<int>& vsz2 = ui->vsplitter2->sizes();
    if (!vsz2.isEmpty()) {
        QListIterator<int> v(vsz2);
        QVariantList vSizes;
        while (v.hasNext())
            vSizes.append(v.next());
        settings.setValue("MainWindow/vsplitter2/sizes", vSizes);
    }
    settings.setValue("Project/lastOpenDir", d->lastProjectOpenDir);
    settings.setValue("Project/lastSaveDir", d->lastProjectSaveDir);
    settings.setValue("Project/filename", d->project->filename());
    settings.setValue("Options/zoom", d->renderWidget->scale());
    settings.setValue("Options/alphaEnabled", ui->actionEnableAlpha->isChecked());
    settings.setValue("Options/clampToBorder", ui->actionClampToBorder->isChecked());
    settings.setValue("Options/backgroundColor", d->colorDialog->currentColor());
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    Q_D(MainWindow);
    int rc = (d->project->isDirty())
            ? QMessageBox::question(this,
                                    tr("Save before exit?"),
                                    tr("Your project has changed. Do you want to save the changes before exiting?"),
                                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)
            : QMessageBox::NoButton;
    if (rc == QMessageBox::Yes)
        saveProject();
    if (rc != QMessageBox::Cancel) {
        saveSettings();
        safeDelete(d->docBrowser);
        safeDelete(d->colorDialog);
        QMainWindow::closeEvent(e);
        e->accept();
    }
    else {
        e->ignore();
    }
}

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    MSG* msg = reinterpret_cast<MSG*>(message);
    switch (msg->message)
    {
    case WM_DEVICECHANGE:
    {
        qDebug() << "WM_DEVICECHANGE";
        PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)msg->lParam;
        qDebug() << (pHdr->dbch_devicetype != DBT_DEVTYP_DEVICEINTERFACE);
        switch (msg->wParam)
        {
        case DBT_DEVICEARRIVAL:
            qDebug() << "  DBT_DEVICEARRIVAL";
            break;
        case DBT_DEVICEQUERYREMOVE:
            qDebug() << "  DBT_DEVICEQUERYREMOVE";
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            qDebug() << "  DBT_DEVICEREMOVECOMPLETE";
            break;
        default:
            qDebug() << msg->wParam << msg->lParam;
        }
        break;
    }
    default:
        break;
    }
    return false;
}
#endif

void MainWindow::valueChanged(int v)
{
    Q_D(MainWindow);
    if (sender()) {
        const QString& name = sender()->objectName();
        d->renderWidget->setUniformValue(name, v);
        d->project->setDirty();
    }
}

void MainWindow::valueChanged(double v)
{
    Q_D(MainWindow);
    if (sender()) {
        const QString& name = sender()->objectName();
        d->renderWidget->setUniformValue(name, v);
        d->project->setDirty();
    }
}

void MainWindow::valueChanged(bool v)
{
    Q_D(MainWindow);
    if (sender()) {
        const QString& name = sender()->objectName();
        d->renderWidget->setUniformValue(name, v);
        d->project->setDirty();
    }
}

void MainWindow::valueChanged(const QColor& color)
{
    Q_D(MainWindow);
    if (sender()) {
        const QString& name = sender()->objectName();
        d->renderWidget->setUniformValue(name, color);
        d->project->setDirty();
    }
}

void MainWindow::acceptColor(void)
{
    Q_D(MainWindow);
    if (sender()) {
        ColorPicker* const colorPicker = reinterpret_cast<ColorPicker*>(sender());
        const QString& name = colorPicker->objectName();
        d->renderWidget->setUniformValue(name, colorPicker->currentColor());
        d->project->setDirty();
    }
}

void MainWindow::rejectColor(void)
{
    Q_D(MainWindow);
    if (sender()) {
        ColorPicker* const colorPicker = reinterpret_cast<ColorPicker*>(sender());
        const QString& name = colorPicker->objectName();
        d->renderWidget->setUniformValue(name, colorPicker->oldColor());
    }
}

void MainWindow::imageDropped(const QImage&)
{
    processShaderChange();
}

void MainWindow::imageDropped(int index, const QImage& img)
{
    Q_D(MainWindow);
    d->renderWidget->setChannel(index, img);
    d->project->setChannel(index, img);
    processShaderChange();
}

void MainWindow::frameDropped(const uchar* data, int w, int h, int index, Project::SourceSelector)
{
    Q_D(MainWindow);
    d->renderWidget->setChannel(index, data, w, h);
}

void MainWindow::frameDropped(const uchar* data, int length, int index, Project::SourceSelector)
{
    Q_D(MainWindow);
    d->renderWidget->setChannel(index, data, length);
}

void MainWindow::setCamReady(int index)
{
    Q_D(MainWindow);
    d->project->setChannel(index, Project::SourceWebcam);
}

void MainWindow::reloadImage(void)
{
    Q_D(MainWindow);
    d->renderWidget->setImage(d->project->image());
}

void MainWindow::saveImageSnapshot(void)
{
    Q_D(MainWindow);
    const QString& filename =
            QFileDialog::getSaveFileName(this,
                                         tr("Save image snapshot"),
                                         d->lastImageSaveDir,
                                         tr("Image files (*.png *.jpg *.jpeg *.tiff *.ppm)"));
    if (filename.isNull())
        return;
    bool ok = d->renderWidget->resultImage().save(filename);
    if (ok)
        d->lastImageSaveDir = QFileInfo(filename).path();
}

void MainWindow::batchProcess(void)
{
    Q_D(MainWindow);
    const QStringList& filenames =
            QFileDialog::getOpenFileNames(this,
                                          tr("Select images to load"),
                                          d->lastBatchOpenDir,
                                          tr("Image files (*.png *.jpg *.jpeg *.tiff *.ppm)"));
    if (filenames.isEmpty())
        return;
    d->lastBatchOpenDir = QFileInfo(filenames.first()).path();

    const QString& outDir =
            QFileDialog::getExistingDirectory(this,
                                              tr("Select save directory"),
                                              d->lastBatchSaveDir);
    if (outDir.isEmpty())
        return;
    d->lastBatchSaveDir = QFileInfo(outDir).path();
    QCursor oldCursor = cursor();
    setCursor(Qt::WaitCursor);
    QProgressBar progress(ui->logTextEdit);
    progress.setMinimum(0);
    progress.setMaximum(filenames.size());
    progress.setTextVisible(false);
    progress.resize(ui->logTextEdit->size());
    progress.show();

    // weil Renderer von QWidget ableitet, kann er nur im Hauptthread laufen,
    // aber nicht im Hintergrund, etwa per QtConcurrent::run()
    Renderer renderer;
    renderer.buildProgram(d->vertexShaderEditor->toPlainText(), d->fragmentShaderEditor->toPlainText());
    renderer.setUniforms(d->renderWidget->uniforms());

    QStringListIterator fn(filenames);
    int i = 0;
    while (fn.hasNext()) {
        QFileInfo fInfo(fn.next());
        ui->statusBar->showMessage(tr("Processing %1 ...").arg(fInfo.fileName()));
        progress.setValue(++i);
        const QString& outFile = QString("%1/%2").arg(outDir).arg(fInfo.fileName());
        renderer.process(QImage(fInfo.canonicalFilePath()), outFile);
    }
    ui->statusBar->showMessage(tr("Batch processing completed."), 3000);

    setCursor(oldCursor);
}

void MainWindow::chooseBackgroundColor(void)
{
    Q_D(MainWindow);
    d->colorDialog->show();
    d->colorDialog->raise();
    d->colorDialog->activateWindow();
}

void MainWindow::setTimerActive(bool active)
{
    ui->labelFPS->setText(tr("Stopped."));
    d_ptr->renderWidget->setTimerActive(active);
}

void MainWindow::setFPS(double fps)
{
    ui->labelFPS->setText(QString("%1 fps").arg(fps, 7, 'f', 1));
}

void MainWindow::setMousePos(const QPointF& pos)
{
    QScriptValueList args;
    args << pos.x() << pos.y();
    d_ptr->onMousePosChanged.call(QScriptValue(), args);
}

QScriptValue scriptPrintFunction(QScriptContext* context, QScriptEngine* engine)
{
    QString result;
    for (int i = 0; i < context->argumentCount(); ++i) {
        if (i > 0)
            result.append(" ");
        result.append(context->argument(i).toString());
    }
    QScriptValue calleeData = context->callee().data();
    QTextEdit* edit = qobject_cast<QTextEdit*>(calleeData.toQObject());
    edit->append(result);
    return engine->undefinedValue();
}

void MainWindow::executeScript(void)
{
    Q_D(MainWindow);
    ui->logTextEdit->setText(QString());
    if (d->scriptEditor->toPlainText().isEmpty()) {
        debug(tr("Empty script. Doing nothing."));
    }
    else {
        d->scriptRunner->execute(d->scriptEditor->toPlainText());
        d->onMousePosChanged = d->scriptRunner->engine()->globalObject().property("onMousePosChanged");
    }
}

void MainWindow::processScriptChange(void)
{
    Q_D(MainWindow);
    d->project->setScriptSource(d->scriptEditor->toPlainText());
}

void MainWindow::debug(const QString& message)
{
    ui->logTextEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")).arg(message));
}

void MainWindow::parseShadersForParameters(void)
{
    Q_D(MainWindow);
    QByteArray ba = d->vertexShaderEditor->toPlainText().toUtf8()
            .append("\n")
            .append(d->fragmentShaderEditor->toPlainText().toUtf8());
    QTextStream in(&ba);
    QRegExp re0("uniform (float|int|bool)\\s+(\\w+).*//\\s*(.*)");
    QRegExp reColor("uniform vec4\\s+(\\w+).*//\\s*((\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)|(#([0-9a-f]{2,2})([0-9a-f]{2,2})([0-9a-f]{2,2})))");
    QRegExp rePragma("#pragma\\s+size\\s*\\((\\d+)\\s*,\\s*(\\d+)\\)");
    // check if variables' definitions have changed in shader
    QCryptographicHash hash(QCryptographicHash::Sha1);
    while (!in.atEnd()) {
        const QString& line = in.readLine();
        if (re0.indexIn(line) > -1 || rePragma.indexIn(line) > -1 || reColor.indexIn(line) > -1)
            hash.addData(line.toUtf8());
    }
    // if changes occured regenerate widgets
    if (d->currentParameterHash != hash.result()) {
        ui->paramLayout->setEnabled(false); // hinder layout manager from relayouting every time a widget or sublayout is added which is an expensive operation
        emptyLayout(ui->paramLayout);
        QRegExp reFI("([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)\\s*,\\s*([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)\\s*,\\s*([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)");
        QRegExp reB("(true|false)");
        d->currentParameterHash = hash.result();
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
                        QObject::connect(slider, SIGNAL(valueChanged(int)), SLOT(valueChanged(int)));
                        slider->setObjectName(name);
                        slider->setMinimum(minV.toInt());
                        slider->setMaximum(maxV.toInt());
                        slider->setValue(defaultV.toInt());
                        QSpinBox* spinbox = new QSpinBox;
                        QObject::connect(slider, SIGNAL(valueChanged(int)), spinbox, SLOT(setValue(int)));
                        QObject::connect(spinbox, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
                        spinbox->setMinimum(minV.toInt());
                        spinbox->setMaximum(maxV.toInt());
                        spinbox->setValue(defaultV.toInt());
                        innerLayout->addWidget(slider);
                        innerLayout->addWidget(spinbox);
                        ui->paramLayout->addWidget(groupbox);
                    }
                    else if (type == "float") {
                        QVBoxLayout* innerLayout = new QVBoxLayout;
                        QGroupBox* groupbox = new QGroupBox(name);
                        groupbox->setLayout(innerLayout);
                        groupbox->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
                        DoubleSlider* slider = new DoubleSlider(minV.toDouble(), maxV.toDouble(), Qt::Horizontal);
                        slider->setObjectName(name);
                        QObject::connect(slider, SIGNAL(valueChanged(double)), SLOT(valueChanged(double)));
                        slider->setDoubleValue(defaultV.toDouble());
                        QDoubleSpinBox* spinbox = new QDoubleSpinBox;
                        QObject::connect(slider, SIGNAL(valueChanged(double)), spinbox, SLOT(setValue(double)));
                        QObject::connect(spinbox, SIGNAL(valueChanged(double)), slider, SLOT(setDoubleValue(double)));
                        spinbox->setMinimum(minV.toDouble());
                        spinbox->setMaximum(maxV.toDouble());
                        spinbox->setValue(defaultV.toDouble());
                        double x = qCeil(spinbox->maximum()) - qFloor(spinbox->minimum());
                        int decimals = (x < 1)? int(qAbs(log(x))) : 2;
                        spinbox->setDecimals(decimals);
                        for (QVector<double>::const_iterator i = d->steps.constBegin(); i != d->steps.constEnd(); ++i)
                            if (x < *i) { x = *i / 1000; break; }
                        spinbox->setSingleStep(x);
                        innerLayout->addWidget(slider);
                        innerLayout->addWidget(spinbox);
                        ui->paramLayout->addWidget(groupbox);
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
                            QObject::connect(checkbox, SIGNAL(toggled(bool)), SLOT(valueChanged(bool)));
                            checkbox->setObjectName(name);
                            checkbox->setCheckable(true);
                            checkbox->setChecked(b);
                            ui->paramLayout->addWidget(checkbox);
                        }
                        else
                            qWarning() << "invalid type:" << type;
                    }
                }
            }
            else if (rePragma.indexIn(line) > -1) {
                const int w = rePragma.cap(1).toInt();
                const int h = rePragma.cap(2).toInt();
                QImage transparentImage(w, h, QImage::Format_ARGB32);
                transparentImage.fill(Qt::transparent);
                d->renderWidget->setImage(transparentImage);
                d->renderWidget->updateViewport();
            }
            else if (reColor.indexIn(line) > -1) {
                const QString& name = reColor.cap(1).toUtf8();
                QVBoxLayout* innerLayout = new QVBoxLayout;
                QGroupBox* groupbox = new QGroupBox(name);
                groupbox->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
                groupbox->setLayout(innerLayout);
                ColorPicker* colorPicker = new ColorPicker(name);
                QObject::connect(colorPicker, SIGNAL(colorSelected(QColor)), SLOT(valueChanged(QColor)));
                QObject::connect(colorPicker, SIGNAL(currentColorChanged(QColor)), SLOT(valueChanged(QColor)));
                QObject::connect(colorPicker, SIGNAL(accepted()), SLOT(acceptColor()));
                QObject::connect(colorPicker, SIGNAL(rejected()), SLOT(rejectColor()));
                bool ok1, ok2, ok3;
                const QColor& defaultColor = (reColor.cap(2).startsWith("#"))
                        ? QColor(reColor.cap(7).toInt(&ok1, 16), reColor.cap(8).toInt(&ok2, 16), reColor.cap(9).toInt(&ok3, 16))
                        : QColor(reColor.cap(3).toInt(&ok1), reColor.cap(4).toInt(&ok2), reColor.cap(5).toInt(&ok3));
                if (ok1 && ok2 && ok3)
                    colorPicker->setColor(defaultColor);
                innerLayout->addWidget(colorPicker);
                ui->paramLayout->addWidget(groupbox);
            }
        }
        ui->paramLayout->addStretch(1);
        ui->paramLayout->setEnabled(true); // reactivate layout manager
    }
}



void MainWindow::processShaderChange(void)
{
    Q_D(MainWindow);
    parseShadersForParameters();
    updateShaderSources();
    d->project->setDirty(--d->programHasJustStarted < 0); // XXX: dirty hack to counterfeit delayed textChanged() signal from editors
    ui->actionSave->setEnabled(true);
    ui->actionReloadImage->setEnabled(d->project->hasImage());
    updateWindowTitle();
}

void MainWindow::badVertexShaderCode(const QString& msg)
{
    ui->logTextEdit->append(msg);
}

void MainWindow::badFragmentShaderCode(const QString& msg)
{
    ui->logTextEdit->append(msg);
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
    int rc = (d->project->isDirty())
            ? QMessageBox::question(this,
                                    tr("Save before creating a new project?"),
                                    tr("Your project has changed. Do you want to save the changes before creating a new project?"),
                                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)
            : QMessageBox::NoButton;
    if (rc == QMessageBox::Yes)
        saveProject();
    if (rc != QMessageBox::Cancel) {
        openProject(":/examples/default.xml");
        d->project->setFilename(QString());
    }
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
    Q_D(MainWindow);
    int rc = (d->project->isDirty())
            ? QMessageBox::question(this,
                                    tr("Save before loading another project?"),
                                    tr("Your project has changed. Do you want to save the changes before loading another project?"),
                                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)
            : QMessageBox::NoButton;
    if (rc == QMessageBox::Yes)
        saveProject();
    if (rc != QMessageBox::Cancel) {
        const QString& filename =
                QFileDialog::getOpenFileName(this,
                                             tr("Load project"),
                                             d->lastProjectOpenDir,
                                             tr("Project files (*.xml *.xmlz)"));
        if (filename.isEmpty())
            return;
        d->lastProjectOpenDir = QFileInfo(filename).path();
        openProject(filename);
    }
}

void MainWindow::openProject(const QString& filename)
{
    Q_ASSERT(!filename.isEmpty());
    Q_D(MainWindow);
    QFileInfo fInfo(filename);
    if (!fInfo.isReadable() || !fInfo.isFile())
        return;
    bool ok = d->project->load(filename);
    if (ok) {
        ui->actionInstantUpdate->setChecked(false);
        ui->actionRecycleImage->setChecked(false);
        d->renderWidget->stopCode();
        d->vertexShaderEditor->blockSignals(true);
        d->vertexShaderEditor->setPlainText(d->project->vertexShaderSource());
        d->vertexShaderEditor->blockSignals(false);
        d->fragmentShaderEditor->blockSignals(true);
        d->fragmentShaderEditor->setPlainText(d->project->fragmentShaderSource());
        d->fragmentShaderEditor->blockSignals(false);
        d->renderWidget->setImage(d->project->image());
        d->renderWidget->setBackgroundColor(d->project->backgroundColor());
        d->renderWidget->enableAlpha(d->project->alphaEnabled());
        ui->actionEnableAlpha->setChecked(d->project->alphaEnabled());
        d->renderWidget->enableInstantUpdate(d->project->instantUpdateEnabled());
        ui->actionInstantUpdate->setChecked(d->project->instantUpdateEnabled());
        d->renderWidget->clampToBorder(d->project->borderClampingEnabled());
        ui->actionClampToBorder->setChecked(d->project->borderClampingEnabled());
        d->renderWidget->enableImageRecycling(d->project->imageRecyclingEnabled());
        ui->actionRecycleImage->setChecked(d->project->imageRecyclingEnabled());
        for (int i = 0; i < Project::MAX_CHANNELS; ++i) {
            const QVariant& ch = d->project->channel(i);
            switch (ch.type()) {
            case QVariant::Image:
            {
                const QImage& img = ch.value<QImage>();
                d->renderWidget->setChannel(i, img);
                d->channelWidget[i]->setImage(img);
                break;
            }
            default:
                d->renderWidget->setChannel(0);
                d->channelWidget[i]->clear();
                break;
            }
        }
        d->scriptEditor->setPlainText(d->project->scriptSource());
        processShaderChange();
        appendToRecentFileList(filename, "Project/recentFiles", ui->menuRecentProjects, d->recentProjectsActs);
        d->renderWidget->resizeToOriginalImageSize();
        d->renderWidget->setUniforms(d->project->uniforms());
        // update parameter widgets
        QStringListIterator k(d->project->uniforms().keys());
        while (k.hasNext()) {
            const QString& key = k.next();
            const QList<QWidget*>& children = ui->paramLayout->findChildren<QWidget*>(key);
            if (children.size() != 1)
                continue;
            const QVariant& value = d->project->uniforms()[key];
            QListIterator<QWidget*> ch(children);
            while (ch.hasNext()) {
                QWidget* const w = ch.next();
                switch (value.type()) {
                case QVariant::Int:
                    reinterpret_cast<QSlider*>(w)->setValue(value.toInt());
                    break;
                case QVariant::Double:
                    reinterpret_cast<DoubleSlider*>(w)->setDoubleValue(value.toDouble());
                    break;
                case QVariant::Bool:
                    reinterpret_cast<QCheckBox*>(w)->setChecked(value.toBool());
                    break;
                case QVariant::Color:
                    reinterpret_cast<ColorPicker*>(w)->setColor(value.value<QColor>());
                    break;
                default:
                    break;
                }
            }
        }
        d->programHasJustStarted = 3; // dirty hack
        d->project->setClean();
    }
    ui->statusBar->showMessage(ok
                               ? tr("Project '%1' loaded.").arg(QFileInfo(filename).fileName())
                               : tr("Loading '%1' failed: %2").arg(QFileInfo(filename).fileName()).arg(d->project->errorString()), 10000);
    updateWindowTitle();
}

void MainWindow::closeProject(void)
{
    Q_D(MainWindow);
    int rc = (d->project->isDirty())
            ? QMessageBox::question(this,
                                    tr("Save before closing project?"),
                                    tr("Your project has changed. Do you want to save the changes before closing the project?"),
                                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)
            : QMessageBox::NoButton;
    if (rc == QMessageBox::Yes)
        saveProject();
    if (rc != QMessageBox::Cancel) {
        openProject(":/examples/default.xml");
        d->project->setFilename(QString());
    }
}

void MainWindow::saveProject(void)
{
    Q_D(MainWindow);
    QString filename = d->project->filename();
    if (filename.isEmpty()) {
        filename = QFileDialog::getSaveFileName(this,
                                                tr("Save project"),
                                                d->lastProjectSaveDir,
                                                tr("Project files (*.xml *.xmlz)"));
        if (filename.isNull())
            return;
    }
    d->lastProjectSaveDir = QFileInfo(filename).path();
    saveProject(filename);
}

void MainWindow::saveProjectAs(void)
{
    Q_D(MainWindow);
    const QString& filename = QFileDialog::getSaveFileName(this,
                                                           tr("Save project"),
                                                           d->lastProjectSaveDir,
                                                           tr("Project files (*.xml *.xmlz)"));
    if (filename.isNull())
        return;
    saveProject(filename);
}

void MainWindow::saveProject(const QString& filename)
{
    Q_D(MainWindow);
    d->project->setFilename(filename);
    d->project->setVertexShaderSource(d->vertexShaderEditor->toPlainText());
    d->project->setFragmentShaderSource(d->fragmentShaderEditor->toPlainText());
    d->project->setImage(d->renderWidget->image());
    d->project->setUniforms(d->renderWidget->uniforms());
    bool ok = d->project->save();
    ui->statusBar->showMessage(ok
                               ? tr("Project saved as '%1'.").arg(QFileInfo(filename).fileName())
                               : tr("Saving failed."), 3000);
    if (ok)
        appendToRecentFileList(filename, "Project/recentFiles", ui->menuRecentProjects, d->recentProjectsActs);
    updateWindowTitle();
}

void MainWindow::loadImage(void)
{
    Q_D(MainWindow);
    int rc = (d->project->isDirty())
            ? QMessageBox::question(this,
                                    tr("Save project before loading a new image?"),
                                    tr("Your project has changed. Do you want to save the changes before loading a new image?"),
                                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)
            : QMessageBox::NoButton;
    if (rc == QMessageBox::Yes)
        saveProject();
    if (rc != QMessageBox::Cancel) {
        const QString& filename =
                QFileDialog::getOpenFileName(this,
                                             tr("Load image"),
                                             d->lastImageOpenDir,
                                             tr("Image files (*.png *.jpg *.jpeg *.gif *.ico *.mng *.tga *.tif)"));
        if (filename.isEmpty())
            return;
        d->lastImageOpenDir = QFileInfo(filename).path();
        loadImage(filename);
    }

}

void MainWindow::loadImage(const QString& filename)
{
    Q_D(MainWindow);
    QFileInfo fInfo(filename);
    if (!fInfo.isReadable() || !fInfo.isFile())
        return;
    const QImage& img = QImage(filename);
    d->project->setImage(img);
    d->renderWidget->setImage(img);
    processShaderChange();
}

void MainWindow::updateShaderSources(void)
{
    Q_D(MainWindow);
    d->renderWidget->setShaderSources(d->vertexShaderEditor->toPlainText(),
                                      d->fragmentShaderEditor->toPlainText());
}

void MainWindow::copyToClipboard(void)
{
    Q_D(MainWindow);
    QApplication::clipboard()->setImage(d->renderWidget->resultImage(), QClipboard::Clipboard);
    ui->statusBar->showMessage(tr("Image copied to clipboard."), 5000);
}

void MainWindow::pasteFromClipboard(void)
{
    Q_D(MainWindow);
    if (QApplication::clipboard()->mimeData()->hasImage()) {
        const QPixmap &pix = QApplication::clipboard()->pixmap(QClipboard::Clipboard);
        if (!pix.isNull()) {
            const QImage& img = pix.toImage();
            d->project->setImage(img);
            d->renderWidget->loadImage(img);
            processShaderChange();
        }
    }
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(tr("%1 %2 (OpenGL %3) %4")
                   .arg(AppName)
                   .arg(AppVersion)
                   .arg(d_ptr->renderWidget->glVersionString())
                   .arg(d_ptr->project->filename().isEmpty()
                        ? ""
                        : tr("- %1%2")
                          .arg(d_ptr->project->filename().isEmpty()? tr("<untitled>") : d_ptr->project->filename())
                          .arg(d_ptr->project->isDirty()? "*" : "")));
}

void MainWindow::about(void)
{
    QMessageBox::about(this, tr("About %1 %2%3").arg(AppName).arg(AppVersionNoDebug).arg(AppMinorVersion),
                       tr("<p><b>%1</b> is a live coding environment for OpenGL shaders. "
                          "See <a href=\"%2\" title=\"%1 project homepage\">%2</a> for more info.</p>"
                          "<p>Copyright &copy; 2013 %3 &lt;%4&gt;, Heise Zeitschriften Verlag.</p>"
                          "<p>Editor widgets copyright &copy; 2010&ndash;2011 Ariya Hidayat &lt;ariya.hidayat@gmail.com&gt;.</p>"
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

void MainWindow::showHelp(void)
{
    Q_D(MainWindow);
    d->docBrowser->show();
    d->docBrowser->activateWindow();
    d->docBrowser->raise();
}

static void prepareEditor(AbstractEditor* editor)
{
    editor->setWordWrapMode(QTextOption::NoWrap);
    QFont monospace;
#ifdef Q_OS_MAC
    monospace.setPointSize(10);
    monospace.setFamily("Monaco");
#else
    monospace.setPointSize(8);
    monospace.setFamily("Monospace");
#endif
    monospace.setStyleHint(QFont::TypeWriter);
    const int tabStop = 2;
    editor->setFont(monospace);
    QFontMetrics metrics(monospace);
    editor->setTabStopWidth(tabStop * metrics.width(' '));
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

static void emptyLayout(QLayout* layout)
{
    Q_ASSERT(layout != NULL);
    if (layout == NULL)
        return;
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != NULL) {
        QLayout* subLayout = item->layout();
        QWidget* widget = item->widget();
        if (subLayout) {
            subLayout->removeItem(item);
            emptyLayout(subLayout);
        }
        else if (widget) {
            widget->hide();
            delete widget;
        }
        else {
            delete item;
        }
    }
}
