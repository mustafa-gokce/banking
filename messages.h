#ifndef BANKING_MESSAGES_H
#define BANKING_MESSAGES_H

#include <msgpack.hpp>

/*
 * This is a list of all the messages that can be sent between the client and the server.
 */
enum class MSG_ID : uint16_t {
    NONE = 0,
    HEARTBEAT = 1,
    BANK_LIST_REQUEST = 2,
    BANK_LIST_RESPONSE = 3,
    LOGIN_REQUEST = 4,
    LOGIN_RESPONSE = 5,
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
 * This is a list of all the heartbeat types.
 */
enum class HEARTBEAT_TYPE : uint16_t {
    NONE = 0,
    CLIENT = 0,
    SERVER = 1,
};
MSGPACK_ADD_ENUM(HEARTBEAT_TYPE)

/*
 * This is the message that is sent from an endpoint to the other endpoint to indicate that it is still alive.
 */
class HEARTBEAT {
public:
    HEARTBEAT_TYPE type{HEARTBEAT_TYPE::NONE};
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
 * This is the message that is sent from the client to the server to request a login.
 */
class LOGIN_REQUEST {
public:
    std::string username{};
    std::string password{};
    uint16_t bank_id{};
    MSGPACK_DEFINE (username, password, bank_id);
};

/*
 * This is the message that is sent from the server to the client in response to a LOGIN_REQUEST.
 */
class LOGIN_RESPONSE {
public:
    uint32_t user_id{};
    std::string citizen_name{};
    std::string token{};
    MSGPACK_DEFINE (user_id, citizen_name, token);
};

#endif //BANKING_MESSAGES_H
