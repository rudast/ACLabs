#include <arpa/inet.h>
#include <dirent.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "utils.h"

constexpr uint8_t MSG_PRODUCER = 1;
constexpr uint8_t MSG_TASK = 3;
constexpr uint8_t MSG_END = 6;

void send_data(int socket_, const void* data, size_t size);
void recv_data(int socket_, void* data, size_t size);
void send_image(int socket_, const Image& image);
std::vector<std::string> get_ppm_files(const std::string& dir);

int main(int argc, char* argv[]) {
    std::string broker_host = "127.0.0.1";
    int broker_port = 5001;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--host" || arg == "-h") && i + 1 < argc) {
            broker_host = argv[++i];
        } else if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            broker_port = std::stoi(argv[++i]);
        }
    }

    struct hostent* server = gethostbyname(broker_host.c_str());
    if (server == nullptr) {
        std::cerr << "[Producer] Error: No such host " << broker_host << "\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(broker_port);
    std::memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);

    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(client, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[Producer] Failed to connect to broker at " << broker_host << ":"
                  << broker_port << "\n";
        return 1;
    }
    std::cout << "[Producer] Connected to broker\n";

    uint8_t msg = MSG_PRODUCER;
    send_data(client, &msg, 1);

    std::vector<std::string> files = get_ppm_files("./images");
    std::cout << "[Producer] Found " << files.size() << " files\n";

    for (const auto& path : files) {
        try {
            Image img = read_file(path);
            uint8_t task_msg = MSG_TASK;
            send_data(client, &task_msg, 1);
            send_image(client, img);
            std::cout << "[Producer] Sent: " << path << "\n";
        } catch (const std::exception& e) {
            std::cerr << "[Producer] Error reading " << path << ": " << e.what() << "\n";
        }
    }

    uint8_t end_msg = MSG_END;
    send_data(client, &end_msg, 1);
    std::cout << "[Producer] Waiting for completion...\n";

    uint8_t done_msg;
    recv_data(client, &done_msg, 1);
    if (done_msg == MSG_END) {
        std::cout << "[Producer] All tasks completed!\n";
    }

    close(client);
    return 0;
}

void send_data(int socket_, const void* data, size_t size) {
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    while (size > 0) {
        ssize_t s = send(socket_, ptr, size, MSG_NOSIGNAL);
        if (s <= 0) throw std::runtime_error("Send failed");
        ptr += s;
        size -= s;
    }
}

void recv_data(int socket_, void* data, size_t size) {
    uint8_t* ptr = static_cast<uint8_t*>(data);
    while (size > 0) {
        ssize_t r = recv(socket_, ptr, size, 0);
        if (r <= 0) throw std::runtime_error("Recv failed");
        ptr += r;
        size -= r;
    }
}

void send_image(int socket_, const Image& image) {
    send_data(socket_, &image.width, sizeof(image.width));
    send_data(socket_, &image.height, sizeof(image.height));
    uint32_t size = static_cast<uint32_t>(image.bytes.size());
    send_data(socket_, &size, sizeof(size));
    send_data(socket_, image.bytes.data(), size);
}

std::vector<std::string> get_ppm_files(const std::string& dir) {
    std::vector<std::string> files;
    DIR* d = opendir(dir.c_str());
    if (!d) return files;
    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr) {
        std::string name = entry->d_name;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".ppm") {
            files.push_back(dir + "/" + name);
        }
    }
    closedir(d);
    return files;
}