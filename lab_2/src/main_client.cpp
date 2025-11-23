#include <iostream>
#include <optional>
#include <string>

#include "client.h"

int main() {
    Client client("127.0.0.1", 8080);
    if (!client.connectToServer()) {
        return 1;
    }

    while (client.isConnected()) {
        std::string input;
        std::cin >> input;

        if (input == std::string("q")) {
            client.disconnect();
            break;
        } else {
            if (client.isConnected()) {
                client.sendMessage(input);
                if (input == "ping") {
                    auto answer = client.receiveMessage();
                    if (answer != std::nullopt) {
                        std::cout << "Client received message by Server: " << answer->c_str()
                                  << '\n';
                    } else {
                        client.disconnect();
                        break;
                    }
                }
            } else {
                std::cout << "Server disconnected" << '\n';
                return 0;
            }
        }
    }
    return 0;
}