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
        void send_ping(PING &ping);

        /*
         * Receive a ping from the server.
         */
        void receive_ping();
        void receive_ping(PING &ping);

        /*
         * Send a bank list request to the server.
         */
        void send_bank_list_request();
        void send_bank_list_request(BANK_LIST_REQUEST &bank_list_request);

        /*
         * Receive a bank list response from the server.
         */
        void receive_bank_list_response();
        void receive_bank_list_response(BANK_LIST_RESPONSE &bank_list_response);

        /*
         * Send a login request to the server.
         */
        void send_login_request(const std::string& user, const std::string& pass, const uint16_t& bank);
        void send_login_request(LOGIN_REQUEST &login_request);

        /*
         * Receive a login response from the server.
         * Need to save token for future requests.
         */
        void receive_login_response(LOGIN_RESPONSE &login_response);
        [[maybe_unused]] void receive_login_response();

        /*
         * Send a logout request to the server.
         */
        void send_logout_request(const std::string& user, const std::string& token);
        void send_logout_request(LOGOUT_REQUEST &logout_request);

        /*
         * Receive a logout response from the server.
         */
        void receive_logout_response();
        void receive_logout_response(LOGOUT_RESPONSE &logout_response);

        /*
         * Send an account list request to the server.
         */
        void send_account_list_request(const uint32_t &user, const std::string &token, const uint16_t &bank);
        void send_account_list_request(ACCOUNT_LIST_REQUEST &account_list_request);

        /*
         * Receive an account list response from the server.
         * Need to save account list for future requests.
         */
        void receive_account_list_response(ACCOUNT_LIST_RESPONSE &account_list_response);
        [[maybe_unused]] void receive_account_list_response();

        /*
         * Send an add balance request to the server.
         * This is like a superuser adding balance to an IBAN.
         * Or a user adding balance to his/her own IBAN using an ATM.
         */
        void send_add_balance_request(const uint32_t &user, const std::string &token, const uint16_t &bank,
                                      const std::string &iban, const double_t &amount);
        void send_add_balance_request(ADD_BALANCE_REQUEST &add_balance_request);

        /*
         * Receive an add balance response from the server.
         */
        void receive_add_balance_response();
        void receive_add_balance_response(ADD_BALANCE_RESPONSE &add_balance_response);

        /*
         * Send a transaction request to the server.
         */
        void send_transaction_request(const uint32_t &user, const std::string &token, const uint16_t &bank,
                                      const std::string &from, const std::string &to, const double_t &amount);
        void send_transaction_request(TRANSACTION_REQUEST &transaction_request);

        /*
         * Receive a transaction response from the server.
         */
        void receive_transaction_response();
        void receive_transaction_response(TRANSACTION_RESPONSE &transaction_response);

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
