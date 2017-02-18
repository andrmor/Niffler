#ifndef AROOTMANAGER_H
#define AROOTMANAGER_H

#include  <QObject>

class TApplication;
class TCanvas;

class ARootManager : public QObject
{
    Q_OBJECT

public:
    ARootManager();
    ~ARootManager();

    TApplication* RootApp;

    bool SetRealAspectRatio(TCanvas * const c, int axis = 1);
    void CloseCanvas();

private slots:
    void timerTimeout();
};

#endif // AROOTMANAGER_H
