#ifndef BANKING_MESSAGES_H
#define BANKING_MESSAGES_H

#include <msgpack.hpp>

/*
 * This is a list of all the messages that can be sent between the client and the server.
 */
enum class MSG_ID : uint16_t {
    NONE = 0,
    PING = 1,
    BANK_LIST_REQUEST = 2,
    BANK_LIST_RESPONSE = 3,
    LOGIN_REQUEST = 4,
    LOGIN_RESPONSE = 5,
    LOGOUT_REQUEST = 6,
    LOGOUT_RESPONSE = 7,
    ACCOUNT_LIST_REQUEST = 8,
    ACCOUNT_LIST_RESPONSE = 9,
    ADD_BALANCE_REQUEST = 10,
    ADD_BALANCE_RESPONSE = 11,
    TRANSACTION_REQUEST = 12,
    TRANSACTION_RESPONSE = 13,
};
MSGPACK_ADD_ENUM(MSG_ID)

/*
 * This is the base class for all messages.
 * It contains the message ID and the message itself.
 */
class MSG {
public:
    MSG_ID id{MSG_ID::NONE};
    msgpack::object msg{};
    MSGPACK_DEFINE (id, msg);
};

/*
 * This is a list of all the ping types.
 */
enum class PING_TYPE : uint8_t {
    NONE = 0,
    CLIENT = 1,
    SERVER = 2,
};
MSGPACK_ADD_ENUM(PING_TYPE)

/*
 * This is the message that is sent from an endpoint to the other endpoint to check that it is alive.
 */
class PING {
public:
    PING_TYPE type{PING_TYPE::NONE};
    std::string token{};
    uint64_t client_time{};
    uint64_t server_time{};
    MSGPACK_DEFINE (type, token, client_time, server_time);
};

/*
 * This is the message that is sent from the client to the server to request a login.
 */
class LOGIN_REQUEST {
public:
    std::string user{};
    std::string pass{};
    uint16_t bank{};
    MSGPACK_DEFINE (user, pass, bank);
};

/*
 * This is a list of all the login response types.
 */
enum class LOGIN_RESPONSE_TYPE : uint8_t {
    LOGIN_SUCCESS = 0,
    SERVER_ERROR = 1,
    INVALID_USERNAME_OR_PASSWORD = 2,
    INVALID_BANK_ID = 3,
    ALREADY_LOGGED_IN = 4,
    UNKNOWN = 255,
};
MSGPACK_ADD_ENUM(LOGIN_RESPONSE_TYPE)

/*
 * This is the message that is sent from the server to the client in response to a LOGIN_REQUEST.
 */
class LOGIN_RESPONSE {
public:
    LOGIN_RESPONSE_TYPE type{LOGIN_RESPONSE_TYPE::UNKNOWN};
    uint32_t id{};
    uint16_t bank{};
    uint32_t citizen{};
    std::string name{};
    std::string user{};
    std::string token{};
    MSGPACK_DEFINE (type, id, bank, citizen, name, user, token);
};

/*
 * This is the message that is sent from the client to the server to request a logout.
 */
class LOGOUT_REQUEST {
public:
    std::string user{};
    std::string token{};
    MSGPACK_DEFINE (user, token);
};

/*
 * This is a list of all the logout response types.
 */
enum class LOGOUT_RESPONSE_TYPE : uint8_t {
    LOGOUT_SUCCESS = 0,
    SERVER_ERROR = 1,
    NOT_LOGGED_IN = 2,
    INVALID_TOKEN = 3,
    UNKNOWN = 255,
};
MSGPACK_ADD_ENUM(LOGOUT_RESPONSE_TYPE)

/*
 * This is the message that is sent from the server to the client in response to a LOGOUT_REQUEST.
 */
class LOGOUT_RESPONSE {
public:
    LOGOUT_RESPONSE_TYPE type{LOGOUT_RESPONSE_TYPE::UNKNOWN};
    MSGPACK_DEFINE (type);
};

/*
 * This is the message that is sent from the client to the server to request a list of banks.
 */
class BANK_LIST_REQUEST {
public:
    MSGPACK_DEFINE ();
};

/*
 * This is the message sub-object that is sent from the server to the client in response to a BANK_LIST_REQUEST.
 */
class Bank {
public:
    uint16_t id{};
    std::string name{};
    MSGPACK_DEFINE (id, name);
};

/*
 * This is the message that is sent from the server to the client in response to a BANK_LIST_REQUEST.
 */
class BANK_LIST_RESPONSE {
public:
    std::vector<Bank> banks{};
    MSGPACK_DEFINE (banks);
};

/*
 * This is the message that is sent from the client to the server to request a list of IBANs.
 */
class ACCOUNT_LIST_REQUEST {
public:
    uint32_t user{};
    std::string token{};
    uint16_t bank{};
    MSGPACK_DEFINE (user, token, bank);
};

/*
 * This is the message sub-object that is sent from the server to the client in response to a ACCOUNT_LIST_RESPONSE.
 */
class Account {
public:
    std::string iban{};
    uint32_t user{};
    uint16_t bank{};
    double_t balance{};
    MSGPACK_DEFINE (iban, user, bank, balance);
};

/*
 * This is the message that is sent from the server to the client in response to a ACCOUNT_LIST_REQUEST.
 */
class ACCOUNT_LIST_RESPONSE {
public:
    std::vector<Account> accounts{};
    MSGPACK_DEFINE (accounts);
};

/*
 * This is the message that is sent from the client to the server to add balance to an IBAN as a superuser.
 */
class ADD_BALANCE_REQUEST {
public:
    uint32_t user{};
    std::string token{};
    uint16_t bank{};
    std::string iban{};
    double_t amount{};
    MSGPACK_DEFINE (user, token, bank, iban, amount);
};

/*
 * This is the message that is sent from the server to the client in response to an ADD_BALANCE_REQUEST.
 * Inheriting from ADD_BALANCE_REQUEST is a hack to reuse the MSGPACK_DEFINE macro.
 * Echo back the total balance of the account after adding the amount.
 */
class ADD_BALANCE_RESPONSE : public ADD_BALANCE_REQUEST {
};

/*
 * This is the message that is sent from the client to the server to request a transaction.
 */
class TRANSACTION_REQUEST {
public:
    std::string user{};
    std::string token{};
    std::string from{};
    std::string to{};
    double_t amount{};
    MSGPACK_DEFINE (user, token, from, to, amount);
};

/*
 * This is a list of all the transaction response types.
 */
enum class TRANSACTION_RESPONSE_TYPE : uint8_t {
    TRANSACTION_SUCCESS = 0,
    SERVER_ERROR = 1,
    NOT_LOGGED_IN = 2,
    INVALID_TOKEN = 3,
    INVALID_FROM_IBAN = 4,
    INVALID_TO_IBAN = 5,
    INVALID_AMOUNT = 6,
    INSUFFICIENT_FUNDS = 7,
    UNKNOWN = 255,
};
MSGPACK_ADD_ENUM(TRANSACTION_RESPONSE_TYPE)

/*
 * This is the message that is sent from the server to the client in response to a TRANSACTION_REQUEST.
 */
class TRANSACTION_RESPONSE {
public:
    TRANSACTION_RESPONSE_TYPE type{TRANSACTION_RESPONSE_TYPE::UNKNOWN};
    std::string token{};
    float_t fee{};
    MSGPACK_DEFINE (type, token, fee);
};

#endif //BANKING_MESSAGES_H
