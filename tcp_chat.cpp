#include "tcp_chat.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace {

constexpr const char* kClrSys = "\033[1;34m";
constexpr const char* kClrMe = "\033[1;32m";
constexpr const char* kClrErr = "\033[1;31m";
constexpr const char* kClrReset = "\033[0m";

std::mutex g_io_mtx;

std::string CurrentTime() {
    std::time_t now = std::time(nullptr);
    std::tm local_tm;
    localtime_r(&now, &local_tm);
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << local_tm.tm_hour << ":"
        << std::setfill('0') << std::setw(2) << local_tm.tm_min << ":"
        << std::setfill('0') << std::setw(2) << local_tm.tm_sec;
    return oss.str();
}

void PrintPrompt(const std::string& self_label) {
    std::lock_guard<std::mutex> lock(g_io_mtx);
    std::cout << kClrMe << self_label << ": " << kClrReset << std::flush;
}

void PrintSystem(const std::string& text) {
    std::lock_guard<std::mutex> lock(g_io_mtx);
    std::cout << "\r" << kClrSys << "[" << CurrentTime() << "] [系统] " << text
              << kClrReset << std::endl;
}

void PrintError(const std::string& text) {
    std::lock_guard<std::mutex> lock(g_io_mtx);
    std::cout << "\r" << kClrErr << "[" << CurrentTime() << "] [错误] " << text
              << kClrReset << std::endl;
}

void PrintPeerMessage(const std::string& peer, const std::string& text) {
    std::lock_guard<std::mutex> lock(g_io_mtx);
    std::cout << "\r" << "[" << CurrentTime() << "] " << peer << ": " << text << std::endl;
}

bool SendAll(int fd, const void* buf, size_t len) {
    const char* p = static_cast<const char*>(buf);
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool RecvAll(int fd, void* buf, size_t len) {
    char* p = static_cast<char*>(buf);
    size_t recvd = 0;
    while (recvd < len) {
        ssize_t n = recv(fd, p + recvd, len - recvd, 0);
        if (n <= 0) {
            return false;
        }
        recvd += static_cast<size_t>(n);
    }
    return true;
}

}  // namespace

TcpChat::TcpChat(DesCipher cipher) : cipher_(std::move(cipher)), running_(true) {}

void TcpChat::RunAsServer(unsigned short port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        throw std::runtime_error("socket create failed");
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(listen_fd);
        throw std::runtime_error("bind failed");
    }

    if (listen(listen_fd, 1) < 0) {
        close(listen_fd);
        throw std::runtime_error("listen failed");
    }

    PrintSystem("监听中... 端口 " + std::to_string(port));

    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int conn_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    close(listen_fd);

    if (conn_fd < 0) {
        throw std::runtime_error("accept failed");
    }

    char ipbuf[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &client_addr.sin_addr, ipbuf, sizeof(ipbuf));
    PrintSystem(std::string("收到连接: ") + ipbuf + ":" +
                std::to_string(ntohs(client_addr.sin_port)) +
                " (socket=" + std::to_string(conn_fd) + ")");
    PrintSystem("开始聊天，输入 quit 结束连接。");

    const std::string peer_label = std::string(ipbuf) + ":" +
                                   std::to_string(ntohs(client_addr.sin_port));
    ChatLoop(conn_fd, "Server", peer_label);
    close(conn_fd);
}

void TcpChat::RunAsClient(const std::string& server_ip, unsigned short port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("socket create failed");
    }

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        close(sockfd);
        throw std::runtime_error("invalid server ip");
    }

    if (connect(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        close(sockfd);
        throw std::runtime_error("connect failed");
    }

    PrintSystem("连接成功。");
    PrintSystem("开始聊天，输入 quit 结束连接。");

    ChatLoop(sockfd, "Client", "Server");
    close(sockfd);
}

void TcpChat::ChatLoop(int sockfd, const std::string& self_label, const std::string& peer_label) {
    running_.store(true);
    std::atomic<bool> prompt_needed(true);

    std::thread receiver([this, sockfd, &prompt_needed, peer_label]() {
        while (running_.load()) {
            std::string encrypted;
            if (!RecvFrame(sockfd, &encrypted)) {
                if (running_.exchange(false)) {
                    PrintSystem("对端已断开连接。");
                }
                shutdown(sockfd, SHUT_RDWR);
                break;
            }
            try {
                std::string plain = cipher_.DecryptFromHex(encrypted);
                PrintPeerMessage(peer_label, plain);
                if (plain == "quit") {
                    PrintSystem("收到 quit，聊天结束。");
                    running_.store(false);
                    shutdown(sockfd, SHUT_RDWR);
                    break;
                }
                prompt_needed.store(true);
            } catch (const std::exception& e) {
                PrintError(std::string("解密失败: ") + e.what());
                prompt_needed.store(true);
            }
        }
    });

    while (running_.load()) {
        if (prompt_needed.exchange(false)) {
            PrintPrompt(self_label);
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000;

        int ready = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            PrintError("输入监视失败。");
            running_.store(false);
            shutdown(sockfd, SHUT_RDWR);
            break;
        }
        if (ready == 0 || !FD_ISSET(STDIN_FILENO, &readfds)) {
            continue;
        }

        std::string line;
        if (!std::getline(std::cin, line)) {
            line = "quit";
        }

        std::string encrypted = cipher_.EncryptToHex(line);
        if (!SendFrame(sockfd, encrypted)) {
            PrintError("发送失败。");
            running_.store(false);
            shutdown(sockfd, SHUT_RDWR);
            break;
        }

        prompt_needed.store(true);
        if (line == "quit") {
            running_.store(false);
            shutdown(sockfd, SHUT_RDWR);
            break;
        }
    }

    if (receiver.joinable()) {
        receiver.join();
    }
}

bool TcpChat::SendFrame(int sockfd, const std::string& payload) {
    uint32_t len = htonl(static_cast<uint32_t>(payload.size()));
    if (!SendAll(sockfd, &len, sizeof(len))) {
        return false;
    }
    if (!payload.empty() && !SendAll(sockfd, payload.data(), payload.size())) {
        return false;
    }
    return true;
}

bool TcpChat::RecvFrame(int sockfd, std::string* payload) {
    uint32_t net_len = 0;
    if (!RecvAll(sockfd, &net_len, sizeof(net_len))) {
        return false;
    }
    uint32_t len = ntohl(net_len);
    if (len > 1024 * 1024) {
        return false;
    }
    payload->assign(len, '\0');
    if (len > 0 && !RecvAll(sockfd, payload->data(), len)) {
        return false;
    }
    return true;
}
