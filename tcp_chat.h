#ifndef TCP_CHAT_H
#define TCP_CHAT_H

#include "des_utils.h"

#include <atomic>
#include <string>

class TcpChat {
public:
    explicit TcpChat(DesCipher cipher);
    void RunAsServer(unsigned short port);
    void RunAsClient(const std::string& server_ip, unsigned short port);

private:
    void ChatLoop(int sockfd, const std::string& self_label, const std::string& peer_label);
    bool SendFrame(int sockfd, const std::string& payload);
    bool RecvFrame(int sockfd, std::string* payload);

    DesCipher cipher_;
    std::atomic<bool> running_;
};

#endif
