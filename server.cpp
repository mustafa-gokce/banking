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
    std::vector<LOGIN_RESPONSE> user_sessions;

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
                    if (login_response.type == LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS) {
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
                    if (login_response.type == LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS) {
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
                for (const auto &response: user_sessions) {
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
                    user_sessions.push_back(login_response);
                    std::cout << "[server] user << " << login_response.user << " logged in successfully\n";
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

            // handle the LOGOUT_REQUEST message
            else if (msg.id == MSG_ID::LOGOUT_REQUEST) {

                // parse the LOGOUT_REQUEST
                LOGOUT_REQUEST logout_request;
                msg.msg.convert(logout_request);
                std::cout << "[server] got LOGOUT_REQUEST\n";

                // create a LOGOUT_RESPONSE
                LOGOUT_RESPONSE logout_response;
                logout_response.type = LOGOUT_RESPONSE_TYPE::LOGOUT_SUCCESS;

                // check if the user has already logged in
                int token_index = -1;
                for (int i = 0; i < user_sessions.size(); i++) {
                    if (user_sessions[i].user == logout_request.user) {
                        token_index = i;
                        break;
                    }
                }
                if (token_index == -1) {
                    std::cout << "[server] user has not logged in\n";
                    if (logout_response.type == LOGOUT_RESPONSE_TYPE::LOGOUT_SUCCESS) {
                        logout_response.type = LOGOUT_RESPONSE_TYPE::NOT_LOGGED_IN;
                    }
                }

                // check if token is valid
                if (logout_response.type == LOGOUT_RESPONSE_TYPE::LOGOUT_SUCCESS) {
                    if (user_sessions[token_index].token != logout_request.token) {
                        std::cout << "[server] invalid token\n";
                        if (logout_response.type == LOGOUT_RESPONSE_TYPE::LOGOUT_SUCCESS) {
                            logout_response.type = LOGOUT_RESPONSE_TYPE::INVALID_TOKEN;
                        }
                    }
                }

                // remove the LOGIN_RESPONSE from the list of login responses
                if (logout_response.type == LOGOUT_RESPONSE_TYPE::LOGOUT_SUCCESS) {
                    std::cout << "[server] user " << logout_request.user << " logged out successfully\n";
                    user_sessions.erase(user_sessions.begin() + token_index);
                }

                // pack the LOGOUT_RESPONSE
                msg = MSG{MSG_ID::LOGOUT_RESPONSE, msgpack::object(logout_response, z)};

                // send the LOGOUT_RESPONSE
                std::stringstream buffer;
                msgpack::pack(buffer, msg);
                const std::string payload = buffer.str();
                sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
                std::cout << "[server] sent LOGOUT_RESPONSE\n";
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

            // handle the ACCOUNT_LIST_REQUEST message
            else if (msg.id == MSG_ID::ACCOUNT_LIST_REQUEST) {

                // parse the ACCOUNT_LIST_REQUEST
                ACCOUNT_LIST_REQUEST account_list_request;
                msg.msg.convert(account_list_request);
                std::cout << "[server] got ACCOUNT_LIST_REQUEST\n";

                // create a ACCOUNT_LIST_RESPONSE
                ACCOUNT_LIST_RESPONSE account_list_response;

                // check if the user has already logged in and the token is valid
                int token_index = -1;
                for (int i = 0; i < user_sessions.size(); i++) {
                    if (user_sessions[i].id == account_list_request.user) {
                        if (user_sessions[i].token == account_list_request.token) {
                            token_index = i;
                        }
                        break;
                    }
                }

                // check if the user has already logged in
                if (token_index != -1) {

                    // get the accounts from the database
                    sqlite3_stmt *stmt;
                    std::string sql = "SELECT iban, user, bank, balance FROM accounts WHERE user = ? AND bank = ?";
                    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                        std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                        continue;
                    }

                    // fill the ACCOUNT_LIST_RESPONSE
                    sqlite3_bind_int(stmt, 1, (int) account_list_request.user);
                    sqlite3_bind_int(stmt, 2, account_list_request.bank);
                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                        Account account{};
                        account.iban = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
                        account.user = sqlite3_column_int(stmt, 1);
                        account.bank = sqlite3_column_int(stmt, 2);
                        account.balance = sqlite3_column_double(stmt, 3);
                        account_list_response.accounts.push_back(account);
                    }
                    sqlite3_finalize(stmt);
                }

                // pack the ACCOUNT_LIST_RESPONSE
                msg = MSG{MSG_ID::ACCOUNT_LIST_RESPONSE, msgpack::object(account_list_response, z)};

                // send the ACCOUNT_LIST_RESPONSE
                std::stringstream buffer;
                msgpack::pack(buffer, msg);
                const std::string payload = buffer.str();
                sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
                std::cout << "[server] sent ACCOUNT_LIST_RESPONSE\n";
            }

            // handle the ADD_BALANCE_REQUEST message
            else if (msg.id == MSG_ID::ADD_BALANCE_REQUEST) {

                // parse the ADD_BALANCE_REQUEST
                ADD_BALANCE_REQUEST add_balance_request;
                msg.msg.convert(add_balance_request);
                std::cout << "[server] got ADD_BALANCE_REQUEST\n";

                // check if the user has already logged in and the token is valid
                int token_index = -1;
                for (int i = 0; i < user_sessions.size(); i++) {
                    if (user_sessions[i].id == add_balance_request.user) {
                        if (user_sessions[i].token == add_balance_request.token) {
                            token_index = i;
                        }
                        break;
                    }
                }

                // create an ADD_BALANCE_RESPONSE
                ADD_BALANCE_RESPONSE add_balance_response;

                // check if the user has already logged in
                if (token_index != -1) {

                    // fill the ADD_BALANCE_RESPONSE
                    add_balance_response.user = add_balance_request.user;
                    add_balance_response.token = add_balance_request.token;
                    add_balance_response.bank = add_balance_request.bank;
                    add_balance_response.iban = add_balance_request.iban;

                    // add the balance to the account
                    sqlite3_stmt *stmt;
                    std::string sql = "UPDATE accounts SET balance = balance + ? WHERE iban = ? AND user = ? AND bank = ?";
                    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                        std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                        continue;
                    }
                    sqlite3_bind_double(stmt, 1, add_balance_request.amount);
                    sqlite3_bind_text(stmt, 2, add_balance_request.iban.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_int(stmt, 3, (int) add_balance_request.user);
                    sqlite3_bind_int(stmt, 4, add_balance_request.bank);
                    if (sqlite3_step(stmt) != SQLITE_DONE) {
                        std::cout << "[server] can not update balance: " << sqlite3_errmsg(db) << "\n";
                    }
                    sqlite3_finalize(stmt);

                    // get the new balance from the database
                    sql = "SELECT balance FROM accounts WHERE iban = ? AND user = ? AND bank = ?";
                    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                        std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                        continue;
                    }
                    sqlite3_bind_text(stmt, 1, add_balance_request.iban.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_int(stmt, 2, (int) add_balance_request.user);
                    sqlite3_bind_int(stmt, 3, add_balance_request.bank);
                    if (sqlite3_step(stmt) != SQLITE_ROW) {
                        std::cout << "[server] can not get balance: " << sqlite3_errmsg(db) << "\n";
                    }
                    add_balance_response.amount = sqlite3_column_double(stmt, 0);
                    sqlite3_finalize(stmt);

                    // pack the ADD_BALANCE_RESPONSE
                    msg = MSG{MSG_ID::ADD_BALANCE_RESPONSE, msgpack::object(add_balance_response, z)};

                    // send the ADD_BALANCE_RESPONSE
                    std::stringstream buffer;
                    msgpack::pack(buffer, msg);
                    const std::string payload = buffer.str();
                    sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
                    std::cout << "[server] sent ADD_BALANCE_RESPONSE\n";
                }
            }

            // handle the TRANSACTION_REQUEST message
            else if (msg.id == MSG_ID::TRANSACTION_REQUEST) {

                // parse the TRANSACTION_REQUEST
                TRANSACTION_REQUEST transaction_request;
                msg.msg.convert(transaction_request);
                std::cout << "[server] got TRANSACTION_REQUEST\n";

                // create a TRANSACTION_RESPONSE
                TRANSACTION_RESPONSE transaction_response;
                transaction_response.type = TRANSACTION_RESPONSE_TYPE::NOT_LOGGED_IN;

                // check if the user has already logged in and the token is valid
                int user_index = -1;
                for (auto & user_session : user_sessions) {
                    if (user_session.id == transaction_request.user) {
                        if (user_session.token == transaction_request.token) {
                            transaction_response.type = TRANSACTION_RESPONSE_TYPE::TRANSACTION_SUCCESS;
                            break;
                        }
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_TOKEN;
                        break;
                    }
                }

                // check if amount is positive
                if (transaction_response.type == TRANSACTION_RESPONSE_TYPE::TRANSACTION_SUCCESS) {
                    if (transaction_request.amount <= 0) {
                        std::cout << "[server] amount is not positive\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_AMOUNT;
                    }
                }

                // check if from account exists
                if (transaction_response.type == TRANSACTION_RESPONSE_TYPE::TRANSACTION_SUCCESS) {
                    sqlite3_stmt *stmt;
                    std::string sql = "SELECT balance FROM accounts WHERE iban = ? AND user = ? AND bank = ?";
                    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                        std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                    }
                    sqlite3_bind_text(stmt, 1, transaction_request.from.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_int(stmt, 2, (int) transaction_request.user);
                    sqlite3_bind_int(stmt, 3, transaction_request.bank);
                    if (sqlite3_step(stmt) != SQLITE_ROW) {
                        std::cout << "[server] from account not found: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_FROM_IBAN;
                    }
                    sqlite3_finalize(stmt);
                }

                // check if to account exists
                if (transaction_response.type == TRANSACTION_RESPONSE_TYPE::TRANSACTION_SUCCESS) {
                    sqlite3_stmt *stmt;
                    std::string sql = "SELECT balance FROM accounts WHERE iban = ?";
                    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                        std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                    }
                    sqlite3_bind_text(stmt, 1, transaction_request.to.c_str(), -1, SQLITE_STATIC);
                    if (sqlite3_step(stmt) != SQLITE_ROW) {
                        std::cout << "[server] can not get balance: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_TO_IBAN;
                    }
                    sqlite3_finalize(stmt);
                }

                // if from account and to account are in the same bank or not apply fee
                float_t fee = 0.0;
                if (transaction_response.type == TRANSACTION_RESPONSE_TYPE::TRANSACTION_SUCCESS) {
                    uint16_t bank_from, bank_to;

                    // get from bank
                    sqlite3_stmt *stmt;
                    std::string sql = "SELECT bank FROM accounts WHERE iban = ?";
                    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                        std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                    }
                    sqlite3_bind_text(stmt, 1, transaction_request.from.c_str(), -1, SQLITE_STATIC);
                    if (sqlite3_step(stmt) != SQLITE_ROW) {
                        std::cout << "[server] can not get bank: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_FROM_IBAN;
                    }
                    bank_from = sqlite3_column_int(stmt, 0);
                    sqlite3_finalize(stmt);

                    // get to bank
                    sql = "SELECT bank FROM accounts WHERE iban = ?";
                    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                        std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                    }
                    sqlite3_bind_text(stmt, 1, transaction_request.to.c_str(), -1, SQLITE_STATIC);
                    if (sqlite3_step(stmt) != SQLITE_ROW) {
                        std::cout << "[server] can not get bank: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_TO_IBAN;
                    }
                    bank_from = sqlite3_column_int(stmt, 0);
                    sqlite3_finalize(stmt);

                    // from account and to account are not in the same bank
                    if (bank_from != bank_to) {

                        // get the fee from the database
                        sql = "SELECT fee FROM banks WHERE id = ?";
                        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                        }
                        sqlite3_bind_int(stmt, 1, bank_from);
                        if (sqlite3_step(stmt) != SQLITE_ROW) {
                            std::cout << "[server] can not get fee: " << sqlite3_errmsg(db) << "\n";
                            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                        }
                        fee = (float_t) sqlite3_column_double(stmt, 0);
                        sqlite3_finalize(stmt);
                    }
                }

                // check if from account has enough balance
                if (transaction_response.type == TRANSACTION_RESPONSE_TYPE::TRANSACTION_SUCCESS) {
                    sqlite3_stmt *stmt;
                    std::string sql = "SELECT balance FROM accounts WHERE iban = ? AND user = ? AND bank = ?";
                    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                        std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                    }
                    sqlite3_bind_text(stmt, 1, transaction_request.from.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_int(stmt, 2, (int) transaction_request.user);
                    sqlite3_bind_int(stmt, 3, transaction_request.bank);
                    if (sqlite3_step(stmt) != SQLITE_ROW) {
                        std::cout << "[server] from account not found: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_FROM_IBAN;
                    }
                    if (sqlite3_column_double(stmt, 0) < transaction_request.amount + fee) {
                        std::cout << "[server] insufficient funds\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::INSUFFICIENT_FUNDS;
                    }
                    sqlite3_finalize(stmt);
                }

                // if transaction is success update the balance of the accounts
                if (transaction_response.type == TRANSACTION_RESPONSE_TYPE::TRANSACTION_SUCCESS) {

                    // update the balance of from account
                    sqlite3_stmt *stmt;
                    std::string sql = "UPDATE accounts SET balance = balance - ? WHERE iban = ? AND user = ? AND bank = ?";
                    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                        std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                    }
                    sqlite3_bind_double(stmt, 1, transaction_request.amount + fee);
                    sqlite3_bind_text(stmt, 2, transaction_request.from.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_int(stmt, 3, (int) transaction_request.user);
                    sqlite3_bind_int(stmt, 4, transaction_request.bank);
                    if (sqlite3_step(stmt) != SQLITE_DONE) {
                        std::cout << "[server] can not update balance: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                    }
                    sqlite3_finalize(stmt);

                    // update the balance of the to account
                    sql = "UPDATE accounts SET balance = balance + ? WHERE iban = ?";
                    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
                    {
                        std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                    }
                    sqlite3_bind_double(stmt, 1, transaction_request.amount);
                    sqlite3_bind_text(stmt, 2, transaction_request.to.c_str(), -1, SQLITE_STATIC);
                    if (sqlite3_step(stmt) != SQLITE_DONE) {
                        std::cout << "[server] can not update balance: " << sqlite3_errmsg(db) << "\n";
                        transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                    }
                    sqlite3_finalize(stmt);

                    // fill the TRANSACTION_RESPONSE
                    transaction_response.fee = fee;
                    transaction_response.token = random_string(32);
                }

                // pack the TRANSACTION_RESPONSE
                msg = MSG{MSG_ID::TRANSACTION_RESPONSE, msgpack::object(transaction_response, z)};

                // send the TRANSACTION_RESPONSE
                std::stringstream buffer;
                msgpack::pack(buffer, msg);
                const std::string payload = buffer.str();
                sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
                std::cout << "[server] sent TRANSACTION_RESPONSE\n";
            }

        } catch (const std::exception &e) {
            std::cout << "[server] " << e.what() << "\n";
        }
    }

    // program ends as expected
    return 0;
}
