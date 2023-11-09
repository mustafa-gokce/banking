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

#endif //BANKING_MESSAGES_H
