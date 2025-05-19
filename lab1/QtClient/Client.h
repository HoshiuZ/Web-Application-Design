#ifndef CLIENT_H
#define CLIENT_H

#include <QWidget>
#include <QTcpSocket>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>

class Client : public QWidget
{
    Q_OBJECT

public:
    Client(QWidget *parent = nullptr);
    void setClientId(const QString& id);

private slots:
    void onConnectClicked();
    void onSendClicked();
    void onExitClicked();
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);
    void onConnected();


private:
    QTcpSocket   *socket;
    QLineEdit    *inputLine;
    QTextEdit    *chatArea;
    QPushButton  *connectButton;
    QPushButton  *sendButton;
    QPushButton  *exitButton;
    QString clientId;

    QString serverIp   = "139.155.108.193";
    quint16 serverPort = 12345;
};

#endif // CLIENT_H
