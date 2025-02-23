#include "monitor.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define MAX_RETRY_TIMES 5  // 连接失败重试次数
#define RETRY_INTERVAL_SEC 1  // 连接失败重试间隔
#define MAX_RECV_WAIT_TIME 5  // 接收函数等待时间

// 向 Unix Socket 发送信息
int sendToUnixSocket(int socketFd, const std::string &message) {
    ssize_t bytesSent = send(socketFd, message.c_str(), message.length(), 0);
    if (bytesSent < 0) {
        std::cerr << "Failed to send message to socket" << std::endl;
        return -1;
    }
    std::cout << "send msg: " << message.c_str() << std::endl;
    return 0; // 发送成功
}
// 从 Unix Socket 接收信息
std::string receiveFromUnixSocket(int socketFd) {
    char buffer[4096]; // 缓冲区大小可根据需要调整
    std::string receivedMessage = "";

    struct timeval timeout;
    timeout.tv_sec = 5;  // 设置超时时间为5秒
    timeout.tv_usec = 0;
    setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ssize_t bytesRead = recv(socketFd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead < 0) {
        std::cerr << "Failed to receive message from socket" << std::endl;
        return "";
    }

    buffer[bytesRead] = '\0'; // 确保字符串以 '\0' 结尾
    receivedMessage = buffer;
    std::cout << "recv msg:" << receivedMessage << std::endl;

    return receivedMessage;
}

// QMP协议握手
int Monitor::qemuMonitorNegotiation() {
    std::cout << "Negotiating..." << std::endl;
    std::string negotiationCMD = "{ \"execute\":\"qmp_capabilities\"}";
    std::string reply;
    if (qemuMonitorSendMessage(negotiationCMD, reply) < 0) {
        std::cerr << "Failed to Negotiation! Msg returned: " << reply << std::endl;
        return -1; 
    }
    // std::cout << "Negotiation Result:" << reply << std::endl;
    return 0;
}


/**
 * 连接UNIX SOCKET
 */
int Monitor::qemuMonitorOpenUnixSocket() {
    if (this->unixSocketPath.empty()) {
        std::cerr << "empty UnixSocketPath" << std::endl;
        return -1;
    }
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return sockfd;
    } 
    std::cout << "client socket created! sockfd: " << sockfd << std::endl;
    // 设置接收数据的超时时间
    struct timeval timeout;
    timeout.tv_sec = MAX_RECV_WAIT_TIME;  // 超时时间为 5 秒
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // 连接到服务器
    struct sockaddr_un serverAddr;
    memset(&serverAddr, 0, sizeof(struct sockaddr_un));
    serverAddr.sun_family = AF_UNIX;
    strncpy(serverAddr.sun_path, this->unixSocketPath.c_str(), sizeof(serverAddr.sun_path) - 1);
    
    
    int count = 0;  // 尝试连接次数
    int conn = -1;  // 连接结果
    while (count < MAX_RETRY_TIMES) {
        conn = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_un));
        if (conn < 0) {
            std::cout << count + 1 << " times connect failed, sleep and retry..." << std::endl;
            count++;
            sleep(RETRY_INTERVAL_SEC);
            continue;
        } else {
            std::cout << "Connected to server!" << std::endl;
            break;
        }
    }

    if (conn < 0) {
        std::cerr << "Failed to connect to server, socket closed!" << std::endl;
        close(sockfd);
        return -1;
    }

    // if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_un)) == -1)
    // {
    //     std::cerr << "Failed to connect to server" << std::endl;
    //     close(sockfd);
    //     return -1;
    // }
    // std::cout << "Connected to server!" << std::endl;

    
    receiveFromUnixSocket(sockfd);  // 接收连接时的hello消息
    this->open = true;
    this->unixSocketFd = sockfd;

    qemuMonitorNegotiation();  // 连接时进行协议握手

    return sockfd;
}

int Monitor::qemuMonitorCloseUnixSocket() {
    close(this->unixSocketFd);
    std::cout << "Connect Closed!" << std::endl;
    this->unixSocketFd = -1;
    this->open = false;
}

int Monitor::qemuMonitorSendMessage(const std::string cmd, std::string& reply) {

    // todo 判断socket连接状态
    if (sendToUnixSocket(this->unixSocketFd, cmd) < 0) {
        return -1;
    }
    // sleep(1);
    // 接收服务器的回复
    reply = receiveFromUnixSocket(this->unixSocketFd);
    if (reply.empty()) {
        return -1;
    }
    return 0;
}

Monitor::~Monitor() {
    qemuMonitorCloseUnixSocket();
}


