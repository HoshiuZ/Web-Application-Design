#include "Client.h"

#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 用户ID
    bool ok;
    QString clientId = QInputDialog::getText(nullptr, "设置ID", "请输入聊天室内你的ID：", QLineEdit::Normal, "", &ok);

    if(!ok || clientId.trimmed().isEmpty()) {
        QMessageBox::warning(nullptr, "提示", "你只有输入ID才能进入聊天室。");
        return 0;
    }

    Client w;
    w.setClientId(clientId);
    w.show();
    return a.exec();
}
