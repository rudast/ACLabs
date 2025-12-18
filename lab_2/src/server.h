#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>

enum class ServerStates { NONE, WAIT, PROCESS, ERROR, ANSWER };

class Server {
   public:
    Server(int port, int queueSize);
    ~Server();

    bool start();
    void run();
    void stop();

    ServerStates getState() const {
        return state;
    }

   private:
    void handleClient(int client);

    ServerStates state = ServerStates::NONE;
    int queueSize;
    int server_socket;
    std::atomic<bool> is_running{true};

    sockaddr_in settings;
};

#endif