#ifndef MYWEBSERVER_H
#define MYWEBSERVER_H

#include <QObject>
#include <QList>
#include <QByteArray>

class QWebSocketServer;
class QWebSocket;

class MyWebServer : public QObject
{
    Q_OBJECT
public:
    explicit MyWebServer(quint16 port, bool debug = false, QObject *parent = Q_NULLPTR);
    ~MyWebServer();

    QString GetUrl();    
    void PauseAccepting(bool flag);

signals:
    void closed();
    void NotifyMessageReceived(QString message);

private slots:
    void onNewConnection();
    void processTextMessage(QString message);
    void processBinaryMessage(QByteArray message);
    void socketDisconnected();

private:
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    bool m_debug;
};

#endif // MYWEBSERVER_H
