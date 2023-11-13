#ifndef BANKING_SERVER_H
#define BANKING_SERVER_H

#include <string>
#include <zmq.hpp>
#include <sqlite3.h>
#include "Messages.h"

namespace Server {

    /*
     * This is the server class.
     */
    class Server {

    public:

        /*
        * Initializes the server.
        * The address is the address of the server.
        */
        bool initialize(const std::string &address);

        /*
        * Terminates the server.
        */
        void terminate();

        /*
         * Destroys the client.
         */
        Server();

        /*
         * Destroys the client.
         */
        ~Server();

        /*
         * Handles a request from the client.
         */
        bool handle_request();

    private:
        std::string _address{}; // The address of the server.
        msgpack::zone _z; // this is needed for the msgpack::object constructor
        MSG _msg; // this is the message that will be sent or received
        zmq::context_t _ctx; // create a zmq context
        zmq::socket_t _sock; // create a zmq socket
        sqlite3 *_db{}; // create database handler
        std::vector<LOGIN_RESPONSE> _user_sessions; // hold login response messages for each client

        /*
         * Sends a message to the client.
         */
        void _send_message();

        /*
         * Receives a message from the client.
         */
        bool _receive_message();

        /*
         * Sends a PING message to the client.
         */
        void _send_ping(PING &ping);

        /*
         * Handles a PING message from the client.
         */
        void _handle_ping(PING &ping);

        /*
         * Sends a LOGIN_RESPONSE message to the client.
         */
        void _send_login_response(LOGIN_RESPONSE &login_response);

        /*
         * Handles a LOGIN_REQUEST message from the client.
         */
        void _handle_login_request(const LOGIN_REQUEST &login_request, LOGIN_RESPONSE &login_response);

        /*
         * Sends a LOGOUT_RESPONSE message to the client.
         */
        void _send_logout_response(LOGOUT_RESPONSE &logout_response);

        /*
         * Handles a LOGOUT_REQUEST message from the client.
         */
        void _handle_logout_request(const LOGOUT_REQUEST &logout_request);

        /*
         * Sends an BANK_LIST_RESPONSE message to the client.
         */
        void _send_bank_list_response(BANK_LIST_RESPONSE &bank_list_response);

        /*
         * Handles a BANK_LIST_REQUEST message from the client.
         */
        void _handle_bank_list_request([[maybe_unused]]BANK_LIST_REQUEST &bank_list_request);

        /*
         * Sends an ACCOUNT_LIST_RESPONSE message to the client.
         */
        void _send_account_list_response(ACCOUNT_LIST_RESPONSE &account_list_response);

        /*
         * Handles an ACCOUNT_LIST_REQUEST message from the client.
         */
        void _handle_account_list_request(ACCOUNT_LIST_REQUEST &account_list_request);

        /*
         * Sends an ADD_BALANCE_RESPONSE message to the client.
         */
        void _send_add_balance_response(ADD_BALANCE_RESPONSE &add_balance_response);

        /*
         * Handles an ADD_BALANCE_REQUEST message from the client.
         */
        void _handle_add_balance_request(ADD_BALANCE_REQUEST &add_balance_request,
                                         ADD_BALANCE_RESPONSE &add_balance_response);

        /*
         * Sends a TRANSACTION_RESPONSE message to the client.
         */
        void _send_transaction_response(TRANSACTION_RESPONSE &transaction_response);

        /*
         * Handles a TRANSACTION_REQUEST message from the client.
         */
        void _handle_transaction_request(TRANSACTION_REQUEST &transaction_request,
                                         TRANSACTION_RESPONSE &transaction_response);
    };

} // Server

#endif //BANKING_SERVER_H
