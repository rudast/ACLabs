#include "server.h"

#include <netinet/in.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <thread>

Server::Server(int port, int queueSize) : queueSize(queueSize), server_socket(-1) {
    settings.sin_family = AF_INET;
    settings.sin_port = htons(port);
    settings.sin_addr.s_addr = htonl(INADDR_ANY);
}

Server::~Server() {
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
        state = ServerStates::NONE;
    }
}

bool Server::start() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Socket creation error: " << std::strerror(errno) << '\n';
        return false;
    }

    int bind_result = bind(server_socket, (const struct sockaddr*)&settings, sizeof(settings));

    if (bind_result < 0) {
        std::cerr << "Bind error: " << std::strerror(errno) << '\n';
        close(server_socket);
        return false;
    }

    int listen_result = listen(server_socket, queueSize);

    if (listen_result < 0) {
        std::cerr << "Listen error: " << std::strerror(errno) << '\n';
        close(server_socket);
        return false;
    }
    state = ServerStates::WAIT;

    return true;
}

void Server::handleClient(int client) {
    state = ServerStates::PROCESS;
    char buff[1024];
    int messageSize;

    while ((messageSize = recv(client, buff, sizeof(buff), 0)) > 0) {
        if (messageSize >= 1023) messageSize = 1023;
        buff[messageSize] = '\0';

        std::cout << "Server received message by client #" << client << ": " << buff << '\n';

        state = ServerStates::ANSWER;
        std::string msg(buff);
        if (msg == "ping") {
            if (send(client, "pong", 4, 0) < 0) {
                std::cerr << "Send error: " << std::strerror(errno) << '\n';
            } else {
                std::cout << "Server send to client #" << client << ": pong\n";
            }
        }
    }

    if (messageSize == 0) {
        std::cout << "Client " << client << " disconnected\n";

    } else if (messageSize < 0) {
        state = ServerStates::ERROR;
        switch (errno) {
            case ECONNRESET:
                std::cout << "Client #" << client << " reset connection" << '\n';
                break;

            case ETIMEDOUT:
                std::cout << "Timeout by client #" << client << '\n';
                break;

            case ENOTCONN:
                std::cout << "Socket is not connected by client #" << client << '\n';
                break;

            case EFAULT:
                std::cout << "No access to buffer by client #" << client << '\n';
                break;

            default:
                std::cout << "Receive error by client #" << client << ": " << std::strerror(errno)
                          << '\n';
                break;
        }
    }

    close(client);
    state = ServerStates::WAIT;
}

void Server::run() {
    while (is_running) {
        int client = 0;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        client = accept(server_socket, (sockaddr*)&client_addr, &client_len);

        if (client < 0) {
            state = ServerStates::ERROR;
            if (errno == EMFILE || errno == ENOMEM) {
                std::cerr << "Accept error: " << std::strerror(errno) << '\n';
                break;
            }
            std::cerr << "Accept error: " << std::strerror(errno) << '\n';
            state = ServerStates::WAIT;
            continue;
        }

        std::cout << "Client #" << client << " connected\n";
        std::thread(&Server::handleClient, this, client).detach();
    }
}

void Server::stop() {
    is_running = false;
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
        state = ServerStates::NONE;
    }
    is_running = false;
    std::cout << "Server stopped.\n";
}