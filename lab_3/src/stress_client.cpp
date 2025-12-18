#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

static int connect_one(const std::string& ip, int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

static bool send_all(int fd, const char* data, std::size_t len) {
    std::size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd, data + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

static bool recv_some(int fd, std::string& out) {
    char buf[1024];
    ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
    if (n <= 0) return false;
    out.assign(buf, buf + n);
    return true;
}

int main(int argc, char** argv) {
    // ./stress_client [ip] [port] [clients] [msgs_per_client] [hold_ms]
    std::string ip = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? std::stoi(argv[2]) : 8080;
    int clients = (argc > 3) ? std::stoi(argv[3]) : 50;
    int msgs = (argc > 4) ? std::stoi(argv[4]) : 20;
    int hold_ms = (argc > 5) ? std::stoi(argv[5]) : 1000;

    std::cout << "[stress] ip=" << ip
              << " port=" << port
              << " clients=" << clients
              << " msgs/client=" << msgs
              << " hold_ms=" << hold_ms << "\n";

    std::atomic<int> ok_conn{0}, bad_conn{0};
    std::atomic<long long> ok_send{0}, bad_send{0};
    std::atomic<long long> ok_recv{0}, bad_recv{0};

    auto t0 = std::chrono::steady_clock::now();

    std::vector<std::thread> th;
    th.reserve(clients);

    for (int i = 0; i < clients; ++i) {
        th.emplace_back([&, i] {
            int fd = connect_one(ip, port);
            if (fd < 0) { bad_conn++; return; }
            ok_conn++;

            std::this_thread::sleep_for(std::chrono::milliseconds(i % 20));

            for (int k = 0; k < msgs; ++k) {
                std::string s = "client=" + std::to_string(i) +
                                " msg=" + std::to_string(k) + "\n";

                if (send_all(fd, s.c_str(), s.size())) ok_send++;
                else { bad_send++; break; }

                // читаем эхо
                std::string r;
                if (recv_some(fd, r)) ok_recv++;
                else { bad_recv++; break; }

                if ((k % 5) == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            }

            if (hold_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
            }

            ::close(fd);
        });
    }

    for (auto& x : th) x.join();

    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::cout << "[stress] done in " << ms << " ms\n";
    std::cout << "[stress] connections ok=" << ok_conn.load()
              << " bad=" << bad_conn.load() << "\n";
    std::cout << "[stress] sends ok=" << ok_send.load()
              << " bad=" << bad_send.load() << "\n";
    std::cout << "[stress] recvs ok=" << ok_recv.load()
              << " bad=" << bad_recv.load() << "\n";

    return 0;
}
