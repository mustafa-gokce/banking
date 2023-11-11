#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <msgpack.hpp>
#include "Client.h"
#include "Tools.h"

namespace Client {

    Client::Client() {
        std::cout << "[client] created" << std::endl;
    }

    Client::~Client() {
        terminate();
        std::cout << "[client] destroyed" << std::endl;
    }

    void Client::initialize(const std::string& address) {
        _address = address;

        // connect to the server
        _sock = zmq::socket_t(_ctx, ZMQ_REQ);
        _sock.connect(_address);

        // wait for a second for ZMQ to properly initialize
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        std::cout << "[client] initialized" << std::endl;
    }

    void Client::terminate() {
        if (_sock) {
            _sock.close();
            std::cout << "[client] socket connection closed" << std::endl;
        }
        if (_ctx.handle() != nullptr) {
            _ctx.close();
            std::cout << "[client] socket context closed" << std::endl;
        }
    }

    void Client::send_ping(PING &ping) {

        // pack the PING message
        _msg = MSG{MSG_ID::PING, msgpack::object(ping, _z)};

        // send the PING message
        _send_message();
        std::cout << "[client] sent PING" << std::endl;;
    }

    void Client::send_ping() {

        // create a PING message
        PING ping;
        ping.type = PING_TYPE::CLIENT;
        ping.client_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        std::string random_token = Tools::Tools::random_string(32);
        ping.token = random_token;

        // send the PING message
        send_ping(ping);
    }

    void Client::receive_ping(PING &ping) {

        // receive a message
        _receive_message();

        // handle the PING message
        if (_msg.id == MSG_ID::PING) {

            // parse the PING message
            _msg.msg.convert(ping);
        }
    }

    void Client::receive_ping() {

        // receive a PING message
        PING ping;
        receive_ping(ping);

        // print the PING message
        std::cout << "[client] received PING" << std::endl;;
        std::cout << "        ping.type:" << unsigned (ping.type) << std::endl;
        std::cout << "        ping.token:" << ping.token << std::endl;
        std::cout << "        ping.client_time:" << ping.client_time << std::endl;
        std::cout << "        ping.server_time:" << ping.server_time << std::endl;
    }

    void Client::send_bank_list_request(BANK_LIST_REQUEST &bank_list_request) {

        // pack the BANK_LIST_REQUEST message
        _msg = MSG{MSG_ID::BANK_LIST_REQUEST, msgpack::object(bank_list_request, _z)};

        // send the BANK_LIST_REQUEST message
        _send_message();
        std::cout << "[client] sent BANK_LIST_REQUEST" << std::endl;;
    }

    void Client::send_bank_list_request() {

        // create a BANK_LIST_REQUEST message
        BANK_LIST_REQUEST bank_list_request;

        // send the BANK_LIST_REQUEST message
        send_bank_list_request(bank_list_request);
    }

    void Client::receive_bank_list_response(BANK_LIST_RESPONSE &bank_list_response) {

        // receive a message
        _receive_message();

        // handle the BANK_LIST_RESPONSE message
        if (_msg.id == MSG_ID::BANK_LIST_RESPONSE) {

            // parse the BANK_LIST_RESPONSE message
            _msg.msg.convert(bank_list_response);
        }
    }

    void Client::receive_bank_list_response() {

        // receive a BANK_LIST_RESPONSE message
        BANK_LIST_RESPONSE bank_list_response;
        receive_bank_list_response(bank_list_response);

        // print the BANK_LIST_RESPONSE
        std::cout << "[client] received BANK_LIST_RESPONSE" << std::endl;;
        for (const auto &bank: bank_list_response.banks) {
            std::cout << "        bank.id:" << bank.id << ", " << "bank.name:" << bank.name << std::endl;
        }
    }

    void Client::send_login_request(LOGIN_REQUEST &login_request) {

        // pack the LOGIN_REQUEST message
        _msg = MSG{MSG_ID::LOGIN_REQUEST, msgpack::object(login_request, _z)};

        // send the LOGIN_REQUEST message
        _send_message();
        std::cout << "[client] sent LOGIN_REQUEST" << std::endl;;
    }

    void Client::send_login_request(const std::string& user, const std::string& pass, const uint16_t& bank) {

        // create a LOGIN_REQUEST message
        LOGIN_REQUEST login_request;
        login_request.user = user;
        login_request.pass = pass;
        login_request.bank = bank;

        // send the LOGIN_REQUEST message
        send_login_request(login_request);
    }

    void Client::receive_login_response(LOGIN_RESPONSE &login_response) {

        // receive a message
        _receive_message();

        // handle the LOGIN_RESPONSE message
        if (_msg.id == MSG_ID::LOGIN_RESPONSE) {

            // parse the LOGIN_RESPONSE message
            _msg.msg.convert(login_response);
        }

        // print the LOGIN_RESPONSE
        std::cout << "[client] received LOGIN_RESPONSE" << std::endl;;
        std::cout << "        login_response.type:" << unsigned(login_response.type) << std::endl;
        std::cout << "        login_response.id:" << unsigned(login_response.id) << std::endl;
        std::cout << "        login_response.bank:" << unsigned(login_response.bank) << std::endl;
        std::cout << "        login_response.citizen:" << unsigned(login_response.citizen) << std::endl;
        std::cout << "        login_response.name:" << login_response.name << std::endl;
        std::cout << "        login_response.user:" << login_response.user << std::endl;
        std::cout << "        login_response.token:" << login_response.token << std::endl;
    }

    [[maybe_unused]] void Client::receive_login_response() {

        // receive a LOGIN_RESPONSE message
        LOGIN_RESPONSE login_response;
        receive_login_response(login_response);
    }

    void Client::send_logout_request(LOGOUT_REQUEST &logout_request) {

        // pack the LOGOUT_REQUEST message
        _msg = MSG{MSG_ID::LOGOUT_REQUEST, msgpack::object(logout_request, _z)};

        // send the LOGOUT_REQUEST message
        _send_message();
        std::cout << "[client] sent LOGOUT_REQUEST" << std::endl;;
    }

    void Client::send_logout_request(const std::string &user, const std::string &token) {

        // create a LOGOUT_REQUEST message
        LOGOUT_REQUEST logout_request;
        logout_request.user = user;
        logout_request.token = token;

        // send the LOGOUT_REQUEST message
        send_logout_request(logout_request);
    }

    void Client::receive_logout_response(LOGOUT_RESPONSE &logout_response) {

        // receive a message
        _receive_message();

        // handle the LOGOUT_RESPONSE message
        if (_msg.id == MSG_ID::LOGOUT_RESPONSE) {

            // parse the LOGOUT_RESPONSE message
            _msg.msg.convert(logout_response);
        }
    }

    void Client::receive_logout_response() {

        // receive a LOGOUT_RESPONSE message
        LOGOUT_RESPONSE logout_response;
        receive_logout_response(logout_response);

        // print the LOGOUT_RESPONSE
        std::cout << "[client] received LOGOUT_RESPONSE" << std::endl;;
        std::cout << "        logout_response.type:" << unsigned(logout_response.type) << std::endl;
    }

    void Client::send_account_list_request(ACCOUNT_LIST_REQUEST &account_list_request) {

        // pack the ACCOUNT_LIST_REQUEST message
        _msg = MSG{MSG_ID::ACCOUNT_LIST_REQUEST, msgpack::object(account_list_request, _z)};

        // send the ACCOUNT_LIST_REQUEST message
        _send_message();
        std::cout << "[client] sent ACCOUNT_LIST_REQUEST" << std::endl;;
    }

    void Client::send_account_list_request(const uint32_t &user, const std::string &token, const uint16_t &bank) {

        // create a ACCOUNT_LIST_REQUEST message
        ACCOUNT_LIST_REQUEST account_list_request;
        account_list_request.user = user;
        account_list_request.token = token;
        account_list_request.bank = bank;

        // send the ACCOUNT_LIST_REQUEST message
        send_account_list_request(account_list_request);
    }

    void Client::receive_account_list_response(ACCOUNT_LIST_RESPONSE &account_list_response) {

        // receive a message
        _receive_message();

        // handle the ACCOUNT_LIST_RESPONSE message
        if (_msg.id == MSG_ID::ACCOUNT_LIST_RESPONSE) {

            // parse the ACCOUNT_LIST_RESPONSE message
            _msg.msg.convert(account_list_response);
        }

        // print the ACCOUNT_LIST_RESPONSE
        std::cout << "[client] received ACCOUNT_LIST_RESPONSE" << std::endl;;
        for (const auto &account: account_list_response.accounts) {
            std::cout << "        account.iban:" << account.iban;
            std::cout << ", account.user:" << unsigned(account.user);
            std::cout << ", account.bank:" << unsigned(account.bank);
            std::cout.precision(2);
            std::cout << std::fixed;
            std::cout << ", account.balance:" << account.balance << std::endl;
        }
    }

    void Client::receive_account_list_response() {

        // receive a ACCOUNT_LIST_RESPONSE message
        ACCOUNT_LIST_RESPONSE account_list_response;
        receive_account_list_response(account_list_response);
    }

    void Client::send_add_balance_request(ADD_BALANCE_REQUEST &add_balance_request) {

        // pack the ADD_BALANCE_REQUEST message
        _msg = MSG{MSG_ID::ADD_BALANCE_REQUEST, msgpack::object(add_balance_request, _z)};

        // send the ADD_BALANCE_REQUEST message
        _send_message();
        std::cout << "[client] sent ADD_BALANCE_REQUEST" << std::endl;;
    }

    void Client::send_add_balance_request(const uint32_t &user, const std::string &token, const uint16_t &bank,
                                          const std::string &iban, const double_t &amount) {

        // create an ADD_BALANCE_REQUEST message
        ADD_BALANCE_REQUEST add_balance_request;
        add_balance_request.user = user;
        add_balance_request.token = token;
        add_balance_request.bank = bank;
        add_balance_request.iban = iban;
        add_balance_request.amount = amount;

        // send the ADD_BALANCE_REQUEST message
        send_add_balance_request(add_balance_request);
    }

    void Client::receive_add_balance_response(ADD_BALANCE_RESPONSE &add_balance_response) {

        // receive a message
        _receive_message();

        // handle the ADD_BALANCE_RESPONSE message
        if (_msg.id == MSG_ID::ADD_BALANCE_RESPONSE) {

            // parse the ADD_BALANCE_RESPONSE message
            _msg.msg.convert(add_balance_response);
        }
    }

    void Client::receive_add_balance_response() {

        // receive an ADD_BALANCE_RESPONSE message
        ADD_BALANCE_RESPONSE add_balance_response;
        receive_add_balance_response(add_balance_response);

        // print the ADD_BALANCE_RESPONSE
        std::cout << "[client] received ADD_BALANCE_RESPONSE" << std::endl;;
        std::cout.precision(2);
        std::cout << std::fixed;
        std::cout << "        add_balance_response.amount:" << add_balance_response.amount << std::endl;
    }

    void Client::send_transaction_request(TRANSACTION_REQUEST &transaction_request) {

        // pack the TRANSACTION_REQUEST message
        _msg = MSG{MSG_ID::TRANSACTION_REQUEST, msgpack::object(transaction_request, _z)};

        // send the TRANSACTION_REQUEST message
        _send_message();
        std::cout << "[client] sent TRANSACTION_REQUEST" << std::endl;;
    }

    void Client::send_transaction_request(const uint32_t &user, const std::string &token, const uint16_t &bank,
                                          const std::string &from, const std::string &to, const double_t &amount) {

        // create a TRANSACTION_REQUEST message
        TRANSACTION_REQUEST transaction_request;
        transaction_request.user = user;
        transaction_request.token = token;
        transaction_request.bank = bank;
        transaction_request.from = from;
        transaction_request.to = to;
        transaction_request.amount = amount;

        // send the TRANSACTION_REQUEST message
        send_transaction_request(transaction_request);
    }

    void Client::receive_transaction_response(TRANSACTION_RESPONSE &transaction_response) {

        // receive a message
        _receive_message();

        // handle the TRANSACTION_RESPONSE message
        if (_msg.id == MSG_ID::TRANSACTION_RESPONSE) {

            // parse the TRANSACTION_RESPONSE message
            _msg.msg.convert(transaction_response);
        }
    }

    void Client::receive_transaction_response() {

        // receive a TRANSACTION_RESPONSE message
        TRANSACTION_RESPONSE transaction_response;
        receive_transaction_response(transaction_response);

        // print the TRANSACTION_RESPONSE
        std::cout << "[client] received TRANSACTION_RESPONSE" << std::endl;;
        std::cout << "        transaction_response.type:" << unsigned(transaction_response.type) << std::endl;
        std::cout << "        transaction_response.token:" << transaction_response.token << std::endl;
        std::cout.precision(2);
        std::cout << std::fixed;
        std::cout << "        transaction_response.fee:" << transaction_response.fee << std::endl;
    }

    void Client::_send_message() {

        // serialize the message
        std::stringstream buffer;
        msgpack::pack(buffer, _msg);
        const std::string payload = buffer.str();

        // send the message
        _sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
    }

    void Client::_receive_message() {

        // receive a message
        zmq::message_t message;
        (void) _sock.recv(message);

        // clear the message
        _msg = MSG{};

        // unpack the message
        msgpack::unpacked result;
        std::stringstream sbuf;
        sbuf << message.to_string();
        std::size_t off = 0;
        msgpack::unpack(result, sbuf.str().data(), sbuf.str().size(), off);

        // convert msgpack::object to MSG
        result.get().convert(_msg);
    }

} // Client