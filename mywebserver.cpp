#include "mywebserver.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QDebug>

MyWebServer::MyWebServer(quint16 port, bool debug, QObject *parent) :
    QObject(parent),
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Echo Server"),
                                            QWebSocketServer::NonSecureMode, this)),
    m_clients(),
    m_debug(debug)
{
    if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
        if (m_debug)
            qDebug() << "Echoserver listening on port" << port;
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &MyWebServer::onNewConnection);
        connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &MyWebServer::closed);
    }

    qDebug() << "Listening on addresses:"<<m_pWebSocketServer->serverAddress();
    qDebug() << "Port:"<<m_pWebSocketServer->serverPort();
    qDebug() << "Url:" << m_pWebSocketServer->serverUrl();
}

MyWebServer::~MyWebServer()
{
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

QString MyWebServer::GetUrl()
{
  return m_pWebSocketServer->serverUrl().toString();
}


void MyWebServer::onNewConnection()
{
    if (m_debug)
        qDebug() << "New connection";
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &MyWebServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &MyWebServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &MyWebServer::socketDisconnected);

    m_clients << pSocket;
}

void MyWebServer::processTextMessage(QString message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (m_debug)
        qDebug() << "Server - message received:" << message;

    emit NotifyMessageReceived(message);

    if (pClient) {
        pClient->sendTextMessage("Nifty echoes: "+message);
    }
}

void MyWebServer::processBinaryMessage(QByteArray message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (m_debug)
        qDebug() << "Binary Message received:" << message;
    if (pClient) {
        pClient->sendBinaryMessage(message);
    }
}

void MyWebServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (m_debug)
        qDebug() << "socketDisconnected:" << pClient;
    if (pClient) {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

void MyWebServer::PauseAccepting(bool flag)
{
  if (flag) m_pWebSocketServer->pauseAccepting();
  else m_pWebSocketServer->resumeAccepting();
}
