#include "Client.h"
#include <QMessageBox>
#include <QInputDialog>

Client::Client(QWidget *parent)
    : QWidget(parent)
{
    // 创建控件
    chatArea      = new QTextEdit(this);
    chatArea->setReadOnly(true);

    inputLine     = new QLineEdit(this);
    connectButton = new QPushButton("进入聊天室", this);
    sendButton    = new QPushButton("发送消息", this);
    exitButton    = new QPushButton("退出聊天室", this);
    sendButton->setEnabled(false);
    exitButton->setEnabled(false);

    // 布局
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(chatArea);
    layout->addWidget(inputLine);
    layout->addWidget(connectButton);
    layout->addWidget(sendButton);
    layout->addWidget(exitButton);
    setLayout(layout);

    setWindowTitle("聊天室");
    resize(400, 500);

    // 初始化 socket
    socket = new QTcpSocket(this);

    // 连接信号与槽
    connect(connectButton, &QPushButton::clicked, this, &Client::onConnectClicked);
    connect(sendButton, &QPushButton::clicked, this, &Client::onSendClicked);
    connect(exitButton, &QPushButton::clicked, this, &Client::onExitClicked);
    connect(inputLine, &QLineEdit::returnPressed, this, &Client::onSendClicked);
    connect(socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &Client::onError);
    connect(socket, &QTcpSocket::connected, this, &Client::onConnected);
}

void Client::onConnectClicked()
{
    chatArea->append("正在进入聊天室.....");
    socket->connectToHost(serverIp, serverPort);
    connectButton->setEnabled(false);
}

void Client::onSendClicked()
{
    QString msg = inputLine->text().trimmed();
    if (msg.isEmpty())
        return;

    QString fullMessage = QString("%1").arg(msg);
    socket->write(fullMessage.toUtf8());

    chatArea->append(QString("<span style=\"color:blue;\"><b>我</b>: %1</span>").arg(msg));

    inputLine->clear();
}

void Client::onExitClicked()
{
    int ret = QMessageBox::question(this, "退出确认", "确定要退出聊天室吗？", QMessageBox::Yes | QMessageBox::No);
    if(ret == QMessageBox::Yes) {
        if(socket->state() == QAbstractSocket::ConnectedState) {
            QString leaveMsg = QString("%1 退出了聊天室。").arg(clientId);
            socket->write(leaveMsg.toUtf8());
            socket->waitForBytesWritten(100);
            socket->disconnectFromHost();
        }
        close();
    }
}

void Client::onReadyRead()
{
    QByteArray data = socket->readAll();
    QString msg = QString::fromUtf8(data);

    chatArea->append(QString("<span style=\"color:green;\">%1</span>").arg(msg));

    if (!sendButton->isEnabled())
        sendButton->setEnabled(true);
}

void Client::onDisconnected()
{
    chatArea->append("<span style=\"color:red;\">已退出聊天室</span>");
    connectButton->setEnabled(true);
    sendButton->setEnabled(false);
    exitButton->setEnabled(false);
}

void Client::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QMessageBox::critical(this, "网络错误",
                          socket->errorString());
    connectButton->setEnabled(true);
    sendButton->setEnabled(false);
    exitButton->setEnabled(false);
}

void Client::onConnected()
{
    chatArea->append("<span style=\"color:green;\">已进入聊天室</span>");
    socket->write(clientId.toUtf8());
    sendButton->setEnabled(true);
    exitButton->setEnabled(true);
}

void Client::setClientId(const QString& id)
{
    clientId = id;
}
