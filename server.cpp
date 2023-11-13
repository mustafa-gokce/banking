#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include "src/Server.h"

// global variable to stop the server
volatile sig_atomic_t stop;

// signal signal_handler
void signal_handler(int signal_number) {
    stop = 1;
    std::cout << "[server] signal " << signal_number << " received" << std::endl;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {

    // register signal SIGINT and signal handler
    signal(SIGINT, signal_handler);

    // create server
    Server::Server server;

    // initialize server (bind to address)
    server.initialize("tcp://127.0.0.1:2609");

    // infinite loop
    while (!stop) {

        // handle incoming requests
        if (!server.handle_request()){
            std::cout << "[server] no message received" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}
