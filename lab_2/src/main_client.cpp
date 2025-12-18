#include <iostream>
#include <optional>
#include <string>

#include "client.h"

enum class ClientState {
    WAIT_INPUT,
    SEND,
    WAIT_RESPONSE,
    DISCONNECT
};

int main() {
    Client client("127.0.0.1", 8080);
    if (!client.connectToServer()) {
        return 1;
    }

    ClientState state = ClientState::WAIT_INPUT;
    std::string lastInput;

    while (true) {
        switch (state) {
            case ClientState::WAIT_INPUT: {
                if (!client.isConnected()) {
                    std::cout << "Server disconnected\n";
                    state = ClientState::DISCONNECT;
                    break;
                }

                std::cout << "Enter message (ping/q/...): ";
                if (!(std::cin >> lastInput)) {
                    state = ClientState::DISCONNECT;
                    break;
                }

                if (lastInput == "q") {
                    state = ClientState::DISCONNECT;
                } else {
                    state = ClientState::SEND;
                }
                break;
            }

            case ClientState::SEND: {
                if (!client.isConnected()) {
                    std::cout << "Server disconnected\n";
                    state = ClientState::DISCONNECT;
                    break;
                }

                if (!client.sendMessage(lastInput)) {
                    std::cout << "Send failed, disconnecting...\n";
                    state = ClientState::DISCONNECT;
                    break;
                }

                if (lastInput == "ping") {
                    state = ClientState::WAIT_RESPONSE;
                } else {
                    state = ClientState::WAIT_INPUT;
                }
                break;
            }

            case ClientState::WAIT_RESPONSE: {
                if (!client.isConnected()) {
                    std::cout << "Server disconnected\n";
                    state = ClientState::DISCONNECT;
                    break;
                }

                auto answer = client.receiveMessage();
                if (!answer) {
                    state = ClientState::DISCONNECT;
                    break;
                }

                std::cout << "Client received message by Server: " << *answer << '\n';
                
                state = ClientState::WAIT_INPUT;
                break;
            }

            case ClientState::DISCONNECT: {
                client.disconnect();
                return 0;
            }
        }
    }

    return 0;
}
