#include <random>
#include <string>
#include <QApplication>
#include <QPushButton>
#include <iostream>
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include <msgpack.hpp>
#include "messages.h"

std::string random_string(std::string::size_type length) {
    static auto &chars = "0123456789"
                         "abcdefghijklmnopqrstuvwxyz"
                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chars) - 2);

    std::string s;

    s.reserve(length);

    while (length--)
        s += chars[pick(rg)];

    return s;
}

int main(int argc, char *argv[]) {
    /*
    QApplication a(argc, argv);
    QPushButton button("Hello world!", nullptr);
    button.resize(200, 100);
    button.show();
    return QApplication::exec();
     */

    // create a msgpack zone
    msgpack::zone z;
    MSG msg;

    // create a zmq context and socket
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::req);
    sock.connect("tcp://127.0.0.1:2609");

    // wait for a second for ZMQ to properly initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // create a PING message
    PING ping;
    ping.type = PING_TYPE::CLIENT;
    ping.client_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    std::string random_token = random_string(32);
    ping.token = random_token;

    // pack the PING message
    msg = MSG{MSG_ID::PING, msgpack::object(ping, z)};

    // send the PING message
    std::stringstream buffer;
    msgpack::pack(buffer, msg);
    const std::string payload = buffer.str();
    sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
    std::cout << "[client] sent PING\n";

    // receive a message
    zmq::message_t message;
    (void) sock.recv(message);

    // parse the message
    msg = MSG{};
    msgpack::unpacked result;
    std::stringstream sbuf;
    sbuf << message.to_string();
    std::size_t off = 0;
    msgpack::unpack(result, sbuf.str().data(), sbuf.str().size(), off);
    result.get().convert(msg);

    // handle the PING message
    if (msg.id == MSG_ID::PING) {

        // parse the PING message
        ping = PING{};
        msg.msg.convert(ping);

        // check validity by comparing the token
        bool ping_valid = ping.token == random_token;

        // print the PING message
        std::cout << "[client] received PING\n";
        std::cout << "        ping.type:" << unsigned (ping.type) << "\n";
        std::cout << "        ping.token:" << ping.token << "\n";
        std::cout << "        ping.client_time:" << ping.client_time << "\n";
        std::cout << "        ping.server_time:" << ping.server_time << "\n";
        std::cout << "        ping_valid:" << std::boolalpha << ping_valid << "\n";

    } else {
        std::cout << "[client] received unknown message\n";
    }

    /*

    // create a BANK_LIST_REQUEST message
    BANK_LIST_REQUEST bank_list_request;

    // pack the BANK_LIST_REQUEST message
    msg = MSG{MSG_ID::BANK_LIST_REQUEST, msgpack::object(bank_list_request, z)};

    // send the BANK_LIST_REQUEST message
    std::stringstream buffer;
    msgpack::pack(buffer, msg);
    const std::string payload = buffer.str();
    sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
    std::cout << "[client] sent BANK_LIST_REQUEST\n";

    // receive a message
    zmq::message_t message;
    (void) sock.recv(message);

    // parse the message
    msg = MSG{};
    msgpack::unpacked result;
    std::stringstream sbuf;
    sbuf << message.to_string();
    std::size_t off = 0;
    msgpack::unpack(result, sbuf.str().data(), sbuf.str().size(), off);
    result.get().convert(msg);

    // handle the BANK_LIST_RESPONSE message
    if (msg.id == MSG_ID::BANK_LIST_RESPONSE) {

        // parse the BANK_LIST_RESPONSE
        BANK_LIST_RESPONSE bank_list_response;
        msg.msg.convert(bank_list_response);

        // print the BANK_LIST_RESPONSE
        std::cout << "[client] received BANK_LIST_RESPONSE\n";
        for (const auto &bank: bank_list_response.banks) {
            std::cout << "        bank.id:" << bank.id << ", " << "bank.name:" << bank.name << "\n";
        }

    } else {
        std::cout << "[client] received unknown message\n";
    }

    */

    // program ends as expected
    return 0;
}
