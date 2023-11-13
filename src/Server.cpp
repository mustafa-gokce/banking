#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <msgpack.hpp>
#include "Server.h"
#include "Tools.h"

namespace Server {

    Server::Server() {
        std::cout << "[server] server created." << std::endl;
    }

    Server::~Server() {
        terminate();
        std::cout << "[server] server destroyed." << std::endl;
    }

    bool Server::initialize(const std::string &address) {
        _address = address;

        // open database banking.sqlite located in the same directory as the executable
        if (sqlite3_open("banking.sqlite", &_db)) {
            std::cout << "[server] can not open database: " << sqlite3_errmsg(_db) << std::endl;
            return false;
        } else {
            std::cout << "[server] opened database successfully" << std::endl;
        }

        // create a zmq context and socket
        _sock = zmq::socket_t(_ctx, ZMQ_REP);
        _sock.bind(_address);

        // wait for a second for ZMQ to properly initialize
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // check if the socket is properly bound
        if (_sock.handle() != nullptr) {
            std::cout << "[server] listening on " << _address << std::endl;
        } else {
            std::cout << "[server] can not listen on " << _address << std::endl;
            return false;
        }

        return true;
    }

    void Server::terminate() {
        if (_sock) {
            _sock.close();
            std::cout << "[client] socket connection closed" << std::endl;
        }
        if (_ctx.handle() != nullptr) {
            _ctx.close();
            std::cout << "[client] socket context closed" << std::endl;
        }
    }

    void Server::_send_message() {
        std::stringstream buffer;
        msgpack::pack(buffer, _msg);
        const std::string payload = buffer.str();
        _sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
    }

    bool Server::_receive_message() {

        // receive a message
        zmq::message_t message;
        if (!_sock.recv(message, zmq::recv_flags::dontwait)) {
            return false;
        }

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

        return true;
    }

    void Server::_send_ping(PING &ping) {

        // pack the PING message
        _msg = MSG{MSG_ID::PING, msgpack::object(ping, _z)};

        // send the PING message
        _send_message();
    }

    void Server::_handle_ping(PING &ping) {

        // prepare the response PING
        ping.type = PING_TYPE::SERVER;
        ping.server_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

        // send the PING message
        _send_ping(ping);
    }

    void Server::_send_login_response(LOGIN_RESPONSE &login_response) {

        // pack the LOGIN_RESPONSE message
        _msg = MSG{MSG_ID::LOGIN_RESPONSE, msgpack::object(login_response, _z)};

        // send the LOGIN_RESPONSE message
        _send_message();
    }

    void Server::_handle_login_request(const LOGIN_REQUEST &login_request, LOGIN_RESPONSE &login_response) {

        // get the user from the database
        sqlite3_stmt *stmt;
        std::string sql = "SELECT id, citizen, name, user FROM users WHERE user = ? AND pass = ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            login_response.type = LOGIN_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }

        // check if the user exists
        sqlite3_bind_text(stmt, 1, login_request.user.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, login_request.pass.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            login_response.type = LOGIN_RESPONSE_TYPE::INVALID_USERNAME_OR_PASSWORD;
            std::cout << "[server] user not found" << std::endl;
            return;
        }

        // fill the LOGIN_RESPONSE
        login_response.id = sqlite3_column_int(stmt, 0);
        login_response.citizen = sqlite3_column_int(stmt, 1);
        login_response.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        login_response.user = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        sqlite3_finalize(stmt);

        // get IBANs from the database for the user
        sql = "SELECT iban FROM accounts WHERE user = ? AND bank = ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            login_response.type = LOGIN_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }

        // check user has at least one account in the bank
        sqlite3_bind_int(stmt, 1, (int) login_response.id);
        sqlite3_bind_int(stmt, 2, login_request.bank);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            login_response.type = LOGIN_RESPONSE_TYPE::INVALID_BANK_ID;
            std::cout << "[server] user has no accounts in the bank" << std::endl;
            return;
        }
        sqlite3_finalize(stmt);

        // fill the LOGIN_RESPONSE
        login_response.bank = login_request.bank;

        // check if the user has already logged in using std::any_of algorithm
        if (std::any_of(_user_sessions.begin(), _user_sessions.end(),
                        [&login_response](const LOGIN_RESPONSE &login_response_) {
                            return login_response_.user == login_response.user;
                        })) {
            login_response.type = LOGIN_RESPONSE_TYPE::ALREADY_LOGGED_IN;
            std::cout << "[server] user has already logged in" << std::endl;
            return;
        }

        // fill the LOGIN_RESPONSE
        login_response.type = LOGIN_RESPONSE_TYPE::LOGIN_SUCCESS;
        login_response.token = Tools::Tools::random_string(32);

        // add the LOGIN_RESPONSE to the list of login responses
        _user_sessions.push_back(login_response);
        std::cout << "[server] user " << login_response.user << " logged in successfully" << std::endl;
    }

    void Server::_send_logout_response(LOGOUT_RESPONSE &logout_response) {

        // pack the LOGOUT_RESPONSE message
        _msg = MSG{MSG_ID::LOGOUT_RESPONSE, msgpack::object(logout_response, _z)};

        // send the LOGOUT_RESPONSE message
        _send_message();
    }

    void Server::_handle_logout_request(const LOGOUT_REQUEST &logout_request) {

        // create a LOGOUT_RESPONSE
        LOGOUT_RESPONSE logout_response;
        logout_response.type = LOGOUT_RESPONSE_TYPE::LOGOUT_SUCCESS;

        // check if the user has already logged in
        int token_index = -1;
        for (int i = 0; i < _user_sessions.size(); i++) {
            if (_user_sessions[i].user == logout_request.user) {
                token_index = i;
                break;
            }
        }
        if (token_index == -1) {
            logout_response.type = LOGOUT_RESPONSE_TYPE::NOT_LOGGED_IN;
            std::cout << "[server] user has not logged in" << std::endl;
        }

        // check if token is valid
        if (logout_response.type == LOGOUT_RESPONSE_TYPE::LOGOUT_SUCCESS) {
            if (_user_sessions[token_index].token != logout_request.token) {
                logout_response.type = LOGOUT_RESPONSE_TYPE::INVALID_TOKEN;
                std::cout << "[server] invalid token" << std::endl;
            }
        }

        // remove the LOGIN_RESPONSE from the list of login responses
        if (logout_response.type == LOGOUT_RESPONSE_TYPE::LOGOUT_SUCCESS) {
            std::cout << "[server] user " << logout_request.user << " logged out successfully" << std::endl;
            _user_sessions.erase(_user_sessions.begin() + token_index);
        }

        // send the LOGOUT_RESPONSE
        _send_logout_response(logout_response);
        std::cout << "[server] sent LOGOUT_RESPONSE" << std::endl;
    }

    void Server::_send_bank_list_response(BANK_LIST_RESPONSE &bank_list_response) {

        // pack the BANK_LIST_RESPONSE message
        _msg = MSG{MSG_ID::BANK_LIST_RESPONSE, msgpack::object(bank_list_response, _z)};

        // send the BANK_LIST_RESPONSE message
        _send_message();
    }

    void Server::_handle_bank_list_request([[maybe_unused]]BANK_LIST_REQUEST &bank_list_request) {

        // create a BANK_LIST_RESPONSE
        BANK_LIST_RESPONSE bank_list_response;

        // get the banks from the database
        bool error = false;
        sqlite3_stmt *stmt;
        std::string sql = "SELECT id, name FROM banks";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            error = true;
        }

        // fill the BANK_LIST_RESPONSE
        if (!error){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                Bank bank{};
                bank.id = sqlite3_column_int(stmt, 0);
                bank.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                bank_list_response.banks.push_back(bank);
            }
        }
        sqlite3_finalize(stmt);

        // send the BANK_LIST_RESPONSE
        _send_bank_list_response(bank_list_response);
        std::cout << "[server] sent BANK_LIST_RESPONSE" << std::endl;
    }

    void Server::_send_account_list_response(ACCOUNT_LIST_RESPONSE &account_list_response) {

        // pack the ACCOUNT_LIST_RESPONSE message
        _msg = MSG{MSG_ID::ACCOUNT_LIST_RESPONSE, msgpack::object(account_list_response, _z)};

        // send the ACCOUNT_LIST_RESPONSE message
        _send_message();
    }

    void Server::_handle_account_list_request(ACCOUNT_LIST_REQUEST &account_list_request) {

        // create a ACCOUNT_LIST_RESPONSE
        ACCOUNT_LIST_RESPONSE account_list_response;

        // check if the user has already logged in and the token is valid
        int token_index = -1;
        for (int i = 0; i < _user_sessions.size(); i++) {
            if (_user_sessions[i].id == account_list_request.user) {
                if (_user_sessions[i].token == account_list_request.token) {
                    token_index = i;
                }
                break;
            }
        }

        // check if the user has already logged in
        if (token_index != -1) {

            // get the accounts from the database
            bool error = false;
            sqlite3_stmt *stmt;
            std::string sql = "SELECT iban, user, bank, balance FROM accounts WHERE user = ? AND bank = ?";
            if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
                error = true;
            }

            // fill the ACCOUNT_LIST_RESPONSE
            if (!error) {
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
            }
            sqlite3_finalize(stmt);
        }

        // send the ACCOUNT_LIST_RESPONSE
        _send_account_list_response(account_list_response);
        std::cout << "[server] sent ACCOUNT_LIST_RESPONSE" << std::endl;
    }

    void Server::_send_add_balance_response(ADD_BALANCE_RESPONSE &add_balance_response) {

        // pack the ADD_BALANCE_RESPONSE message
        _msg = MSG{MSG_ID::ADD_BALANCE_RESPONSE, msgpack::object(add_balance_response, _z)};

        // send the ADD_BALANCE_RESPONSE message
        _send_message();
    }

    void Server::_handle_add_balance_request(ADD_BALANCE_REQUEST &add_balance_request,
                                             ADD_BALANCE_RESPONSE &add_balance_response) {

        // check if the user has already logged in and the token is valid
        int user_index = -1;
        bool valid_token;
        for (int i = 0; i < _user_sessions.size(); i++) {
            if (_user_sessions[i].id == add_balance_request.user) {
                user_index = i;
                valid_token = (_user_sessions[i].token == add_balance_request.token);
                break;
            }
        }

        // check if the user has already logged in
        if (user_index == -1) {
            std::cout << "[server] user has not logged in" << std::endl;
            return;
        }

        // check if the token is valid
        if (!valid_token) {
            std::cout << "[server] invalid token" << std::endl;
            return;
        }

        // fill the ADD_BALANCE_RESPONSE
        add_balance_response.user = add_balance_request.user;
        add_balance_response.token = add_balance_request.token;
        add_balance_response.bank = add_balance_request.bank;
        add_balance_response.iban = add_balance_request.iban;

        // add the balance to the account
        sqlite3_stmt *stmt;
        std::string sql = "UPDATE accounts SET balance = balance + ? WHERE iban = ? AND user = ? AND bank = ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_bind_double(stmt, 1, add_balance_request.amount);
        sqlite3_bind_text(stmt, 2, add_balance_request.iban.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, (int) add_balance_request.user);
        sqlite3_bind_int(stmt, 4, add_balance_request.bank);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cout << "[server] can not update balance: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_finalize(stmt);

        // get the new balance from the database
        sql = "SELECT balance FROM accounts WHERE iban = ? AND user = ? AND bank = ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_bind_text(stmt, 1, add_balance_request.iban.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, (int) add_balance_request.user);
        sqlite3_bind_int(stmt, 3, add_balance_request.bank);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            std::cout << "[server] can not get balance: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        add_balance_response.amount = sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
    }

    void Server::_send_transaction_response(TRANSACTION_RESPONSE &transaction_response) {

        // pack the TRANSACTION_RESPONSE message
        _msg = MSG{MSG_ID::TRANSACTION_RESPONSE, msgpack::object(transaction_response, _z)};

        // send the TRANSACTION_RESPONSE message
        _send_message();
    }

    void Server::_handle_transaction_request(TRANSACTION_REQUEST &transaction_request,
                                             TRANSACTION_RESPONSE &transaction_response) {

        // check if the user has already logged in and the token is valid
        int user_index = -1;
        bool valid_token;
        for (int i = 0; i < _user_sessions.size(); i++) {
            if (_user_sessions[i].id == transaction_request.user) {
                user_index = i;
                valid_token = (_user_sessions[i].token == transaction_request.token);
                break;
            }
        }

        // check if the user has already logged in
        if (user_index == -1) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::NOT_LOGGED_IN;
            std::cout << "[server] user has not logged in" << std::endl;
            return;
        }

        // check if the token is valid
        if (!valid_token) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_TOKEN;
            std::cout << "[server] invalid token" << std::endl;
            return;
        }

        // check if amount is positive
        if (transaction_response.type == TRANSACTION_RESPONSE_TYPE::TRANSACTION_SUCCESS) {
            if (transaction_request.amount <= 0) {
                std::cout << "[server] amount is not positive" << std::endl;
                transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_AMOUNT;
            }
        }

        // SQL statements
        sqlite3_stmt *stmt;
        std::string sql;

        // check if from account exists
        sql = "SELECT balance FROM accounts WHERE iban = ? AND user = ? AND bank = ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_bind_text(stmt, 1, transaction_request.from.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, (int) transaction_request.user);
        sqlite3_bind_int(stmt, 3, transaction_request.bank);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_FROM_IBAN;
            std::cout << "[server] from account not found: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_finalize(stmt);

        // check if to account exists
        sql= "SELECT balance FROM accounts WHERE iban = ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_bind_text(stmt, 1, transaction_request.to.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_TO_IBAN;
            std::cout << "[server] can not get balance: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_finalize(stmt);

        // apply fee if from account and to account are in the same bank
        float_t fee = 0.0;
        uint16_t bank_from, bank_to;

        // get from bank
        sql = "SELECT bank FROM accounts WHERE iban = ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_bind_text(stmt, 1, transaction_request.from.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_FROM_IBAN;
            std::cout << "[server] can not get from bank: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        bank_from = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        // get to bank
        sql = "SELECT bank FROM accounts WHERE iban = ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_bind_text(stmt, 1, transaction_request.to.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_TO_IBAN;
            std::cout << "[server] can not get to bank: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        bank_to = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        // from account and to account are not in the same bank
        if (bank_from != bank_to) {

            // get the fee from the database
            sql = "SELECT fee FROM banks WHERE id = ?";
            if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
                return;
            }
            sqlite3_bind_int(stmt, 1, bank_from);
            if (sqlite3_step(stmt) != SQLITE_ROW) {
                transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
                std::cout << "[server] can not get fee: " << sqlite3_errmsg(_db) << std::endl;
                return;
            }
            fee = (float_t) sqlite3_column_double(stmt, 0);
            sqlite3_finalize(stmt);
        }

        // fill the TRANSACTION_RESPONSE
        transaction_response.fee = fee;

        // check if from account has enough balance
        sql = "SELECT balance FROM accounts WHERE iban = ? AND user = ? AND bank = ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_bind_text(stmt, 1, transaction_request.from.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, (int) transaction_request.user);
        sqlite3_bind_int(stmt, 3, transaction_request.bank);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::INVALID_FROM_IBAN;
            std::cout << "[server] from account not found: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        if (sqlite3_column_double(stmt, 0) < transaction_request.amount + fee) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::INSUFFICIENT_FUNDS;
            std::cout << "[server] insufficient funds" << std::endl;
            return;
        }
        sqlite3_finalize(stmt);

        // update the balance of from account
        sql = "UPDATE accounts SET balance = balance - ? WHERE iban = ? AND user = ? AND bank = ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_bind_double(stmt, 1, transaction_request.amount + fee);
        sqlite3_bind_text(stmt, 2, transaction_request.from.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, (int) transaction_request.user);
        sqlite3_bind_int(stmt, 4, transaction_request.bank);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not update balance: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_finalize(stmt);

        // update the balance of the to account
        sql = "UPDATE accounts SET balance = balance + ? WHERE iban = ?";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_bind_double(stmt, 1, transaction_request.amount);
        sqlite3_bind_text(stmt, 2, transaction_request.to.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not update balance: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_finalize(stmt);

        // fill the TRANSACTION_RESPONSE
        transaction_response.token = Tools::Tools::random_string(32);

        // add the transaction to the database
        sql = "INSERT INTO transactions (token, source, destination, amount, fee) VALUES (?, ?, ?, ?, ?)";
        if (sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not prepare statement: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_bind_text(stmt, 1, transaction_response.token.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, transaction_request.from.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, transaction_request.to.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 4, transaction_request.amount);
        sqlite3_bind_double(stmt, 5, fee);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            transaction_response.type = TRANSACTION_RESPONSE_TYPE::SERVER_ERROR;
            std::cout << "[server] can not insert transaction: " << sqlite3_errmsg(_db) << std::endl;
            return;
        }
        sqlite3_finalize(stmt);

        // fill the TRANSACTION_RESPONSE
        transaction_response.type = TRANSACTION_RESPONSE_TYPE::TRANSACTION_SUCCESS;
    }

    bool Server::handle_request() {

        // receive a message
        if (!_receive_message()) {
            return false;
        }

        // handle the message
        switch (_msg.id) {

            case MSG_ID::NONE: {
                std::cout << "[server] got NONE" << std::endl;
                break;
            }

            case MSG_ID::PING: {
                std::cout << "[server] got PING" << std::endl;

                // parse the PING message
                PING ping;
                _msg.msg.convert(ping);

                // handle the PING message
                _handle_ping(ping);

                break;
            }

            case MSG_ID::LOGIN_REQUEST: {
                std::cout << "[server] got LOGIN_REQUEST" << std::endl;

                // parse the LOGIN_REQUEST message
                LOGIN_REQUEST login_request;
                _msg.msg.convert(login_request);

                // handle the LOGIN_REQUEST message
                LOGIN_RESPONSE login_response;
                _handle_login_request(login_request, login_response);

                // send the LOGIN_RESPONSE
                _send_login_response(login_response);
                std::cout << "[server] sent LOGIN_RESPONSE" << std::endl;

                break;
            }

            case MSG_ID::LOGOUT_REQUEST: {
                std::cout << "[server] got LOGOUT_REQUEST" << std::endl;

                // parse the LOGOUT_REQUEST message
                LOGOUT_REQUEST logout_request;
                _msg.msg.convert(logout_request);

                // handle the LOGOUT_REQUEST message
                _handle_logout_request(logout_request);

                break;
            }

            case MSG_ID::BANK_LIST_REQUEST: {
                std::cout << "[server] got BANK_LIST_REQUEST" << std::endl;

                // parse the BANK_LIST_REQUEST message
                BANK_LIST_REQUEST bank_list_request;
                _msg.msg.convert(bank_list_request);

                // handle the BANK_LIST_REQUEST message
                _handle_bank_list_request(bank_list_request);

                break;
            }

            case MSG_ID::ACCOUNT_LIST_REQUEST: {
                std::cout << "[server] got ACCOUNT_LIST_REQUEST" << std::endl;

                // parse the ACCOUNT_LIST_REQUEST message
                ACCOUNT_LIST_REQUEST account_list_request;
                _msg.msg.convert(account_list_request);

                // handle the ACCOUNT_LIST_REQUEST message
                _handle_account_list_request(account_list_request);

                break;
            }

            case MSG_ID::ADD_BALANCE_REQUEST: {
                std::cout << "[server] got ADD_BALANCE_REQUEST" << std::endl;

                // parse the ADD_BALANCE_REQUEST message
                ADD_BALANCE_REQUEST add_balance_request;
                _msg.msg.convert(add_balance_request);

                // handle the ADD_BALANCE_REQUEST message
                ADD_BALANCE_RESPONSE add_balance_response;
                _handle_add_balance_request(add_balance_request, add_balance_response);
                _send_add_balance_response(add_balance_response);
                std::cout << "[server] sent ADD_BALANCE_RESPONSE" << std::endl;

                break;
            }

            case MSG_ID::TRANSACTION_REQUEST: {
                std::cout << "[server] got TRANSACTION_REQUEST" << std::endl;

                // parse the TRANSACTION_REQUEST message
                TRANSACTION_REQUEST transaction_request;
                _msg.msg.convert(transaction_request);

                // handle the TRANSACTION_REQUEST message
                TRANSACTION_RESPONSE transaction_response;
                _handle_transaction_request(transaction_request, transaction_response);
                _send_transaction_response(transaction_response);
                std::cout << "[server] sent TRANSACTION_RESPONSE" << std::endl;

                break;
            }

            default: {
                std::cout << "[server] got unknown message" << std::endl;
                break;
            }
        }
        return true;
    }
} // Server