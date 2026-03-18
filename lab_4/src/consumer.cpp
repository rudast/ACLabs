#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "utils.h"

constexpr uint8_t MSG_CONSUMER = 2;
constexpr uint8_t MSG_TASK = 3;
constexpr uint8_t MSG_RESULT = 4;
constexpr uint8_t MSG_END = 6;
constexpr uint8_t MSG_ACK = 7;

void send_data(int socket_, const void* data, size_t size);
void recv_data(int socket_, void* data, size_t size);
void send_image(int socket_, const Image& image);
Image recv_image(int socket_);

int main(int argc, char* argv[]) {
    std::string broker_host = "127.0.0.1";
    int broker_port = 5001; // Соответствует docker-compose.yaml

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--host" || arg == "-h") && i + 1 < argc) {
            broker_host = argv[++i];
        } else if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            broker_port = std::stoi(argv[++i]);
        }
    }

    struct hostent *server = gethostbyname(broker_host.c_str());
    if (server == nullptr) {
        std::cerr << "[Consumer] Error: No such host " << broker_host << "\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(broker_port);
    std::memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[Consumer] Failed to connect to broker at " << broker_host << ":" << broker_port << "\n";
        return 1;
    }
    std::cout << "[Consumer] Connected to broker\n";

    uint8_t msg = MSG_CONSUMER;
    send_data(sock, &msg, 1);

    while (true) {
        uint8_t type;
        recv_data(sock, &type, 1);

        if (type == MSG_END) {
            std::cout << "[Consumer] No more tasks, exiting\n";
            break;
        }

        if (type == MSG_TASK) {
            Image img = recv_image(sock);
            uint8_t ack = MSG_ACK;
            send_data(sock, &ack, 1);

            process_image(img);
            std::cout << "[Consumer] Task processed\n";

            send_data(sock, &MSG_RESULT, 1);
            send_image(sock, img);
        }
    }

    close(sock);
    return 0;
}

// Реализации send_data, recv_data, send_image, recv_image остаются без изменений
void send_data(int socket_, const void* data, size_t size) {
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    while (size > 0) {
        ssize_t s = send(socket_, ptr, size, MSG_NOSIGNAL);
        if (s <= 0) throw std::runtime_error("Send failed");
        ptr += s; size -= s;
    }
}

void recv_data(int socket_, void* data, size_t size) {
    uint8_t* ptr = static_cast<uint8_t*>(data);
    while (size > 0) {
        ssize_t r = recv(socket_, ptr, size, 0);
        if (r <= 0) throw std::runtime_error("Recv failed");
        ptr += r; size -= r;
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