#include "qemu_monitor.h"
#include "../log/log.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define MAX_RETRY_TIMES 2  // 连接失败重试次数
#define RETRY_INTERVAL_SEC 1  // 连接失败重试间隔
#define MAX_RECV_WAIT_TIME 5  // 接收函数等待时间

// 构造函数
QemuMonitor::QemuMonitor(std::string socketPath) :unixSocketPath(socketPath) {
    // std::cout << "Create a QMP socket at " << socketPath << std::endl;
    LOG_INFO("Create a QMP socket at %s", socketPath.c_str());
    qemuMonitorOpenUnixSocket();
};

// 向 Unix Socket 发送信息
int sendToUnixSocket(int socketFd, const std::string& message) {
    ssize_t bytesSent = send(socketFd, message.c_str(), message.length(), 0);
    if ( bytesSent < 0 ) {
        // std::cerr << "Failed to send message to socket" << std::endl;
        LOG_ERROR("Failed to send message to socket");
        return -1;
    }
    // std::cout << "send msg: " << message.c_str() << std::endl;
    LOG_INFO("send msg: %s", message.c_str());
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
    if ( bytesRead < 0 ) {
        // std::cerr << "Failed to receive message from socket" << std::endl;
        LOG_ERROR("Failed to receive message from socket");
        return "";
    }

    buffer[bytesRead] = '\0'; // 确保字符串以 '\0' 结尾
    receivedMessage = buffer;
    // std::cout << "recv msg:" << receivedMessage << std::endl;
    // QMP协议返回的消息是JSON格式的字符串，通常最后会有一个换行符
    // 这里不进行额外处理，以保留原始格式
    LOG_INFO("recv msg: %s", receivedMessage.c_str());

    return receivedMessage;
}

// QMP协议握手
int QemuMonitor::qemuMonitorNegotiation() {
    // std::cout << "Negotiating..." << std::endl;
    LOG_INFO("Negotiating...");
    std::string negotiationCMD = "{ \"execute\":\"qmp_capabilities\"}";
    std::string reply;
    if ( qemuMonitorSendMessage(negotiationCMD, reply) < 0 ) {
        // std::cerr << "Failed to Negotiation! Msg returned: " << reply << std::endl;
        LOG_ERROR("Failed to Negotiation! Msg returned: %s", reply.c_str());
        return -1;
    }
    // std::cout << "Negotiation Result:" << reply << std::endl;
    return 0;
}


/**
 * 连接UNIX SOCKET
 */
int QemuMonitor::qemuMonitorOpenUnixSocket() {
    if ( this->unixSocketPath.empty() ) {
        // std::cerr << "empty UnixSocketPath" << std::endl;
        LOG_ERROR("empty UnixSocketPath");
        return -1;
    }
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if ( sockfd == -1 )
    {
        // std::cerr << "Failed to create socket" << std::endl;
        LOG_ERROR("Failed to create socket");
        return sockfd;
    }
    // std::cout << "client socket created! sockfd: " << sockfd << std::endl;
    LOG_INFO("client socket created! sockfd: %d", sockfd);
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
    while ( count < MAX_RETRY_TIMES ) {
        conn = connect(sockfd, ( struct sockaddr* )&serverAddr, sizeof(struct sockaddr_un));
        if ( conn < 0 ) {
            // std::cout << count + 1 << " times connect failed, sleep and retry..." << std::endl;
            LOG_INFO("%d times connect failed, sleep and retry...", count + 1);
            count++;
            sleep(RETRY_INTERVAL_SEC);
            continue;
        }
        else {
            // std::cout << "Connected to server!" << std::endl;
            LOG_INFO("Connected to server!");
            break;
        }
    }

    if ( conn < 0 ) {
        // std::cerr << "Failed to connect to server, socket closed!" << std::endl;
        LOG_ERROR("Failed to connect to server, socket closed!");
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

int QemuMonitor::qemuMonitorCloseUnixSocket() {
    close(this->unixSocketFd);
    // std::cout << "Connect Closed!" << std::endl;
    LOG_INFO("Connect Closed!");
    this->unixSocketFd = -1;
    this->open = false;
    return 0;
}

int QemuMonitor::qemuMonitorSendMessage(const std::string cmd, std::string& reply) {

    // todo 判断socket连接状态
    if ( sendToUnixSocket(this->unixSocketFd, cmd) < 0 ) {
        return -1;
    }
    // sleep(1);
    // 接收服务器的回复
    reply = receiveFromUnixSocket(this->unixSocketFd);
    if ( reply.empty() ) {
        return -1;
    }
    return 0;
}

QemuMonitor::~QemuMonitor() {
    qemuMonitorCloseUnixSocket();
}


