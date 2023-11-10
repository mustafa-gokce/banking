#include <QApplication>
#include <QPushButton>
#include <iostream>
#include "src/Messages.h"
#include "src/Tools.h"
#include "src/Client.h"

int main(int argc, char *argv[]) {
    /*
    QApplication a(argc, argv);
    QPushButton button("Hello world!", nullptr);
    button.resize(200, 100);
    button.show();
    return QApplication::exec();
     */

    // create client
    Client::Client client;

    // initialize client (connect to server)
    client.initialize("tcp://127.0.0.1:2609");

    // send ping
    client.send_ping();

    // receive ping
    client.receive_ping();

    // send bank list request
    client.send_bank_list_request();

    // receive bank list response
    client.receive_bank_list_response();

    // send login request
    client.send_login_request("aylin.yilmaz42@gmail.com", "G3l!y@m!zP@ss", 1);

    // receive login response (we need to create a LOGIN_RESPONSE object to hold login token)
    LOGIN_RESPONSE login_response;
    client.receive_login_response(login_response);

    // send logout request
    client.send_logout_request(login_response.user, login_response.token);

    // receive logout response
    client.receive_logout_response();

    /*

    // create a TRANSACTION_REQUEST message
    TRANSACTION_REQUEST transaction_request;
    transaction_request.user = 1;
    transaction_request.token = "KynVSJrWp3BLAlytnlVSOaTVa6zOUrup";
    transaction_request.bank = 1;
    transaction_request.from = "TR6136876433976928";
    transaction_request.to = "TR4721031995319313";
    transaction_request.amount = (double_t) 1000;

    // pack the TRANSACTION_REQUEST message
    msg = MSG{MSG_ID::TRANSACTION_REQUEST, msgpack::object(transaction_request, z)};

    // send the TRANSACTION_REQUEST message
    std::stringstream buffer;
    msgpack::pack(buffer, msg);
    const std::string payload = buffer.str();
    sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
    std::cout << "[client] sent TRANSACTION_REQUEST\n";

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

    // handle the TRANSACTION_RESPONSE message
    if (msg.id == MSG_ID::TRANSACTION_RESPONSE) {

        // parse the TRANSACTION_RESPONSE
        TRANSACTION_RESPONSE transaction_response;
        msg.msg.convert(transaction_response);

        // print the TRANSACTION_RESPONSE
        std::cout << "[client] received TRANSACTION_RESPONSE\n";
        std::cout << "        transaction_response.type:" << unsigned(transaction_response.type) << "\n";
        std::cout << "        transaction_response.token:" << transaction_response.token << "\n";
        std::cout.precision(2);
        std::cout << std::fixed;
        std::cout << "        transaction_response.fee:" << transaction_response.fee << "\n";

    } else {
        std::cout << "[client] received unknown message\n";
    }

    // create an ADD_BALANCE_REQUEST message
    ADD_BALANCE_REQUEST add_balance_request;
    add_balance_request.user = 1;
    add_balance_request.token = "KynVSJrWp3BLAlytnlVSOaTVa6zOUrup";
    add_balance_request.bank = 1;
    add_balance_request.iban = "TR6136876433976928";
    add_balance_request.amount = (double_t) 100;

    // pack the ADD_BALANCE_REQUEST message
    msg = MSG{MSG_ID::ADD_BALANCE_REQUEST, msgpack::object(add_balance_request, z)};

    // send the ADD_BALANCE_REQUEST message
    std::stringstream buffer;
    msgpack::pack(buffer, msg);
    const std::string payload = buffer.str();
    sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
    std::cout << "[client] sent ADD_BALANCE_REQUEST\n";

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

    // handle the ADD_BALANCE_RESPONSE message
    if (msg.id == MSG_ID::ADD_BALANCE_RESPONSE) {

        // parse the ADD_BALANCE_RESPONSE
        ADD_BALANCE_RESPONSE add_balance_response;
        msg.msg.convert(add_balance_response);

        // print the ADD_BALANCE_RESPONSE
        std::cout << "[client] received ADD_BALANCE_RESPONSE\n";
        std::cout.precision(2);
        std::cout << std::fixed;
        std::cout << "        add_balance_response.amount:" << add_balance_response.amount << "\n";

    } else {
        std::cout << "[client] received unknown message\n";
    }

    // create a ACCOUNT_LIST_REQUEST message
    ACCOUNT_LIST_REQUEST account_list_request;
    account_list_request.user = 16;
    account_list_request.token = "KynVSJrWp3BLAlytnlVSOaTVa6zOUrup";
    account_list_request.bank = 10;

    // pack the ACCOUNT_LIST_REQUEST message
    msg = MSG{MSG_ID::ACCOUNT_LIST_REQUEST, msgpack::object(account_list_request, z)};

    // send the ACCOUNT_LIST_REQUEST message
    std::stringstream buffer;
    msgpack::pack(buffer, msg);
    const std::string payload = buffer.str();
    sock.send(zmq::buffer(payload), zmq::send_flags::dontwait);
    std::cout << "[client] sent ACCOUNT_LIST_REQUEST\n";

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

    // handle the ACCOUNT_LIST_RESPONSE message
    if (msg.id == MSG_ID::ACCOUNT_LIST_RESPONSE) {

        // parse the ACCOUNT_LIST_RESPONSE
        ACCOUNT_LIST_RESPONSE account_list_response;
        msg.msg.convert(account_list_response);

        // print the ACCOUNT_LIST_RESPONSE
        std::cout << "[client] received ACCOUNT_LIST_RESPONSE\n";
        for (const auto &account: account_list_response.accounts) {
            std::cout << "        account.iban:" << account.iban;
            std::cout << ", account.user:" << unsigned(account.user);
            std::cout << ", account.bank:" << unsigned(account.bank);
            std::cout.precision(2);
            std::cout << std::fixed;
            std::cout << ", account.balance:" << account.balance << "\n";
        }

    } else {
        std::cout << "[client] received unknown message\n";
    }

    */

    // program ends as expected
    return 0;
}
