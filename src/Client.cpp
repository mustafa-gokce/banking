//
// Created by m on 10.11.2023.
//

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
        std::stringstream buffer;
        msgpack::pack(buffer, _msg);
        const std::string payload = buffer.str();
        _sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
        std::cout << "[client] sent PING\n";
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
        zmq::message_t message;
        (void) _sock.recv(message);

        // parse the message
        _msg = MSG{};
        msgpack::unpacked result;
        std::stringstream sbuf;
        sbuf << message.to_string();
        std::size_t off = 0;
        msgpack::unpack(result, sbuf.str().data(), sbuf.str().size(), off);
        result.get().convert(_msg);

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
        std::cout << "[client] received PING\n";
        std::cout << "        ping.type:" << unsigned (ping.type) << "\n";
        std::cout << "        ping.token:" << ping.token << "\n";
        std::cout << "        ping.client_time:" << ping.client_time << "\n";
        std::cout << "        ping.server_time:" << ping.server_time << "\n";
    }

    void Client::send_bank_list_request(BANK_LIST_REQUEST &bank_list_request) {

        // pack the BANK_LIST_REQUEST message
        _msg = MSG{MSG_ID::BANK_LIST_REQUEST, msgpack::object(bank_list_request, _z)};

        // send the BANK_LIST_REQUEST message
        std::stringstream buffer;
        msgpack::pack(buffer, _msg);
        const std::string payload = buffer.str();
        _sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
        std::cout << "[client] sent BANK_LIST_REQUEST\n";
    }

    void Client::send_bank_list_request() {

        // create a BANK_LIST_REQUEST message
        BANK_LIST_REQUEST bank_list_request;

        // send the BANK_LIST_REQUEST message
        send_bank_list_request(bank_list_request);
    }

    void Client::receive_bank_list_response(BANK_LIST_RESPONSE &bank_list_response) {

        // receive a message
        zmq::message_t message;
        (void) _sock.recv(message);

        // parse the message
        _msg = MSG{};
        msgpack::unpacked result;
        std::stringstream sbuf;
        sbuf << message.to_string();
        std::size_t off = 0;
        msgpack::unpack(result, sbuf.str().data(), sbuf.str().size(), off);
        result.get().convert(_msg);

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
        std::cout << "[client] received BANK_LIST_RESPONSE\n";
        for (const auto &bank: bank_list_response.banks) {
            std::cout << "        bank.id:" << bank.id << ", " << "bank.name:" << bank.name << "\n";
        }
    }

} // Client