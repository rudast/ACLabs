#include "server.h"
#include <iostream>

int main() {
    Server s(8081, 10000);
    if (!s.start()) return 1;

    std::cout << "Press ENTER to stop server...\n";
    std::cin.get();

    s.stop();
    return 0;
}
