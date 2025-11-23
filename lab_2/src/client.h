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
    bool isConnected();

   private:
    int sock;
    std::string host;
    sockaddr_in server_addr{};
};

#endif