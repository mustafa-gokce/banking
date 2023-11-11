#include "src/Messages.h"
#include "src/Client.h"

int main(int argc, char *argv[]) {

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

    // send transaction request (like sending money to another users IBAN, consult accounts table for real IBANs)
    client.send_transaction_request(login_response.id, login_response.token, login_response.bank,
                                    account_list_response.accounts[0].iban, "TR2543267363394138", 2500);

    // receive transaction response
    client.receive_transaction_response();

    // send logout request
    client.send_logout_request(login_response.user, login_response.token);

    // receive logout response
    client.receive_logout_response();

    // program ends as expected
    return 0;
}
