#include <mainwindow.h>
#include <ui_mainwindow.h>
#include <msuproj.h>
#include <QFileDialog>
#include <QResizeEvent>
#include <QDebug>
#include <QMessageBox>

extern MSUMR::MSUProj msuProjObj;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    graphicsScene(new QGraphicsScene(this)),
    fPreffix("")
{
    ui->setupUi(this);
    ui->statusbar->showMessage(tr("Select input files."));
    ui->imageView->setScene(graphicsScene);
    connect(ui->modeLatLonButton, &QRadioButton::clicked, this, &MainWindow::changeOutName);
    connect(ui->modeUTMButton, &QRadioButton::clicked, this, &MainWindow::changeOutName);
    connect(ui->autoOutNameBox, &QCheckBox::toggled, this, &MainWindow::autoOutName);
    connect(ui->statusbar, &QStatusBar::messageChanged, this, &MainWindow::showStdStatus);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showStdStatus(const QString message)
{
    if (message.isEmpty())
        ui->statusbar->showMessage(tr("Select input files."));
}

void MainWindow::loadGCPs(const QString &file)
{
    ui->gcpPathEdit->setText(file);
    msuProjObj.readGCP(file.toStdString());
    ui->gcpRowsLabel->setText(tr("Input GCPs Rows %1").arg(msuProjObj.getGcpXSize()));
    ui->gcpLinesLabel->setText(tr("Input GCPs Lines %1").arg(msuProjObj.getGcpYSize()));
    ui->gcpRowStepLabel->setText(tr("Input GCPs Row Step %1").arg(msuProjObj.getGcpXStep()));
    ui->gcpLineStepLabel->setText(tr("Input GCPs Line Step %1").arg(msuProjObj.getGcpYStep()));
    ui->utmZone->setText(tr("UTM zone %1").arg(msuProjObj.getUTM().c_str()));
}

void MainWindow::changeOutName()
{
    if (ui->autoOutNameBox->isChecked() && !fPreffix.isEmpty())
    {
        QString newName(fPreffix);
        if (ui->modeLatLonButton->isChecked())
            newName += "_proj";
        else
            newName = QString("%1_%2").arg(newName).arg(msuProjObj.getUTM().c_str());
        if (QFile(newName + ".tif").exists())
        {
            unsigned int postfix = 0;
            while (QFile(QString("%1_%2.tif").arg(newName).arg(++postfix)).exists());
            newName = QString("%1_%2.tif").arg(newName).arg(postfix);
        }
        else
            newName += ".tif";
        ui->outPathEdit->setText(newName);
    }
}

void MainWindow::autoOutName(bool state)
{
    ui->outPathEdit->setEnabled(!state);
    ui->outPathButton->setEnabled(!state);
}

void MainWindow::on_imagePathButton_clicked()
{
    QFileDialog openImage(this, tr("Select input image"),
                          QFileInfo(ui->imagePathEdit->text()).path(),
                          tr("Meteor-M2 Images (*.jpg *.bmp);;All files (*.*)"));
    openImage.setFileMode(QFileDialog::ExistingFile);
    if (openImage.exec())
    {
        QString file(openImage.selectedFiles()[0]);
        QStringList gcpFiles(file + ".gcp");

        ui->imagePathEdit->setText(file);
        msuProjObj.setSRC(file.toStdString());
        ui->imageRowsLabel->setText(tr("Input Image Rows: %1").arg(msuProjObj.getSrcXSize()));
        ui->imageLinesLabel->setText(tr("Input Image Lines: %1").arg(msuProjObj.getSrcYSize()));
        graphicsScene->addPixmap(QPixmap(file));
        ui->imageView->fitInView(graphicsScene->sceneRect(), Qt::KeepAspectRatio);
        fPreffix = file.remove(QRegularExpression("\\..*"));

        gcpFiles.append(fPreffix + ".gcp");
        foreach (QString gcpFile, gcpFiles)
            if (QFile(gcpFile).exists())
            {
                this->loadGCPs(gcpFile);
                break;
            }

        this->changeOutName();
    }
    this->on_imagePathEdit_editingFinished();
}

void MainWindow::on_gcpPathButton_clicked()
{
    QString curPath = ui->gcpPathEdit->text();
    if (curPath.isEmpty())
        curPath = ui->imagePathEdit->text();
    QFileDialog openGCPs(this, tr("Select input GCP file"),
                         QFileInfo(curPath).path(),
                         tr("Meteor-M2 GCP file (*.gcp);;All files (*.*)"));
    openGCPs.setFileMode(QFileDialog::ExistingFile);
    if (openGCPs.exec())
        this->loadGCPs(openGCPs.selectedFiles()[0]);
}

void MainWindow::on_imagePathEdit_editingFinished()
{
    ui->gcpBox->setDisabled(ui->imagePathEdit->text().isEmpty());
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    event->accept();
    ui->imageView->fitInView(graphicsScene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::on_startButton_clicked()
{
    if (ui->imagePathEdit->text().isEmpty() ||
        ui->gcpPathEdit->text().isEmpty() ||
        ui->outPathEdit->text().isEmpty())
        QMessageBox::critical(this, tr("Check fields"), tr("All fields must be filled"));
    else
    {
        ui->startButton->setEnabled(false);
        ui->statusbar->showMessage(tr("Transforming image, please wait..."));
        msuProjObj.setDST(ui->outPathEdit->text().toStdString());
        MSUMR::retCode code = msuProjObj.warp(ui->modeUTMButton->isChecked(), ui->modeNDZBox->isChecked());
        if (code == MSUMR::success)
            ui->statusbar->showMessage(tr("Transformation finished successfully."), 7000);
        else
            ui->statusbar->showMessage(tr("An error occured. Please check input data."), 7000);
        this->changeOutName();
        ui->startButton->setEnabled(true);
    }
}
