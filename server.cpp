#include <iostream>
#include <thread>
#include <chrono>
#include "src/Server.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {

    // create server
    Server::Server server;

    // initialize server (bind to address)
    server.initialize("tcp://127.0.0.1:2609");

    // infinite loop
    while (true) {

        // handle incoming requests
        if (!server.handle_request()){
            std::cout << "[server] no message received" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}
