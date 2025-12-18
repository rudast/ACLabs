#ifndef CLIENT_H
#define CLIENT_H

#pragma once

#include <netinet/in.h>

#include <optional>
#include <string>

class Client {
public:
    Client(const std::string& host, int port);

    bool connectToServer();
    bool sendMessage(const std::string& msg);
    std::optional<std::string> receiveMessage();
    void disconnect();

    // Простая проверка: есть ли вообще открытый сокет
    bool isConnected() const { return sock >= 0; }

private:
    int sock{-1};
    std::string host;
    sockaddr_in server_addr{};
};

#endif // CLIENT_H
