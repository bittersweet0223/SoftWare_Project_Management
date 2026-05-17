#include "des_utils.h"
#include "tcp_chat.h"

#include <iostream>
#include <string>

int main() {
    try {
        std::cout << "\033[1;34m";
        std::cout << "========================================" << std::endl;
        std::cout << "   DES 加密 TCP 聊天程序" << std::endl;
        std::cout << "   角色: s=Server, c=Client" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "\033[0m";
        std::cout << "Client or Server?" << std::endl;
        std::string mode;
        std::getline(std::cin, mode);

        // Pre-shared DES key used by both server and client.
        const std::string key = "12345678";

        DesCipher cipher(key);
        TcpChat chat(cipher);

        const unsigned short port = 9090;

        if (!mode.empty() && (mode[0] == 's' || mode[0] == 'S')) {
            chat.RunAsServer(port);
        } else if (!mode.empty() && (mode[0] == 'c' || mode[0] == 'C')) {
            std::cout << "Please input the server address:" << std::endl;
            std::string ip;
            std::getline(std::cin, ip);
            chat.RunAsClient(ip, port);
        } else {
            std::cout << "Invalid mode. Use s or c." << std::endl;
            return 1;
        }

        std::cout << "\033[1;34m[系统] 聊天结束。\033[0m" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
