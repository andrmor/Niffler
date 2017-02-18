#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ARootManager;
class TH3F;
class TCanvas;
class MyWebServer;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pbReconstruct_clicked();
    void on_pbShowSlice_clicked();
    void on_actionSave_as_nifty_file_triggered();
    void on_actionPlot_3D_triggered();
    void on_pbUpdateSettings_clicked();
    void on_cbListenWebSocket_clicked();
    void on_cobMethod_currentIndexChanged(int index);
    void on_pbLoadManifest_clicked();
    void on_pbLoadProjections_clicked();
    void on_pbSaveManifest_clicked();
    void on_cobPalette_currentIndexChanged(int index);
    void on_actionPlot_3D_Boxes_triggered();
    void on_actionPlot_3D_tansparent_triggered();

public slots:
    void onWebMessageReceived(QString message);

private:
    Ui::MainWindow *ui;
    ARootManager* Root;

    MyWebServer *WebServer;

    unsigned int size_x;
    unsigned int size_y;
    int n_cameras;
    float firstcamera;
    float lastcamera;
    unsigned int axis;
    //float radius;
    int iterations;
    int gpu;
    int subsets;
    int use_psf;
    float PSF_fwhm0;
    float PSF_fwhm1;
    float PSF_dist0;
    float PSF_dist1;
    float PSF_efficiency0;
    float PSF_efficiency1;

    float *activity_data;

    bool fDataLoadedFromFiles;
    QString ProjectionDataFilesTemplate;
    QVector<float> ProjectionStack;

    TH3F* h;
    float minAct, maxAct;
    bool fAlreadyShown;

    void UpdateGui();

    void reportReconstructionSettings();
    void readConfigFromJson(QJsonObject &json);
    void writeConfigToJson(QJsonObject &json);
    bool loadProjections(QString fileNameTemplate);
    void postWarning(QString warning);
    bool prepare3Ddraw();
};

#endif // MAINWINDOW_H
