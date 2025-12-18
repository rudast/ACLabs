#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <netinet/in.h>

struct Message {
    int client_fd{-1};
    std::string text;
};

class Server {
public:
    Server(int port, std::size_t maxQueueSize);
    ~Server();

    bool start();
    void stop();

private:
    void acceptLoop();
    void clientLoop(int client_fd);

    void pushMessage(Message msg);
    bool popMessage(Message& msg);

    void workerLoop();

private:
    int port_{0};
    std::size_t maxQueueSize_{0};

    std::atomic<bool> running_{false};

    int listen_fd_{-1};
    sockaddr_in addr_{};

    std::thread acceptor_;
    std::thread worker_;

    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<Message> q_;

    std::mutex send_mtx_;
};
