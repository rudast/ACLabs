#include "server.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <utility>

static void close_fd(int& fd) {
    if (fd >= 0) { ::close(fd); fd = -1; }
}

Server::Server(int port, std::size_t maxQueueSize)
    : port_(port), maxQueueSize_(maxQueueSize) {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(static_cast<uint16_t>(port_));
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
}

Server::~Server() { stop(); }

bool Server::start() {
    if (running_) return true;

    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        std::cerr << "[server] socket() error: " << std::strerror(errno) << "\n";
        return false;
    }

    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (::bind(listen_fd_, (sockaddr*)&addr_, sizeof(addr_)) < 0) {
        std::cerr << "[server] bind() error: " << std::strerror(errno) << "\n";
        close_fd(listen_fd_);
        return false;
    }

    if (::listen(listen_fd_, 128) < 0) {
        std::cerr << "[server] listen() error: " << std::strerror(errno) << "\n";
        close_fd(listen_fd_);
        return false;
    }

    running_ = true;

    worker_   = std::thread(&Server::workerLoop, this);
    acceptor_ = std::thread(&Server::acceptLoop, this);

    std::cout << "[server] started, port=" << port_ << "\n";
    return true;
}

void Server::stop() {
    if (!running_) return;

    running_ = false;
    cv_.notify_all();

    close_fd(listen_fd_);

    if (acceptor_.joinable()) acceptor_.join();
    if (worker_.joinable()) worker_.join();

    {
        std::lock_guard<std::mutex> lock(mtx_);
        while (!q_.empty()) q_.pop();
    }

    std::cout << "[server] stopped\n";
}

void Server::acceptLoop() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);

        int client_fd = ::accept(listen_fd_, (sockaddr*)&client_addr, &len);
        if (client_fd < 0) {
            if (!running_) break;
            std::cerr << "[acceptor] accept() error: " << std::strerror(errno) << "\n";
            continue;
        }

        char ipbuf[INET_ADDRSTRLEN]{};
        ::inet_ntop(AF_INET, &client_addr.sin_addr, ipbuf, sizeof(ipbuf));
        std::cout << "[acceptor] new client fd=" << client_fd
                  << " from " << ipbuf << ":" << ntohs(client_addr.sin_port) << "\n";

        std::thread(&Server::clientLoop, this, client_fd).detach();
    }
}

void Server::clientLoop(int client_fd) {
    std::string acc;
    char buf[1024];

    while (running_) {
        ssize_t n = ::recv(client_fd, buf, sizeof(buf), 0);

        if (n == 0) {
            std::cout << "[client] fd=" << client_fd << " disconnected\n";
            break;
        }
        if (n < 0) {
            std::cerr << "[client] fd=" << client_fd << " recv() error: "
                      << std::strerror(errno) << "\n";
            break;
        }

        acc.append(buf, buf + n);

        std::size_t pos = 0;
        while (true) {
            std::size_t nl = acc.find('\n', pos);
            if (nl == std::string::npos) break;

            std::string line = acc.substr(pos, nl - pos);
            pos = nl + 1;

            if (!line.empty()) pushMessage(Message{client_fd, line});
        }
        acc.erase(0, pos);
    }

    close_fd(client_fd);
}

void Server::pushMessage(Message msg) {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [&] { return !running_ || q_.size() < maxQueueSize_; });
    if (!running_) return;

    q_.push(std::move(msg));
    cv_.notify_all();
}

bool Server::popMessage(Message& msg) {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [&] { return !running_ || !q_.empty(); });

    if (!running_ && q_.empty()) return false;

    msg = std::move(q_.front());
    q_.pop();
    cv_.notify_all();
    return true;
}

void Server::workerLoop() {
    Message msg;
    while (popMessage(msg)) {
        std::cout << "[worker] fd=" << msg.client_fd << " text=\"" << msg.text << "\"\n";

        std::string reply = "OK: " + msg.text + "\n";
        std::lock_guard<std::mutex> lock(send_mtx_);
        if (::send(msg.client_fd, reply.c_str(), reply.size(), 0) < 0) {
            std::cerr << "[worker] send() error to fd=" << msg.client_fd
                      << ": " << std::strerror(errno) << "\n";
        }
    }
}
