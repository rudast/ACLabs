#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

int main() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket error\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "connect error: " << std::strerror(errno) << "\n";
        ::close(fd);
        return 1;
    }

    std::cout << "Connected. Type messages, 'exit' to quit.\n";

    while (true) {
        std::cout << "> ";
        std::string s;
        std::getline(std::cin, s);
        if (s == "exit") break;

        s.push_back('\n');
        ::send(fd, s.c_str(), s.size(), 0);

        // читаем эхо-ответ
        char buf[1024];
        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            std::cout << "server closed\n";
            break;
        }
        std::cout << std::string(buf, buf + n);
    }

    ::close(fd);
    return 0;
}
