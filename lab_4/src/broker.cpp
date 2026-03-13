#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>

#include "utils.h"

constexpr uint8_t MSG_PRODUCER = 1;
constexpr uint8_t MSG_CONSUMER = 2;
constexpr uint8_t MSG_TASK = 3;
constexpr uint8_t MSG_RESULT = 4;
// constexpr uint8_t MSG_NO_TASK = 5;
constexpr uint8_t MSG_END = 6;
constexpr uint8_t MSG_ACK = 7;

void send_data(int socket_, const void* data, size_t size);
void recv_data(int socket_, void* data, size_t size);
void send_image(int socket_, const Image& image);
Image recv_image(int socket_);
void handle_producer(int client_sock);
void handle_consumer(int client_sock);
void handle_client(int client_sock);

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 5000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--host" || arg == "-h") && i + 1 < argc) {
            host = argv[++i];
        } else if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [--host IP] [--port PORT]\n";
            std::cout << "  --host, -h  IP address to bind (default: 127.0.0.1)\n";
            std::cout << "  --port, -p  Port to listen (default: 5000)\n";
            return 0;
        }
    }

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        std::cerr << "[Broker] Failed to create socket\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host.c_str());
    addr.sin_port = htons(port);

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[Broker] Failed to bind " << host << ":" << port << "\n";
        return 1;
    }

    if (listen(server, 10) < 0) {
        std::cerr << "[Broker] Failed to listen\n";
        return 1;
    }

    std::cout << "[Broker] Listening on " << host << ":" << port << "...\n";

    while (true) {
        int client_sock = accept(server, nullptr, nullptr);
        if (client_sock < 0) {
            std::cerr << "[Broker] Accept failed\n";
            continue;
        }

        std::thread(handle_client, client_sock).detach();
    }

    close(server);
    return 0;
}

std::queue<Image> task_queue;
std::mutex mutex_queue;
std::condition_variable cv_queue;

size_t total_tasks = 0;
size_t completed_tasks = 0;
std::mutex mutex_stats;
std::condition_variable cv_stats;

std::atomic<bool> producer_done{false};
std::atomic<int> producer_socket{-1};

void send_data(int socket_, const void* data, size_t size) {
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    while (size > 0) {
        ssize_t send_ = send(socket_, ptr, size, MSG_NOSIGNAL);
        if (send_ <= 0) {
            throw std::runtime_error("[Broker] Failed to send data");
        }

        ptr += send_;
        size -= send_;
    }
}

void recv_data(int socket_, void* data, size_t size) {
    uint8_t* ptr = static_cast<uint8_t*>(data);
    while (size > 0) {
        ssize_t recv_ = recv(socket_, ptr, size, 0);
        if (recv_ <= 0) {
            throw std::runtime_error("[Broker] Failed to recv data");
        }
        ptr += recv_;
        size -= recv_;
    }
}

void send_image(int socket_, const Image& image) {
    send_data(socket_, &image.width, sizeof(image.width));
    send_data(socket_, &image.height, sizeof(image.height));
    uint32_t size = static_cast<uint32_t>(image.bytes.size());
    send_data(socket_, &size, sizeof(size));
    send_data(socket_, image.bytes.data(), size);
}

Image recv_image(int socket_) {
    Image image;
    recv_data(socket_, &image.width, sizeof(image.width));
    recv_data(socket_, &image.height, sizeof(image.height));
    uint32_t size = 0;
    recv_data(socket_, &size, sizeof(size));
    image.bytes.resize(size);
    recv_data(socket_, image.bytes.data(), size);

    return image;
}

void handle_producer(int client_socket) {
    std::cout << "[Broker] Producer connected\n";
    producer_socket.store(client_socket);

    try {
        while (true) {
            uint8_t type = 0;
            recv_data(client_socket, &type, sizeof(type));

            if (type == MSG_END) {
                std::cout << "[Broker] Producer finished sending tasks\n";
                break;
            }

            if (type == MSG_TASK) {
                Image image = recv_image(client_socket);
                {
                    std::lock_guard<std::mutex> lock(mutex_queue);
                    task_queue.push(image);
                    total_tasks++;
                }
                cv_queue.notify_one();
                std::cout << "[Broker] Task received (total: " << total_tasks << ")\n";
            }
        }
    } catch (std::exception& e) {
        std::cerr << "[Broker] Producer error: " << e.what() << "\n";
        close(client_socket);
        producer_socket.store(-1);
        producer_done.store(true);
        cv_queue.notify_all();
        return;
    }

    std::cout << "[Broker] Waiting for " << total_tasks << " tasks to complete...\n";
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mutex_stats);
            if (completed_tasks >= total_tasks) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "[Broker] All tasks completed, sending confirmation to Producer\n";

    try {
        uint8_t done_msg = MSG_END;
        send_data(client_socket, &done_msg, 1);
    } catch (const std::exception& e) {
        std::cerr << "[Broker] Failed to send confirmation: " << e.what() << "\n";
    }

    close(client_socket);
    producer_socket.store(-1);
    producer_done.store(true);
    cv_queue.notify_all();

    std::cout << "[Broker] Producer disconnected\n";
}

void handle_consumer(int client_socket) {
    std::cout << "[Broker] Consumer connected\n";

    try {
        while (true) {
            Image image;

            {
                std::unique_lock<std::mutex> lock(mutex_queue);
                cv_queue.wait(lock, [] { return !task_queue.empty() || producer_done; });

                if (task_queue.empty() && producer_done) {
                    send_data(client_socket, &MSG_END, 1);
                    break;
                }

                image = task_queue.front();
            }

            try {
                send_data(client_socket, &MSG_TASK, 1);
                send_image(client_socket, image);

                uint8_t ack;
                recv_data(client_socket, &ack, 1);
                if (ack != MSG_ACK) {
                    throw std::runtime_error("Expected ACK, got: " + std::to_string(ack));
                }
            } catch (const std::exception& e) {
                std::cerr << "[Broker] Consumer disconnected (task returned to queue): " << e.what()
                          << "\n";
                close(client_socket);
                return;
            }

            {
                std::lock_guard<std::mutex> lock(mutex_queue);
                task_queue.pop();
            }

            try {
                uint8_t type;
                recv_data(client_socket, &type, 1);

                if (type == MSG_RESULT) {
                    Image result = recv_image(client_socket);
                    static size_t counter = 0;
                    std::string path = "./output/result_" + std::to_string(++counter) + ".ppm";
                    write_file(path, result);

                    {
                        std::lock_guard<std::mutex> lock(mutex_stats);
                        ++completed_tasks;
                        std::cout << "[Broker] Result saved [" << completed_tasks << '/'
                                  << total_tasks << "]\n";
                    }
                } else if (type == MSG_END) {
                    break;
                }
            } catch (const std::exception& e) {
                std::cerr << "[Broker] Consumer disconnected after ACK (result lost): " << e.what()
                          << "\n";
                close(client_socket);
                return;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Broker] Consumer error: " << e.what() << "\n";
    }

    close(client_socket);
}

void handle_client(int client_socket) {
    try {
        uint8_t type = 0;
        recv_data(client_socket, &type, 1);

        if (type == MSG_PRODUCER) {
            handle_producer(client_socket);
        } else if (type == MSG_CONSUMER) {
            handle_consumer(client_socket);
        } else {
            std::cerr << "[Broker] Unknown client type: " << (int)type << "\n";
        }

    } catch (std::exception& e) {
        std::cerr << "[Broker] Client error: " << e.what() << "\n";
        close(client_socket);
    }
}