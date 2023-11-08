#include <random>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include <msgpack.hpp>
#include <sqlite3.h>
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

    // create a msgpack zone
    msgpack::zone z;
    MSG msg;

    // create database handler
    sqlite3 *db;

    // open database banking.sqlite located in the same directory as the executable
    if (sqlite3_open("banking.sqlite", &db)) {
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

    // hold login response messages for each client
    std::vector<LOGIN_RESPONSE> login_responses;

    // infinite loop
    while (true) {

        // receive a message
        zmq::message_t message;
        if (sock.recv(message, zmq::recv_flags::dontwait)) {
            std::cout << "[server] received a message\n";
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

            // handle the PING message
            if (msg.id == MSG_ID::PING) {

                // parse the PING message
                PING ping;
                msg.msg.convert(ping);
                std::cout << "[server] got PING\n";

                // prepare the response PING
                ping.type = PING_TYPE::SERVER;
                ping.server_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();

                // pack the PING message
                msg = MSG{MSG_ID::PING, msgpack::object(ping, z)};

                // send the PING message
                std::stringstream buffer;
                msgpack::pack(buffer, msg);
                const std::string payload = buffer.str();
                sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
                std::cout << "[server] sent PING\n";
            }

            // handle the LOGIN_REQUEST message
            else if (msg.id == MSG_ID::LOGIN_REQUEST) {

                // parse the LOGIN_REQUEST
                LOGIN_REQUEST login_request;
                msg.msg.convert(login_request);
                std::cout << "[server] got LOGIN_REQUEST\n";

                // create a LOGIN_RESPONSE
                LOGIN_RESPONSE login_response;
                login_response.type = LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS;

                // get the user from the database
                sqlite3_stmt *stmt;
                std::string sql = "SELECT id, citizen, name, user FROM users WHERE user = ? AND pass = ?";
                if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                    std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                    if (login_response.type == LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS){
                        login_response.type = LOGIN_RESPONSE_TYPE::SERVER_ERROR;
                    }
                }

                // check if the user exists
                sqlite3_bind_text(stmt, 1, login_request.user.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, login_request.pass.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) != SQLITE_ROW) {
                    std::cout << "[server] user not found\n";
                    if (login_response.type == LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS) {
                        login_response.type = LOGIN_RESPONSE_TYPE::INVALID_USERNAME_OR_PASSWORD;
                    }
                }

                // fill the LOGIN_RESPONSE
                if (login_response.type == LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS) {
                    login_response.id = sqlite3_column_int(stmt, 0);
                    login_response.citizen = sqlite3_column_int(stmt, 1);
                    login_response.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
                    login_response.user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
                }
                sqlite3_finalize(stmt);

                // get ibans from the database for the user
                sql = "SELECT iban FROM accounts WHERE user = ? AND bank = ?";
                if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                    std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                    if (login_response.type == LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS){
                        login_response.type = LOGIN_RESPONSE_TYPE::SERVER_ERROR;
                    }
                }

                // check user has at least one account in the bank
                sqlite3_bind_int(stmt, 1, (int) login_response.id);
                sqlite3_bind_int(stmt, 2, login_request.bank);
                if (sqlite3_step(stmt) != SQLITE_ROW) {
                    std::cout << "[server] user has no accounts in the bank\n";
                    if (login_response.type == LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS) {
                        login_response.type = LOGIN_RESPONSE_TYPE::INVALID_BANK_ID;
                    }
                }
                sqlite3_finalize(stmt);

                // fill the LOGIN_RESPONSE
                if (login_response.type == LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS) {
                    login_response.bank = login_request.bank;
                }

                // check if the user has already logged in
                for (const auto &response: login_responses) {
                    if (response.id == login_response.id) {
                        std::cout << "[server] user has already logged in\n";
                        if (login_response.type == LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS) {
                            login_response.type = LOGIN_RESPONSE_TYPE::ALREADY_LOGGED_IN;
                        }
                        break;
                    }
                }

                // check user login is success
                if (login_response.type == LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS) {

                    // generate a random token
                    login_response.token = random_string(32);

                    // add the LOGIN_RESPONSE to the list of login responses
                    login_responses.push_back(login_response);
                }

                // pack the LOGIN_RESPONSE
                msg = MSG{MSG_ID::LOGIN_RESPONSE, msgpack::object(login_response, z)};

                // send the LOGIN_RESPONSE
                std::stringstream buffer;
                msgpack::pack(buffer, msg);
                const std::string payload = buffer.str();
                sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
                std::cout << "[server] sent LOGIN_RESPONSE\n";

            }

            // handle the BANK_LIST_REQUEST message
            else if (msg.id == MSG_ID::BANK_LIST_REQUEST) {

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
                std::cout << "[server] sent BANK_LIST_RESPONSE\n";

            }
        } catch (const std::exception &e) {
            std::cout << "[server] " << e.what() << "\n";
        }
    }

    // program ends as expected
    return 0;
}
