#include "arootmanager.h"

#include <QDebug>
#include <QTimer>

#include "TApplication.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TMath.h"

ARootManager::ARootManager()
{
    //create ROOT application
    int rootargc=1;
    char *rootargv[] = {(char*)"qqq"};
    RootApp = new TApplication("My ROOT", &rootargc, rootargv);
    qDebug() << "->ROOT app is running";
    QTimer* RootUpdateTimer = new QTimer(this);
    RootUpdateTimer->setInterval(100);
    connect(RootUpdateTimer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    RootUpdateTimer->start();
    qDebug()<<"->Timer to refresh Root events started";
}

ARootManager::~ARootManager()
{
    delete RootApp;
}

void ARootManager::timerTimeout()
{
    gSystem->ProcessEvents();
}

bool ARootManager::SetRealAspectRatio(TCanvas * const c, int axis)
{
    if (!c) return false;

    c->Update();

    //Get how many pixels are occupied by the canvas
    const Int_t npx = c->GetWw();
    const Int_t npy = c->GetWh();

    //Get x-y coordinates at the edges of the canvas (extrapolating outside the axes, NOT at the edges of the histogram)
    const Double_t x1 = c->GetX1();
    const Double_t y1 = c->GetY1();
    const Double_t x2 = c->GetX2();
    const Double_t y2 = c->GetY2();

    //Get the length of extrapolated x and y axes
    const Double_t xlength2 = x2 - x1;
    const Double_t ylength2 = y2 - y1;
    const Double_t ratio2 = xlength2/ylength2;

    //Get now number of pixels including window borders
    const Int_t bnpx = c->GetWindowWidth();
    const Int_t bnpy = c->GetWindowHeight();

    if(axis==1)
    {
        c->SetCanvasSize(TMath::Nint(npy*ratio2), npy);
        c->SetWindowSize((bnpx-npx)+TMath::Nint(npy*ratio2), bnpy);
    }
    else if(axis==2)
    {
        c->SetCanvasSize(npx, TMath::Nint(npx/ratio2));
        c->SetWindowSize(bnpx, (bnpy-npy)+TMath::Nint(npx/ratio2));
    }
    else
    {
        qDebug() << "Error in SetRealAspectRatio: axis value " << axis << " is neither 1 (resize along x-axis) nor 2 (resize along y-axis).";
        return false;
    }

    c->Update();
    return true;
    // References:
    // https://root.cern.ch/root/roottalk/roottalk01/3676.html
    // https://root.cern.ch/phpBB3/viewtopic.php?t=4440
}

void ARootManager::CloseCanvas()
{
  if (gPad)
    if (gPad->GetCanvas())
      gPad->GetCanvas()->Close();
}
