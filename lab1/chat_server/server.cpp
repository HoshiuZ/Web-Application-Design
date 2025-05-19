#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <vector>
#include <map>
#include <signal.h>
#include <sys/stat.h>

#define PORT 12345
#define BUFFER_SIZE 1024
#define LOG_FILE "/tmp/chat_server.log"


void daemonize() {
    pid_t pid = fork();       // 第一次 fork
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);  // 父进程退出

    if (setsid() < 0) exit(EXIT_FAILURE); // 创建新会话

    signal(SIGCHLD, SIG_IGN); // 忽略子进程信号
    signal(SIGHUP, SIG_IGN);  // 忽略挂断信号

    pid = fork();             // 第二次 fork
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void log_message(const std::string& msg) {
    std::ofstream log(LOG_FILE, std::ios::app);
    log << msg << std::endl;
}

int main() {
    std::ofstream log(LOG_FILE, std::ios::trunc);  // 清空旧日志
    log.close();

    daemonize();  // 启动守护进程
    log_message("守护进程启动...");  // 将启动守护进程的消息写入日志

    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);  // 创建 TCP socket
    if (server_fd < 0) {
        log_message("Socket 创建失败");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  // 设置端口可复用

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {  // 绑定端口
        log_message("绑定失败");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {  // 监听端口
        log_message("监听失败");
        exit(EXIT_FAILURE);
    }

    std::vector<int> clients;
    std::map<int, std::string> clientIdMap;

    fd_set readfds;
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for (int client : clients) {
            FD_SET(client, &readfds);
            if (client > max_sd) max_sd = client;
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            log_message("select 错误");
            continue;
        }

        // 新客户端连接
        if (FD_ISSET(server_fd, &readfds)) {  // 处理新客户端连接
            client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
            if (client_fd >= 0) {
                clients.push_back(client_fd);
                log_message("新客户端连接: fd=" + std::to_string(client_fd));
                // 等待客户端发送ID，放在后面循环处理
            }
        }

        // 处理客户端消息
        for (auto it = clients.begin(); it != clients.end();) {
            int sd = *it;
            if (FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, BUFFER_SIZE);  //接收信息
                if (valread <= 0) {
                    // 客户端断开
                    std::string userId = clientIdMap.count(sd) ? clientIdMap[sd] : std::to_string(sd);
                    log_message("客户端断开: " + userId + " (fd=" + std::to_string(sd) + ")");
                    close(sd);

                    // 广播用户退出消息（如果已知ID）
                    if (clientIdMap.count(sd)) {
                        std::string leaveMsg = userId + " 退出了聊天室";
                        for (int client : clients) {
                            if (client != sd) {
                                send(client, leaveMsg.c_str(), leaveMsg.size(), 0);
                            }
                        }
                        clientIdMap.erase(sd);
                    }

                    it = clients.erase(it);
                    continue;
                }

                buffer[valread] = '\0';
                std::string recvMsg(buffer);

                if (clientIdMap.count(sd) == 0) {  // 接收到用户 ID
                    // 每个客户端第一次向服务端发送的信息都是 ID 
                    clientIdMap[sd] = recvMsg;
                    log_message("用户 " + recvMsg + " 已连接 (fd=" + std::to_string(sd) + ")");
                    std::string enterMsg = recvMsg + " 进入了聊天室";

                    // 给其他客户端广播进入消息
                    for (int client : clients) {
                        if (client != sd) {
                            send(client, enterMsg.c_str(), enterMsg.size(), 0);
                        }
                    }
                } else {
                    // 普通聊天消息，广播给其他客户端
                    std::string msg = clientIdMap[sd] + ": " + recvMsg;
                    log_message(msg);

                    for (int client : clients) {
                        if (client != sd) {
                            send(client, msg.c_str(), msg.size(), 0);
                        }
                    }
                }
            }
            ++it;
        }
    }

    return 0;
}
