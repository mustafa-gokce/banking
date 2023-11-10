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

    // send login request (consult, users and accounts tables in database)
    client.send_login_request("aylin.yilmaz42@gmail.com", "G3l!y@m!zP@ss", 1);

    // receive login response (we need to create a LOGIN_RESPONSE object to hold login token)
    LOGIN_RESPONSE login_response;
    client.receive_login_response(login_response);

    // send account list request
    client.send_account_list_request(login_response.id, login_response.token, login_response.bank);

    // receive account list response
    ACCOUNT_LIST_RESPONSE account_list_response;
    client.receive_account_list_response(account_list_response);

    // send add balance request (like an ATM deposit)
    client.send_add_balance_request(login_response.id, login_response.token, login_response.bank,
                                    account_list_response.accounts[0].iban, 1000);

    // receive add balance response
    client.receive_add_balance_response();

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

    */

    // program ends as expected
    return 0;
}
