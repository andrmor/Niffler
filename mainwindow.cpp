#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mywebserver.h"
#include "arootmanager.h"
#include "utils.h"

#include "nifti1_io.h"
#include "_et_array_interface.h"

#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <QDebug>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QMessageBox>
#include <QDoubleValidator>

#include "TH3F.h"
#include "TH2F.h"
#include "TH1F.h"
#include "TPad.h"
#include "TCanvas.h"
#include "TStyle.h"

#define BACKGROUND_ACTIVITY 0.0f
#define BACKGROUND_ATTENUATION 0.0f
#define EPSILON 0.0000001f

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    Root = new ARootManager();

    activity_data = 0;
    h = 0;
    fAlreadyShown = false;
    fDataLoadedFromFiles = false;

    WebServer = new MyWebServer(1234, false);
    connect(WebServer, &MyWebServer::NotifyMessageReceived, this, &MainWindow::onWebMessageReceived);

    //=================== Default Config ===================
    size_x = 60;
    size_y = 60;
    n_cameras = 10;
    firstcamera = 0;
    lastcamera = 180;
    axis = 0;
    //radius = 30;
    iterations = 20;
    gpu = 0; //off
    subsets = 1;// so using MLEM (OSEM if value >1)    
    use_psf = 0;
    PSF_fwhm0 = 0.5;
    PSF_fwhm1 = 1;
    PSF_dist0 = 15;
    PSF_dist1 = 30;
    PSF_efficiency0 = 1;
    PSF_efficiency1 = 1;

    ui->pbUpdateSettings->setVisible(false);
    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);

    UpdateGui();
}

MainWindow::~MainWindow()
{
    //if (activity_data) free(activity_data);
    delete [] activity_data;
    delete Root;
    delete WebServer;
    delete ui;
}

void MainWindow::UpdateGui()
{
    ui->sbSizeX->setValue(size_x);
    ui->sbSizeY->setValue(size_y);
    ui->sbCameras->setValue(n_cameras);
    ui->ledCamerasStart->setText(QString::number(firstcamera));
    ui->ledCameraFinish->setText(QString::number(lastcamera));
    ui->cobAxis->setCurrentIndex(axis);

    ui->sbIterations->setValue(iterations);
    ui->cobMethod->setCurrentIndex( (subsets<=1 ? 0 : 1));
    ui->sbOSEMsubsets->setValue(subsets);

    ui->cbUsePSF->setChecked(use_psf);
    ui->ledDist0->setText(QString::number(PSF_dist0));
    ui->ledDist1->setText(QString::number(PSF_dist1));
    ui->ledFWHM0->setText(QString::number(PSF_fwhm0));
    ui->ledFWHM1->setText(QString::number(PSF_fwhm1));
    ui->ledEff0->setText(QString::number(PSF_efficiency0));
    ui->ledEff1->setText(QString::number(PSF_efficiency1));

    ui->pbReconstruct->setEnabled( h!=0 );

    ui->labUrl->setText(WebServer->GetUrl());

    ui->pbReconstruct->setEnabled( ProjectionStack.size()==size_x*size_y*n_cameras );
}

void MainWindow::on_pbUpdateSettings_clicked()
{
    //qDebug() << "updating settings";
    size_x = ui->sbSizeX->value();
    size_y = ui->sbSizeY->value();
    n_cameras = ui->sbCameras->value();
    firstcamera = ui->ledCamerasStart->text().toFloat();
    lastcamera = ui->ledCameraFinish->text().toFloat();
    axis = ui->cobAxis->currentIndex();

    use_psf = ( ui->cbUsePSF->isChecked() ? 1 : 0);
    PSF_dist0 = ui->ledDist0->text().toFloat();
    PSF_dist1 = ui->ledDist1->text().toFloat();
    PSF_fwhm0 = ui->ledFWHM0->text().toFloat();
    PSF_fwhm1 = ui->ledFWHM1->text().toFloat();
    PSF_efficiency0 = ui->ledEff0->text().toFloat();
    PSF_efficiency1 = ui->ledEff1->text().toFloat();

    iterations = ui->sbIterations->value();
    subsets = (ui->cobMethod->currentIndex()==0 ? 1 : ui->sbOSEMsubsets->value());
    ui->sbOSEMsubsets->setValue(subsets);
}

void MainWindow::on_pbReconstruct_clicked()
{
    this->setEnabled(false);
    qApp->processEvents();

    //=================== Reconstruction ===================
    float *sinogram_data = ProjectionStack.data();
    unsigned int psf_size_x;
    unsigned int psf_size_y;    
    int use_attenuation = 0;

    //if (activity_data) free(activity_data);
    //activity_data = (float*) malloc(size_x*size_y*size_x*sizeof(float));
    delete [] activity_data;
    activity_data = new float[size_x*size_y*size_x];

    float *psf_data = 0;
    float *attenuation_data = 0;

    reportReconstructionSettings();

    int status;

    if (use_psf)
    {
        status = et_array_calculate_size_psf(&psf_size_x, &psf_size_y, PSF_fwhm0, PSF_efficiency0, PSF_dist0, PSF_fwhm1, PSF_efficiency1, PSF_dist1);
        if (status != 0)
        {
            qWarning() << "Rec_spect: Error creating PSF";
            exit(11);
        }

        //psf_data = (float*) malloc(psf_size_x*psf_size_y*size_x*sizeof(float));
        psf_data = new float[psf_size_x*psf_size_y*size_x];

        status = et_array_make_psf(psf_data, psf_size_x, psf_size_y, PSF_fwhm0, PSF_efficiency0, PSF_dist0, PSF_fwhm1, PSF_efficiency1, PSF_dist1, size_x);
        if (status != 0)
        {
            qWarning() << "Rec_spect: Error creating PSF";
            delete [] psf_data;
            exit(11);
        }
    }

    if (subsets<=1)
    {
        qDebug() << "Reconstruction using MLEM...";
        status = et_array_mlem(activity_data, size_x, size_y, sinogram_data, n_cameras, firstcamera, lastcamera, axis, iterations, use_attenuation, attenuation_data, use_psf, psf_data, psf_size_x, psf_size_y, BACKGROUND_ACTIVITY, BACKGROUND_ATTENUATION, EPSILON, gpu);
    }
    else
    {
        qDebug() << "Reconstruction using OSEM...";
        status = et_array_osem(activity_data, size_x, size_y, subsets, sinogram_data, n_cameras, firstcamera, lastcamera, axis, iterations, use_attenuation, attenuation_data, use_psf, psf_data, psf_size_x, psf_size_y, BACKGROUND_ACTIVITY, BACKGROUND_ATTENUATION, EPSILON, gpu);
    }

    //=================== Clear dynamic resources ===================
    //if (use_psf) free(psf_data);
    //if (use_attenuation) free(attenuation_data);
    delete [] psf_data;
    delete [] attenuation_data;

    //=================== Processing results ===================
    if (status!=0)
    {
        qDebug() << "Reconstruction failed!";
        exit(1);
    }
    fflush(stdout);
    qDebug() << "Reconstruction finished";

    delete h;
    h = new TH3F("h", "h",
                 size_x, 0,size_x,
                 size_y, 0,size_y,
                 size_x, 0,size_x);

    minAct = 1e10;
    maxAct = -1e10;
    for (unsigned int iz=0; iz<size_x; iz++)
        for (unsigned int iy=0; iy<size_y; iy++)
            for (unsigned int ix=0; ix<size_x; ix++)
            {
                float val = activity_data[ix + iy*size_x + iz*(size_x*size_y)];
                if (val == 0) continue;
                h->Fill( ix, iy, iz, val );
                if (val>maxAct) maxAct=val;
                if (val<minAct) minAct=val;
            }
    qDebug() << "Root histogram filled";

    if (minAct==0) minAct = (float)0.0001;
    ui->sbX->setMaximum(size_x-1);
    ui->hsSlice->setMaximum(size_x-1);

    fAlreadyShown = false;
    Root->CloseCanvas();
    on_pbShowSlice_clicked();

    this->setEnabled(true);
    ui->fReconstruct->setEnabled(true);

    qDebug() << "Done!";
}

void MainWindow::on_pbShowSlice_clicked()
{
    if (!h) return;

    int lim = ui->sbX->value();

    TString sel;
    int selCB = ui->comboBox->currentIndex();    
    switch (selCB)
    {
    case 0:
        sel = "xy";
        h->GetXaxis()->SetRange(0,size_x);
        h->GetYaxis()->SetRange(0,size_y);
        h->GetZaxis()->SetRange(lim,lim);
        break;
    case 1:
        sel = "xz";
        h->GetXaxis()->SetRange(0,size_x);
        h->GetYaxis()->SetRange(lim,lim);
        h->GetZaxis()->SetRange(0,size_x);
        break;
    case 2:
        sel = "yz";
        h->GetXaxis()->SetRange(lim,lim);
        h->GetYaxis()->SetRange(0,size_y);
        h->GetZaxis()->SetRange(0,size_x);
        break;
    }
    TH2F* pr = (TH2F*) h->Project3D(sel);
    if (!ui->cbSuppressZero->isChecked()) minAct = (float)(-0.001);
    pr->GetZaxis()->SetRangeUser(minAct, maxAct);

    pr->Draw(ui->leOpt->text().toLatin1());

    if (!fAlreadyShown)
    {       
        Root->SetRealAspectRatio(gPad->GetCanvas(), 2);
        fAlreadyShown = true;
    }
    gPad->GetCanvas()->Update();
}

void MainWindow::onWebMessageReceived(QString message)
{
    ui->pteStatus->appendPlainText("New package received over web socket");

    if (message.isEmpty())
      {
        ui->pteStatus->appendPlainText("We were pinged");
        return;
      }

    QJsonObject json = StringToJson(message);

    if (!json.isEmpty())
    {
        readConfigFromJson(json);
        //reportReconstructionSettings();
    }
    else
        postWarning("Unrecognized format or empty!");

    UpdateGui();
}

void MainWindow::reportReconstructionSettings()
{
    qDebug() << "=============== NiftyRec SPECT Parameters ===============";
    qDebug() << "First camera:             " << firstcamera;
    qDebug() << "Last camera:              " << lastcamera;
    qDebug() << "Rotation axis:            " << axis;
    //qDebug() << "Rotation radius [pixels]: " << radius;
    qDebug() << "Iterations:               " << iterations;
    qDebug() << "OSEM subsets:             " << subsets;
    qDebug() << "Use GPU:                  " << gpu;
    qDebug() << "Sinogram size X [pixels]: " << size_x;
    qDebug() << "Sinogram size Y [pixels]: " << size_y;
    qDebug() << "Number of projections:    " << n_cameras;
    qDebug() << "Activity size X [pixels]: " << size_x;
    qDebug() << "Activity size Y [pixels]: " << size_y;
    qDebug() << "Activity size Z [pixels]: " << size_x;
    qDebug() << "=========================================================";
}

void MainWindow::readConfigFromJson(QJsonObject &json)
{
    parseJson(json, "size_x", size_x);
    parseJson(json, "size_y", size_y);
    parseJson(json, "n_cameras", n_cameras);
    parseJson(json, "firstcamera", firstcamera);
    parseJson(json, "lastcamera", lastcamera);
    parseJson(json, "axis", axis);
    if (axis<0 || axis>1)
    {
        postWarning("Attempt to set invald axis. Setting to 0");
        axis = 0;
    }
    //parseJson(json, "radius", radius);
    parseJson(json, "iterations", iterations);
    parseJson(json, "gpu", gpu);
    if (gpu<0 || gpu>1)
    {
        postWarning("Attempt to set invald gpu flag. Setting to false");
        gpu = 0;
    }
    parseJson(json, "subsets", subsets);
    if (subsets<1)
    {
        postWarning("Negative or zero subsets converted to 1");
        subsets = 1;
    }

    parseJson(json, "psf", use_psf);
    if (use_psf<0 || use_psf>1)
    {
        postWarning("Attempt to set invald psu flag. Setting to false");
        use_psf = 0;
    }
    parseJson(json, "psf_fwhm0", PSF_fwhm0);
    parseJson(json, "psf_fwhm1", PSF_fwhm1);
    parseJson(json, "pdf_dist0", PSF_dist0);
    parseJson(json, "psf_dist1", PSF_dist1);
    parseJson(json, "psf_efficiency0", PSF_efficiency0);
    parseJson(json, "psf_efficiency1", PSF_efficiency1);

    if (json.contains("projections"))
      {
        fDataLoadedFromFiles = false;
        ProjectionStack.clear();

        QJsonArray ar = json["projections"].toArray();
        //qDebug() << "array size:"<<ar.size();
        if (ar.size() == size_x*size_y*n_cameras)
          {
            for (int i=0; i<ar.size(); i++)
                ProjectionStack << ar[i].toDouble();
          }
        else
          postWarning("Wrong projections data size, ignoring");
      }

    if (json.contains("load"))
    {
        QString ProjectionFileNameTemplate;
        parseJson(json, "load", ProjectionFileNameTemplate);
        bool fOK = loadProjections(ProjectionFileNameTemplate);
        if (fOK)
        {
            const int size = ProjectionStack.size();
            if (size != size_x*size_y*n_cameras)
            {
                postWarning("Wrong projections data size!");
                qDebug() << "Size:"<<size<<"expected:"<<size_x*size_y*n_cameras;
            }
        }
        else
        {
            postWarning("Failed to load projections using the template");
            ProjectionStack.clear();
        }
    }
}

void MainWindow::writeConfigToJson(QJsonObject &json)
{
    json["size_x"] = int(size_x);
    json["size_y"] = int(size_y);
    json["n_cameras"] = n_cameras;
    json["firstcamera"] = firstcamera;
    json["lastcamera"] = lastcamera;
    json["axis"] = int(axis);
    //json["radius"] = radius;
    json["iterations"] = iterations;
    json["gpu"] = gpu;
    json["subsets"] = subsets;

    json["psf"] = use_psf;
    json["psf_fwhm0"] = PSF_fwhm0;
    json["psf_fwhm1"] = PSF_fwhm1;
    json["pdf_dist0"] = PSF_dist0;
    json["psf_dist1"] = PSF_dist1;
    json["psf_efficiency0"] = PSF_efficiency0;
    json["psf_efficiency1"] = PSF_efficiency1;

    if (fDataLoadedFromFiles)
        json["load"] = ProjectionDataFilesTemplate;
    else
    {
        QJsonArray arr;
        for (float val :  ProjectionStack) arr << val;
        json["projections"] = arr;
    }
}

bool MainWindow::loadProjections(QString fileNameTemplate)
{
    ProjectionDataFilesTemplate = fileNameTemplate;
    QStringList sl = ProjectionDataFilesTemplate.split("_");
    QString ext = sl.last();

    QString base = ProjectionDataFilesTemplate;
    base.chop(ext.size());

    fDataLoadedFromFiles = true;
    ProjectionStack.clear();

    QVector<float> c1, c2, c3;
    for (int i=0; i<n_cameras; i++)
    {
        if (!LoadFloatVectorsFromFile(base + QString::number(i) + ext, &c1, &c2, &c3))
        {
            ProjectionStack.clear();
            return false;
        }
        ProjectionStack << c3;
    }
    return true;
}

void MainWindow::postWarning(QString warning)
{
    qWarning() << warning;
    QString ww = "<font color=\"red\">"+warning+"</font>";
    ui->pteStatus->appendHtml(ww);
}

void MainWindow::on_actionSave_as_nifty_file_triggered()
{
  QString fileName = QFileDialog::getSaveFileName(this, "Save reconstruction as nifty file", "", "Nifty files (*.nii)");
  if (fileName.isEmpty()) return;
  fileName.replace("/", "\\");
  std::string output_filename(fileName.toLatin1());
  qDebug() << output_filename.c_str();

  int dim[8];
  dim[0]=3;
  dim[1]=size_x;
  dim[2]=size_y;
  dim[3]=size_x;
  dim[4]=1; dim[5]=1; dim[6]=1; dim[7]=1;

  nifti_image *activityImage = 0;
  activityImage = nifti_make_new_nim(dim, NIFTI_TYPE_FLOAT32, false);
  activityImage->data = static_cast<void *>(activity_data);

  nifti_set_filenames(activityImage, output_filename.c_str(), 0, 0);

  nifti_image_write(activityImage);
  nifti_image_free(activityImage);
}

bool MainWindow::prepare3Ddraw()
{
  if (!h) return false;

  h->GetXaxis()->SetRange(0,size_x);
  h->GetYaxis()->SetRange(0,size_x);
  h->GetZaxis()->SetRange(0,size_x);

  gStyle->SetCanvasPreferGL(true);
  Root->CloseCanvas();
  fAlreadyShown = false;
  return true;
}

void MainWindow::on_actionPlot_3D_triggered()
{
  if (!prepare3Ddraw()) return;
  h->Draw("GLISO");
}

void MainWindow::on_actionPlot_3D_Boxes_triggered()
{
    if (!prepare3Ddraw()) return;
    h->Draw("glbox");
}

void MainWindow::on_actionPlot_3D_tansparent_triggered()
{
    if (!prepare3Ddraw()) return;
    h->Draw("GLCOL"); //invisible :(

    //gPad->GetCanvas()->HandleInput(kMouseMotion, 300,300);
    //gPad->GetCanvas()->HandleInput(kKeyPress, 'c', 0);
}

void MainWindow::on_cbListenWebSocket_clicked()
{
    WebServer->PauseAccepting(!ui->cbListenWebSocket->isChecked());
}

void MainWindow::on_cobMethod_currentIndexChanged(int index)
{
    ui->sbOSEMsubsets->setEnabled(index==1);
}

void MainWindow::on_pbLoadManifest_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load manifest file", "", "Manifest files (*.mani);;All files (*)");
    if (fileName.isEmpty()) return;
    qDebug() << fileName;

    QJsonObject json;
    bool fOK = LoadJsonFromFile(json, fileName);
    if (!fOK)
    {
        postWarning("Failed to load manifest file!");
        return;
    }

    readConfigFromJson(json);
    UpdateGui();
}

void MainWindow::on_pbLoadProjections_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load projections.\nThe file name should end with _int, where int is the camera number", "", "Data files (*.dat *.txt);;All files(*)");
    if (fileName.isEmpty()) return;

    QStringList sl = fileName.split(".");
    QString ext =  (sl.size()>1 ? sl.last() : "");
    QString tmpl = (sl.size()>1 ? sl.first() : fileName);
    if (tmpl.contains("_"))
    {
        sl = fileName.split("_");
        bool fOK;
        QString num = sl.last();
        num.toInt(&fOK);
        if (fOK)
        {
            tmpl.chop(num.count());
            tmpl += ext;
            qDebug() << "Template:"<<tmpl;
            fOK = loadProjections(tmpl);
            UpdateGui();
            if (fOK)
            {
                ui->pteStatus->appendPlainText("Projections loaded!");
                return;
            }
            else
            {
                postWarning("failed to load all projections using template.");
                return;
            }
        }
    }

    postWarning("Load projection failed: template must be fileName_[.ext]");
}

void MainWindow::on_pbSaveManifest_clicked()
{
    if (!ui->pbReconstruct->isEnabled())
    {
        int ret = QMessageBox::warning(this, "", "Not reconstructable data!\nContinue to save manifest?",
                                      QMessageBox::Save | QMessageBox::Cancel,  QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel) return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save manifest file.\nIf projections were loaded, only link will be provided.\nOtherwise, array data will be saved", "", "Manifest files (*.mani);;All files(*)");
    if (fileName.isEmpty()) return;

    QJsonObject json;
    writeConfigToJson(json);
    bool fOK = SaveJsonToFile(json, fileName);
    if (!fOK) postWarning("Failed to save manifest to file!");
    else ui->pteStatus->appendPlainText("Manifest saved.");
}

void MainWindow::on_cobPalette_currentIndexChanged(int index)
{
    switch (index)
    {
      case 0: gStyle->SetPalette(1); break;
      case 1: gStyle->SetPalette(51); break;
      case 2: gStyle->SetPalette(52); break;
      case 3: gStyle->SetPalette(53); break;
      case 4: gStyle->SetPalette(54); break;
      case 5: gStyle->SetPalette(55); break;
      case 6: gStyle->SetPalette(56); break;
    }
    gPad->Update();
}
