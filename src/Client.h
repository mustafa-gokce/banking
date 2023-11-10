//
// Created by m on 10.11.2023.
//

#ifndef BANKING_CLIENT_H
#define BANKING_CLIENT_H

#include <string>
#include <zmq.hpp>
#include "Messages.h"

namespace Client {

    /*
     * This is the client class.
     */
    class Client {
    public:

        /*
         * Initializes the client.
         * The address is the address of the server.
         */
        void initialize(const std::string &address);

        /*
         * Terminates the client.
         */
        void terminate();

        /*
         * Send ping to the server.
         */
        void send_ping();

        /*
         * Receive a ping from the server.
         */
        void receive_ping(PING &ping);

        /*
         * Destroys the client.
         */
        Client();

        /*
         * Destroys the client.
         */
        ~Client();

    private:
        std::string _address{}; // The address of the server.
        msgpack::zone _z; // this is needed for the msgpack::object constructor
        MSG _msg; // this is the message that will be sent or received
        zmq::context_t _ctx; // create a zmq context
        zmq::socket_t _sock; // create a zmq socket
    };

} // Client

#endif //BANKING_CLIENT_H