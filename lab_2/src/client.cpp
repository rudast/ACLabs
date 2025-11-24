#include "client.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

Client::Client(const std::string& host, int port) : sock(-1), host(host) {
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid IP address: " << host << '\n';
    }
}

bool Client::connectToServer() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation error: " << std::strerror(errno) << '\n';
        return false;
    }

    int connect_result =
        connect(sock, reinterpret_cast<const struct sockaddr*>(&server_addr),
                sizeof(server_addr));
    if (connect_result < 0) {
        std::cerr << "Connection error: " << std::strerror(errno) << '\n';
        close(sock);
        sock = -1;
        return false;
    }

    std::cout << "Client connected to " << host << '\n';
    return true;
}

bool Client::sendMessage(const std::string& msg) {
    if (sock < 0) {
        std::cerr << "Send error: socket is not connected\n";
        return false;
    }

    ssize_t send_result = send(sock, msg.c_str(), msg.size(), 0);
    if (send_result < 0) {
        std::cerr << "Send error: " << std::strerror(errno) << '\n';
        return false;
    }

    std::cout << "Client send to server: " << msg << '\n';
    return true;
}

std::optional<std::string> Client::receiveMessage() {
    if (sock < 0) {
        std::cerr << "Receive error: socket is not connected\n";
        return {};
    }

    char buff[1024];
    ssize_t messageSize = recv(sock, buff, sizeof(buff), 0);

    if (messageSize == 0) {
        std::cout << "Server disconnected\n";
        close(sock);
        sock = -1;
        return {};
    }

    if (messageSize < 0) {
        switch (errno) {
            case ECONNRESET:
                std::cout << "Server reset connection" << '\n';
                break;

            case ETIMEDOUT:
                std::cout << "Timeout" << '\n';
                break;

            case ENOTCONN:
                std::cout << "Socket is not connected" << '\n';
                break;

            case EFAULT:
                std::cout << "No access to buffer" << '\n';
                break;

            default:
                std::cout << "Receive error: " << std::strerror(errno) << '\n';
                break;
        }
        close(sock);
        sock = -1;
        return {};
    }

    // recv гарантирует, что messageSize <= sizeof(buff)
    return std::string(buff, static_cast<size_t>(messageSize));
}

void Client::disconnect() {
    if (sock >= 0) {
        close(sock);
        sock = -1;
        std::cout << "Client disconnected\n";
    }
}
