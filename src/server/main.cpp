#include <iostream>
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include <msgpack.hpp>
#include <sqlite3.h>
#include "../messages.h"

int main(int argc, char *argv[]) {

    // create a msgpack zone
    msgpack::zone z;
    MSG msg;

    // create database handler
    sqlite3 *db;

    // open database banking.sqlite located in the same directory as the executable
    if (sqlite3_open("/home/m/study/banking/banking.sqlite", &db)) {
        std::cout << "[server] can not open database: " << sqlite3_errmsg(db) << "\n";
        return (1);
    } else {
        std::cout << "[server] opened database successfully\n";
    }

    // create a zmq context and socket
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::rep);
    sock.bind("tcp://127.0.0.1:2609");

    // wait for a second for ZMQ to properly initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // infinite loop
    while (true) {

        // receive a message
        zmq::message_t message;
        if (sock.recv(message, zmq::recv_flags::dontwait)) {
            std::cout << "[server] received message\n";
        } else {
            std::cout << "[server] no message received\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        // try to parse the message
        try {

            // parse the message
            msg = MSG{};
            msgpack::unpacked result;
            std::stringstream sbuf;
            sbuf << message.to_string();
            std::size_t off = 0;
            msgpack::unpack(result, sbuf.str().data(), sbuf.str().size(), off);
            result.get().convert(msg);

            // handle the BANK_LIST_REQUEST message
            if (msg.id == MSG_ID::BANK_LIST_REQUEST) {

                // parse the BANK_LIST_REQUEST
                BANK_LIST_REQUEST bank_list_request;
                msg.msg.convert(bank_list_request);
                std::cout << "[server] got BANK_LIST_REQUEST\n";

                // get the banks from the database
                sqlite3_stmt *stmt;
                std::string sql = "SELECT id, name FROM banks";
                if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                    std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                    continue;
                }

                // create a BANK_LIST_RESPONSE
                BANK_LIST_RESPONSE bank_list_response;
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    Bank bank{};
                    bank.id = sqlite3_column_int(stmt, 0);
                    bank.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                    bank_list_response.banks.push_back(bank);
                }
                sqlite3_finalize(stmt);

                // pack the BANK_LIST_RESPONSE
                msg = MSG{MSG_ID::BANK_LIST_RESPONSE, msgpack::object(bank_list_response, z)};

                // send the BANK_LIST_RESPONSE
                std::stringstream buffer;
                msgpack::pack(buffer, msg);
                const std::string payload = buffer.str();
                sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
                std::cout << " [server] sent BANK_LIST_RESPONSE\n";

            }
        } catch (const std::exception &e) {
            std::cout << "[server] " << e.what() << "\n";
        }
    }

    // program ends as expected
    return 0;
}
